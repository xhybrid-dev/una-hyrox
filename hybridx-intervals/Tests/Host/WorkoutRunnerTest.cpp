#include <gtest/gtest.h>

#include "WorkoutRunner.hpp"

using namespace Intervals;

namespace
{
constexpr Target kPace { TargetKind::Pace, 240, 260 };   // 4:00-4:20 per km

Sample pace(uint16_t secPerKm)
{
    Sample s;
    s.hasPace      = true;
    s.paceSecPerKm = secPerKm;
    return s;
}

// A long timed step with a pace target, then a 60 s rest.
Workout paced()
{
    Workout w;
    w.addStep(Step { DurationKind::Time, 600000, StepIntensity::Active, kPace, 0 });
    w.addStep(Step { DurationKind::Time, 60000, StepIntensity::Rest, {}, 0 });
    return w;
}
} // namespace

TEST(WorkoutRunner, StartReportsTheFirstStepAndViewDescribesIt)
{
    const Workout w = paced();
    WorkoutRunner r;
    const auto    res = r.start(w, 1000, 0);
    EXPECT_TRUE(res.stepStarted);
    EXPECT_FALSE(res.stepEnded);
    const auto v = r.view(1000, 0);
    EXPECT_TRUE(v.running);
    EXPECT_EQ(v.stepIndex, 0u);
    EXPECT_EQ(v.remainingMs, 600000u);
    EXPECT_EQ(v.nextIndex, 1);
    EXPECT_TRUE(v.settling);
}

TEST(WorkoutRunner, NoCueWhileSettlingThenOneCueWhenOffTarget)
{
    const Workout w = paced();
    WorkoutRunner r;
    r.start(w, 0, 0);
    // Too slow from the start, but the first 15 s are the settling time.
    uint32_t t = 0;
    for (; t < WorkoutRunner::kSettleMs; t += 1000) {
        EXPECT_FALSE(r.tick(t, 0, pace(300)).cue) << t;
    }
    // Then three agreeing ticks are needed (the debouncer): the cue is on the third.
    EXPECT_FALSE(r.tick(t, 0, pace(300)).cue);
    t += 1000;
    EXPECT_FALSE(r.tick(t, 0, pace(300)).cue);
    t += 1000;
    const auto res = r.tick(t, 0, pace(300));
    EXPECT_TRUE(res.cue);
    EXPECT_EQ(res.cueState, ZoneState::Under);
    EXPECT_FALSE(res.reminder);
    EXPECT_EQ(r.view(t, 0).zone, ZoneState::Under);
}

TEST(WorkoutRunner, RemindsEveryMinuteWhileOffAndStopsOnceBackIn)
{
    const Workout w = paced();
    WorkoutRunner r;
    r.start(w, 0, 0);
    uint32_t t       = 0;
    int      cues    = 0;
    int      remind  = 0;
    // 20 s settling and debounce, then 2 minutes too fast.
    for (; t <= 140000; t += 1000) {
        const auto res = r.tick(t, 0, pace(200));
        cues += res.cue ? 1 : 0;
        remind += res.reminder ? 1 : 0;
    }
    EXPECT_EQ(cues, 3) << "the change, then a reminder each minute";
    EXPECT_EQ(remind, 2);
    // Back in the band: no cue for that (the screen shows it), and no more reminders.
    int later = 0;
    for (; t <= 300000; t += 1000) {
        later += r.tick(t, 0, pace(250)).cue ? 1 : 0;
    }
    EXPECT_EQ(later, 0);
    EXPECT_EQ(r.view(t, 0).zone, ZoneState::InZone);
}

TEST(WorkoutRunner, AnOpenTargetNeverCues)
{
    Workout w;
    w.addStep(Step { DurationKind::Time, 600000, StepIntensity::Active, {}, 0 });
    WorkoutRunner r;
    r.start(w, 0, 0);
    for (uint32_t t = 0; t < 200000; t += 1000) {
        ASSERT_FALSE(r.tick(t, 0, pace(999)).cue);
    }
    EXPECT_EQ(r.view(200000, 0).zone, ZoneState::NoTarget);
}

TEST(WorkoutRunner, StepEndsOnActiveTimeAndReportsALapThenCompletes)
{
    const Workout w = paced();
    WorkoutRunner r;
    r.start(w, 5000, 0);
    auto res = r.tick(5000 + 599000, 0, pace(250));
    EXPECT_FALSE(res.stepEnded);
    res = r.tick(5000 + 600000, 0, pace(250));
    EXPECT_TRUE(res.stepEnded);
    EXPECT_EQ(res.endedStep, 0u);
    EXPECT_TRUE(res.stepStarted);
    EXPECT_FALSE(res.completed);
    EXPECT_EQ(r.view(605000, 0).stepIndex, 1u);

    res = r.tick(5000 + 660000, 0, pace(250));
    EXPECT_TRUE(res.stepEnded);
    EXPECT_EQ(res.endedStep, 1u);
    EXPECT_TRUE(res.completed);
    EXPECT_FALSE(r.view(665000, 0).running);
    EXPECT_TRUE(r.view(665000, 0).completed);
    // Ticks after the end do nothing.
    res = r.tick(700000, 0, pace(250));
    EXPECT_FALSE(res.stepEnded || res.cue || res.completed);
}

TEST(WorkoutRunner, DistanceRemainingCountsFromTheStepsStart)
{
    Workout w;
    w.addStep(Step { DurationKind::Open, 0, StepIntensity::Warmup, {}, 0 });
    w.addStep(Step { DurationKind::Distance, 40000, StepIntensity::Active, {}, 0 });   // 400 m
    WorkoutRunner r;
    r.start(w, 0, 0);
    r.tick(60000, 123400, Sample {});            // 1,234 m into the warm-up
    const auto res = r.advance(61000, 123400);   // press: start the 400
    EXPECT_TRUE(res.stepEnded);
    EXPECT_EQ(res.endedStep, 0u);
    EXPECT_EQ(r.view(61000, 123400).remainingCm, 40000u);
    r.tick(90000, 133400, Sample {});            // 100 m on
    EXPECT_EQ(r.view(90000, 133400).remainingCm, 30000u);
    const auto end = r.tick(150000, 163400, Sample {});
    EXPECT_TRUE(end.completed);
}

TEST(WorkoutRunner, ANewStepClearsTheZoneAndSettlesAgain)
{
    Workout w;
    w.addStep(Step { DurationKind::Time, 60000, StepIntensity::Active, kPace, 0 });
    w.addStep(Step { DurationKind::Time, 600000, StepIntensity::Active, kPace, 0 });
    WorkoutRunner r;
    r.start(w, 0, 0);
    for (uint32_t t = 0; t < 60000; t += 1000) {
        r.tick(t, 0, pace(300));
    }
    ASSERT_EQ(r.view(59000, 0).zone, ZoneState::Under);
    const auto res = r.tick(60000, 0, pace(300));
    ASSERT_TRUE(res.stepStarted);
    const auto v = r.view(60000, 0);
    EXPECT_EQ(v.zone, ZoneState::NoTarget);
    EXPECT_TRUE(v.settling);
    EXPECT_FALSE(r.tick(61000, 0, pace(300)).cue);
}

TEST(WorkoutRunner, WorksAcrossTheMillisecondClockWrapping)
{
    const Workout  w = paced();
    WorkoutRunner  r;
    const uint32_t start = 0xFFFFFFFFu - 5000u;
    r.start(w, start, 0);
    const auto res = r.tick(start + 600000u, 0, pace(250));   // wraps past zero
    EXPECT_TRUE(res.stepEnded);
    EXPECT_EQ(r.view(start + 600000u, 0).stepIndex, 1u);
}

TEST(WorkoutRunner, StopForgetsTheWorkout)
{
    const Workout w = paced();
    WorkoutRunner r;
    r.start(w, 0, 0);
    r.stop();
    EXPECT_FALSE(r.active());
    EXPECT_FALSE(r.view(0, 0).running);
    EXPECT_FALSE(r.tick(1000, 0, pace(300)).stepEnded);
    EXPECT_FALSE(r.advance(1000, 0).stepEnded);
}
