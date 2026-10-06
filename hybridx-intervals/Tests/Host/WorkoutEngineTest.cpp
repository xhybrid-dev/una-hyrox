#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "WorkoutEngine.hpp"
#include "WorkoutEvents.hpp"
#include "WorkoutTypes.hpp"

using namespace Intervals;

namespace
{
bool has(const Events& events, EventKind kind)
{
    for (uint8_t i = 0; i < events.count; ++i) {
        if (events.items[i].kind == kind) {
            return true;
        }
    }
    return false;
}
} // namespace

TEST(WorkoutEngine, TimeStepCompletesExactlyAtItsThreshold)
{
    Workout w;
    w.addStep(Step { DurationKind::Time, 1000, StepIntensity::Active, {}, 0 });
    w.addStep(Step { DurationKind::Time, 1000, StepIntensity::Active, {}, 0 });

    WorkoutEngine engine;
    Events        startEvents;
    engine.start(w, 0, 0, startEvents);
    ASSERT_EQ(engine.stepIndex(), 0u);

    Events earlyEvents;
    engine.tick(999, 0, earlyEvents);
    EXPECT_FALSE(has(earlyEvents, EventKind::StepCompleted)) << "one ms early must not complete the step";
    EXPECT_EQ(engine.stepIndex(), 0u);

    Events onTimeEvents;
    engine.tick(1000, 0, onTimeEvents);
    EXPECT_TRUE(has(onTimeEvents, EventKind::StepCompleted));
    EXPECT_EQ(engine.stepIndex(), 1u);
}

TEST(WorkoutEngine, DistanceStepCompletesExactlyAtItsThreshold)
{
    Workout w;
    w.addStep(Step { DurationKind::Distance, 50000, StepIntensity::Active, {}, 0 });   // 500 m in cm
    w.addStep(Step { DurationKind::Time, 1000, StepIntensity::Active, {}, 0 });

    WorkoutEngine engine;
    Events        startEvents;
    engine.start(w, 0, 0, startEvents);

    Events beforeEvents;
    engine.tick(0, 49999, beforeEvents);
    EXPECT_EQ(engine.stepIndex(), 0u);

    Events atEvents;
    engine.tick(0, 50000, atEvents);
    EXPECT_EQ(engine.stepIndex(), 1u);
}

TEST(WorkoutEngine, OpenStepNeverAutoAdvancesOnlyManualDoes)
{
    Workout w;
    w.addStep(Step { DurationKind::Open, 0, StepIntensity::Active, {}, 0 });
    w.addStep(Step { DurationKind::Time, 1000, StepIntensity::Active, {}, 0 });

    WorkoutEngine engine;
    Events        startEvents;
    engine.start(w, 0, 0, startEvents);

    for (uint32_t t = 0; t < 1000000; t += 1000) {
        Events tickEvents;
        engine.tick(t, 0, tickEvents);
        ASSERT_EQ(engine.stepIndex(), 0u) << "an Open step must never complete on its own, at t=" << t;
    }

    Events manualEvents;
    engine.advanceManually(1000000, 0, manualEvents);
    EXPECT_EQ(engine.stepIndex(), 1u);
    EXPECT_TRUE(has(manualEvents, EventKind::StepCompleted));
}

TEST(WorkoutEngine, RepeatBlockVisitsStepsInOrderExactlyMTimesThenFallsThrough)
{
    // Steps 0,1 repeated 3 times, then step 3.
    Workout w;
    w.addStep(Step { DurationKind::Time, 100, StepIntensity::Active, {}, 0 });   // 0
    w.addStep(Step { DurationKind::Time, 100, StepIntensity::Rest, {}, 0 });     // 1
    w.addStep(Step { DurationKind::RepeatUntilStepsComplete, 0, StepIntensity::Active, {}, 3 }); // 2
    w.addStep(Step { DurationKind::Time, 100, StepIntensity::Cooldown, {}, 0 }); // 3

    WorkoutEngine engine;
    Events        startEvents;
    engine.start(w, 0, 0, startEvents);

    std::vector<uint8_t> visited;
    visited.push_back(engine.stepIndex());
    uint32_t now = 0;
    for (int i = 0; i < 20 && !engine.completed(); ++i) {
        now += 100;
        Events tickEvents;
        engine.tick(now, 0, tickEvents);
        if (!engine.completed()) {
            visited.push_back(engine.stepIndex());
        }
    }

    // 0,1 (pass1), 0,1 (pass2), 0,1 (pass3), 3
    std::vector<uint8_t> expected = { 0, 1, 0, 1, 0, 1, 3 };
    EXPECT_EQ(visited, expected);
}

TEST(WorkoutEngine, RepeatBlockLoopedFiresOnEveryLoopRepeatBlockCompletedOnce)
{
    Workout w;
    w.addStep(Step { DurationKind::Time, 100, StepIntensity::Active, {}, 0 });
    w.addStep(Step { DurationKind::RepeatUntilStepsComplete, 0, StepIntensity::Active, {}, 2 });
    w.addStep(Step { DurationKind::Time, 100, StepIntensity::Cooldown, {}, 0 });

    WorkoutEngine engine;
    Events        startEvents;
    engine.start(w, 0, 0, startEvents);

    int      loopedCount    = 0;
    int      completedCount = 0;
    uint32_t now            = 0;
    for (int i = 0; i < 10 && !engine.completed(); ++i) {
        now += 100;
        Events tickEvents;
        engine.tick(now, 0, tickEvents);
        for (uint8_t j = 0; j < tickEvents.count; ++j) {
            if (tickEvents.items[j].kind == EventKind::RepeatBlockLooped) {
                ++loopedCount;
            }
            if (tickEvents.items[j].kind == EventKind::RepeatBlockCompleted) {
                ++completedCount;
            }
        }
    }

    EXPECT_EQ(loopedCount, 1) << "2 total passes = 1 loop-back, then fall through";
    EXPECT_EQ(completedCount, 1);
}

TEST(WorkoutEngine, WorkoutCompletedFiresExactlyOnceAndFurtherTicksAreNoOps)
{
    Workout w;
    w.addStep(Step { DurationKind::Time, 100, StepIntensity::Active, {}, 0 });

    WorkoutEngine engine;
    Events        startEvents;
    engine.start(w, 0, 0, startEvents);

    Events finishEvents;
    engine.tick(100, 0, finishEvents);
    EXPECT_TRUE(engine.completed());
    EXPECT_TRUE(has(finishEvents, EventKind::WorkoutCompleted));

    for (int i = 0; i < 5; ++i) {
        Events noOpEvents;
        engine.tick(100 + i, 0, noOpEvents);
        EXPECT_EQ(noOpEvents.count, 0u) << "tick() after completion must do nothing";
    }
}

TEST(WorkoutEngine, ElapsedTimeIsCorrectAcrossAClockWraparound)
{
    Workout w;
    w.addStep(Step { DurationKind::Time, 5000, StepIntensity::Active, {}, 0 });
    w.addStep(Step { DurationKind::Time, 1000, StepIntensity::Active, {}, 0 });

    WorkoutEngine  engine;
    Events         startEvents;
    const uint32_t startMs = 0xFFFFFFFFu - 2000u;   // wraps 2000 ms into the step
    engine.start(w, startMs, 0, startEvents);

    // 4999 ms elapsed (wrapped): step must not have completed yet.
    Events beforeEvents;
    engine.tick(startMs + 4999u, 0, beforeEvents);   // wraps past UINT32_MAX
    EXPECT_EQ(engine.stepIndex(), 0u);

    Events atEvents;
    engine.tick(startMs + 5000u, 0, atEvents);
    EXPECT_EQ(engine.stepIndex(), 1u);
}

TEST(WorkoutEngine, StepRemainingMsCountsDownOnlyForTimeSteps)
{
    Workout w;
    w.addStep(Step { DurationKind::Time, 1000, StepIntensity::Active, {}, 0 });
    w.addStep(Step { DurationKind::Open, 0, StepIntensity::Active, {}, 0 });

    WorkoutEngine engine;
    Events        startEvents;
    engine.start(w, 0, 0, startEvents);
    EXPECT_EQ(engine.stepRemainingMs(400), 600u);
    EXPECT_EQ(engine.stepRemainingMs(1000), 0u);

    Events manualEvents;
    engine.advanceManually(1000, 0, manualEvents);
    EXPECT_EQ(engine.stepRemainingMs(5000), 0u) << "Open steps report no remaining time";
}

namespace
{
// Warm-up, 3 x (work, rest), cool-down; every step Open so the test walks it by hand.
Workout threeRepWorkout()
{
    Workout w;
    w.addStep(Step { DurationKind::Open, 0, StepIntensity::Warmup, {}, 0 });                    // 0
    w.addStep(Step { DurationKind::Open, 0, StepIntensity::Active, {}, 0 });                    // 1
    w.addStep(Step { DurationKind::Open, 0, StepIntensity::Rest, {}, 0 });                      // 2
    w.addStep(Step { DurationKind::RepeatUntilStepsComplete, 1, StepIntensity::Active, {}, 3 }); // 3
    w.addStep(Step { DurationKind::Open, 0, StepIntensity::Cooldown, {}, 0 });                  // 4
    return w;
}
} // namespace

TEST(WorkoutEngine, RepeatPositionIsRightOnEveryPassIncludingTheFirst)
{
    const Workout w = threeRepWorkout();
    WorkoutEngine engine;
    Events        e;
    engine.start(w, 0, 0, e);

    uint16_t pass = 9, total = 9;
    EXPECT_FALSE(engine.repeatPosition(pass, total)) << "the warm-up is outside the block";
    EXPECT_EQ(pass, 0u);
    EXPECT_EQ(total, 0u);

    // Expected (step, pass) for each manual advance through the workout.
    const uint8_t  steps[] = { 1, 2, 1, 2, 1, 2, 4 };
    const uint16_t passes[] = { 1, 1, 2, 2, 3, 3, 0 };
    for (size_t i = 0; i < sizeof(steps); ++i) {
        Events a;
        engine.advanceManually(0, 0, a);
        ASSERT_EQ(engine.stepIndex(), steps[i]) << "advance " << i;
        const bool inBlock = engine.repeatPosition(pass, total);
        EXPECT_EQ(inBlock, passes[i] != 0) << "advance " << i;
        EXPECT_EQ(pass, passes[i]) << "advance " << i;
        EXPECT_EQ(total, inBlock ? 3u : 0u) << "advance " << i;
    }
}

TEST(WorkoutEngine, NextStepIndexFollowsRepeatsWithoutChangingAnything)
{
    const Workout w = threeRepWorkout();
    WorkoutEngine engine;
    Events        e;
    engine.start(w, 0, 0, e);

    // From each position, nextStepIndex() must predict what advanceManually() then does.
    for (int guard = 0; guard < 20 && !engine.completed(); ++guard) {
        const int16_t predicted = engine.nextStepIndex();
        EXPECT_EQ(engine.nextStepIndex(), predicted) << "a second call must give the same answer";
        Events a;
        engine.advanceManually(0, 0, a);
        if (engine.completed()) {
            EXPECT_EQ(predicted, -1);
        } else {
            EXPECT_EQ(predicted, engine.stepIndex());
        }
    }
    EXPECT_TRUE(engine.completed());
    EXPECT_EQ(engine.nextStepIndex(), -1);
}

TEST(WorkoutEngine, PositionQueriesAreSafeBeforeStart)
{
    WorkoutEngine engine;
    uint16_t      pass = 1, total = 1;
    EXPECT_FALSE(engine.repeatPosition(pass, total));
    EXPECT_EQ(engine.nextStepIndex(), -1);
}

TEST(WorkoutEngine, BackToBackBlocksEachReportTheirOwnPosition)
{
    Workout w;
    w.addStep(Step { DurationKind::Open, 0, StepIntensity::Active, {}, 0 });                    // 0
    w.addStep(Step { DurationKind::RepeatUntilStepsComplete, 0, StepIntensity::Active, {}, 2 }); // 1
    w.addStep(Step { DurationKind::Open, 0, StepIntensity::Active, {}, 0 });                    // 2
    w.addStep(Step { DurationKind::RepeatUntilStepsComplete, 2, StepIntensity::Active, {}, 4 }); // 3
    WorkoutEngine engine;
    Events        e;
    engine.start(w, 0, 0, e);

    uint16_t pass = 0, total = 0;
    ASSERT_TRUE(engine.repeatPosition(pass, total));
    EXPECT_EQ(pass, 1u);
    EXPECT_EQ(total, 2u);
    Events a1;
    engine.advanceManually(0, 0, a1);   // second pass of block one
    ASSERT_TRUE(engine.repeatPosition(pass, total));
    EXPECT_EQ(pass, 2u);
    Events a2;
    engine.advanceManually(0, 0, a2);   // block two, first pass
    ASSERT_EQ(engine.stepIndex(), 2u);
    ASSERT_TRUE(engine.repeatPosition(pass, total));
    EXPECT_EQ(pass, 1u);
    EXPECT_EQ(total, 4u);
}
