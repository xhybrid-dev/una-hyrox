// HybridX Workout: running a plan over the workout model (PlanRunner, W1).

#include <gtest/gtest.h>

#include <vector>

#include "PlanRunner.hpp"

using namespace Workout;

namespace {

// A plain start, one before the 32-bit clock wraps, and one just past it.
constexpr uint32_t kStarts[] = {0u, 123456u, 0xFFFFFFFFu - 200000u};

PlanConfig of(SessionType t)
{
    PlanConfig c;
    c.type = t;
    return c;
}

struct Fixture {
    WorkoutModel m;
    PlanRunner   r;
    uint32_t     t0 = 0;
    std::vector<std::pair<uint32_t, Cue>> cues;   // (ms from the start, cue)

    void start(const PlanConfig &plan, uint32_t start = 0, uint32_t autoLapMs = 0)
    {
        t0 = start;
        WorkoutConfig c;
        c.autoLapMs = autoLapMs;
        r.start(plan, m, t0, c);
    }
    /// A sample and a tick every @p stepMs from @p fromS to @p toS seconds (inclusive).
    void run(uint32_t fromMs, uint32_t toMs, uint32_t stepMs = 1000, uint16_t hr = 120)
    {
        for (uint32_t off = fromMs; off <= toMs; off += stepMs) {
            const uint32_t now = t0 + off;
            m.addSample(now, hr);
            const Cue c = r.tick(m, now);
            if (c != Cue::None) {
                cues.emplace_back(off, c);
            }
        }
    }
    Cue press(uint32_t offMs)
    {
        const Cue c = r.press(m, t0 + offMs);
        if (c != Cue::None) {
            cues.emplace_back(offMs, c);
        }
        return c;
    }
    int count(Cue c) const
    {
        int n = 0;
        for (const auto &e : cues) {
            n += e.second == c ? 1 : 0;
        }
        return n;
    }
};

} // namespace

TEST(PlanRunner, PlainPressTakesALapAndTickDoesNothing)
{
    Fixture f;
    f.start(of(SessionType::Plain), 0, 60000);   // auto lap 1 min still works
    EXPECT_FALSE(f.r.hasPlan());
    f.run(0, 30000);
    EXPECT_EQ(f.press(30500), Cue::Round);
    EXPECT_EQ(f.m.lapCount(), 2);
    f.run(31000, 91000);
    EXPECT_EQ(f.m.lapCount(), 3);                 // the auto lap, a minute after the button lap
    EXPECT_TRUE(f.m.lapAt(1).autoEnded);
    EXPECT_TRUE(f.cues.size() == 1u);             // only the press: the runner did not tick
    EXPECT_EQ(f.r.view(f.m, 91000).steps, 0);
}

TEST(PlanRunner, StrengthSetsRestsAndTheLastSetEndsIt)
{
    for (uint32_t s0 : kStarts) {
        Fixture f;
        f.start(of(SessionType::Strength), s0);   // 3 sets, 60 s rest
        f.run(0, 30000);
        EXPECT_EQ(f.press(30000), Cue::Rest);
        EXPECT_EQ(f.m.lapCount(), 2);
        EXPECT_EQ(f.m.lapAt(0).kind, uint8_t(StepKind::Work));
        EXPECT_EQ(f.m.lapAt(0).round, 1);
        EXPECT_FALSE(f.m.lapAt(0).autoEnded);
        EXPECT_EQ(f.m.lapAt(1).kind, uint8_t(StepKind::Rest));

        const PlanView rest = f.r.view(f.m, s0 + 40000);
        EXPECT_EQ(rest.kind, uint8_t(StepKind::Rest));
        EXPECT_EQ(rest.remainingMs, 50000u);

        f.run(31000, 95000);
        // A warning at 87 s, set 2 at 90 s: the rest ran its 60 s.
        ASSERT_EQ(f.cues.size(), 3u);
        EXPECT_EQ(f.cues[1], std::make_pair(87000u, Cue::Warning));
        EXPECT_EQ(f.cues[2], std::make_pair(90000u, Cue::Work));
        EXPECT_TRUE(f.m.lapAt(1).autoEnded);       // the rest ended by time
        EXPECT_EQ(f.m.lapAt(1).activeMs, 60000u);
        EXPECT_EQ(f.m.lapAt(2).round, 2);

        EXPECT_EQ(f.press(120000), Cue::Rest);     // set 2 done
        EXPECT_EQ(f.press(130000), Cue::Work);     // rest skipped: set 3 now
        EXPECT_EQ(f.m.lapAt(3).activeMs, 10000u);
        EXPECT_EQ(f.r.view(f.m, s0 + 130000).round, 3);
        EXPECT_EQ(f.press(150000), Cue::Done);     // the last set: no rest after it
        EXPECT_TRUE(f.r.done());
        EXPECT_EQ(f.m.state(), RunState::Paused);
        EXPECT_EQ(f.m.lapCount(), 5);
        EXPECT_EQ(f.press(160000), Cue::None);
        EXPECT_EQ(f.r.tick(f.m, s0 + 160000), Cue::None);
    }
}

TEST(PlanRunner, AStrengthSetEndsByItselfAtItsCap)
{
    Fixture f;
    PlanConfig c = of(SessionType::Strength);
    c.setCapSec = 120;
    f.start(c);
    f.run(0, 125000);
    ASSERT_EQ(f.cues.size(), 1u);                  // no warning before a cap
    EXPECT_EQ(f.cues[0], std::make_pair(120000u, Cue::Rest));
    EXPECT_TRUE(f.m.lapAt(0).autoEnded);
    // The rest is anchored to the cap, not to the tick.
    f.run(126000, 185000);
    EXPECT_EQ(f.cues.back(), std::make_pair(180000u, Cue::Work));
}

TEST(PlanRunner, EmomRunsItsMinutesAndIgnoresTheButton)
{
    for (uint32_t s0 : kStarts) {
        Fixture f;
        f.start(of(SessionType::Emom), s0);       // 10 minutes
        f.run(0, 30000);
        EXPECT_EQ(f.press(30500), Cue::None);
        EXPECT_EQ(f.m.lapCount(), 1);
        f.run(31000, 610000);
        EXPECT_EQ(f.count(Cue::Work), 9);
        EXPECT_EQ(f.count(Cue::Warning), 10);
        EXPECT_EQ(f.count(Cue::Done), 1);
        EXPECT_EQ(f.cues.back(), std::make_pair(600000u, Cue::Done));
        EXPECT_EQ(f.m.lapCount(), 10);
        for (uint8_t i = 0; i < 10; ++i) {
            EXPECT_EQ(f.m.lapAt(i).activeMs, 60000u) << int(i);
            EXPECT_EQ(f.m.lapAt(i).round, i + 1) << int(i);
        }
        EXPECT_EQ(f.m.state(), RunState::Paused);
        EXPECT_EQ(f.m.activeMs(s0 + 700000), 600000u);
    }
}

TEST(PlanRunner, LateTicksDoNotMakeThePlanDrift)
{
    Fixture f;
    f.start(of(SessionType::Emom));
    f.run(0, 700000, 1300);                       // a tick every 1.3 s
    ASSERT_TRUE(f.r.done());
    EXPECT_EQ(f.m.lapCount(), 10);
    // Each lap is within one tick of a minute, and the whole plan ends within a
    // tick of ten minutes: no lap's lateness carries into the next.
    uint32_t total = 0;
    for (uint8_t i = 0; i < 10; ++i) {
        EXPECT_NEAR(double(f.m.lapAt(i).activeMs), 60000.0, 1300.0) << int(i);
        total += f.m.lapAt(i).activeMs;
    }
    EXPECT_GE(total, 600000u);
    EXPECT_LT(total, 601300u);
}

TEST(PlanRunner, ALongGapEndsEveryStepItCrossed)
{
    Fixture f;
    f.start(of(SessionType::Emom));
    f.run(0, 0);
    f.m.addSample(185000, 120);
    EXPECT_EQ(f.r.tick(f.m, 185000), Cue::Work);
    EXPECT_EQ(f.m.lapCount(), 4);                 // minutes 1 to 3 ended, minute 4 begun
    const PlanView v = f.r.view(f.m, 185000);
    EXPECT_EQ(v.round, 4);
    EXPECT_EQ(v.stepMs, 5000u);                   // anchored at 3:00
    EXPECT_EQ(v.remainingMs, 55000u);
}

TEST(PlanRunner, AmrapCountsRoundsUntilTheCap)
{
    Fixture f;
    f.start(of(SessionType::Amrap));              // 12 minutes
    f.run(0, 100000);
    EXPECT_EQ(f.press(100000), Cue::Round);
    f.run(101000, 200000);
    EXPECT_EQ(f.press(200000), Cue::Round);
    f.run(201000, 300000);
    EXPECT_EQ(f.press(300000), Cue::Round);
    EXPECT_EQ(f.r.view(f.m, 300000).round, 4);    // the round now under way
    EXPECT_EQ(f.r.view(f.m, 300000).remainingMs, 420000u);
    f.run(301000, 730000);
    EXPECT_EQ(f.cues.back(), std::make_pair(720000u, Cue::Done));
    EXPECT_EQ(f.count(Cue::Warning), 1);
    EXPECT_EQ(f.r.amrapRounds(), 3);
    ASSERT_EQ(f.m.lapCount(), 4);                 // three rounds and the part round at the cap
    for (uint8_t i = 0; i < 4; ++i) {
        EXPECT_EQ(f.m.lapAt(i).round, i + 1) << int(i);
    }
    EXPECT_EQ(f.m.lapAt(3).activeMs, 420000u);
}

TEST(PlanRunner, TabataAlternatesAndEndsAtThreeFifty)
{
    for (uint32_t s0 : kStarts) {
        Fixture f;
        f.start(of(SessionType::Tabata), s0);
        f.run(0, 240000);
        EXPECT_EQ(f.count(Cue::Rest), 7);
        EXPECT_EQ(f.count(Cue::Work), 7);
        EXPECT_EQ(f.count(Cue::Warning), 15);
        EXPECT_EQ(f.cues.back(), std::make_pair(230000u, Cue::Done));
        ASSERT_EQ(f.m.lapCount(), 15);
        for (uint8_t i = 0; i < 15; ++i) {
            EXPECT_EQ(f.m.lapAt(i).kind, uint8_t(i % 2 ? StepKind::Rest : StepKind::Work)) << int(i);
            EXPECT_EQ(f.m.lapAt(i).activeMs, i % 2 ? 10000u : 20000u) << int(i);
        }
    }
}

TEST(PlanRunner, APauseStopsThePlanClock)
{
    Fixture f;
    f.start(of(SessionType::Custom));             // 40 s work, 20 s rest
    f.run(0, 30000);
    ASSERT_TRUE(f.m.pause(30000));
    EXPECT_EQ(f.r.tick(f.m, 130000), Cue::None);  // paused: nothing moves
    EXPECT_EQ(f.press(130000), Cue::None);
    ASSERT_TRUE(f.m.resume(130000));
    f.run(131000, 145000);
    // 40 s of active time: 10 s after the resume.
    EXPECT_EQ(f.cues.back(), std::make_pair(140000u, Cue::Rest));
    EXPECT_EQ(f.m.lapAt(0).activeMs, 40000u);
    EXPECT_EQ(f.m.lapAt(0).pausedMs, 100000u);
}

TEST(PlanRunner, CircuitMovesOnTheButtonOnly)
{
    Fixture f;
    PlanConfig c = of(SessionType::Circuit);
    c.circuitRounds = 2;
    c.runM = 500;
    f.start(c);
    f.run(0, 200000);
    EXPECT_TRUE(f.cues.empty());                  // no timed steps
    PlanView v = f.r.view(f.m, 200000);
    EXPECT_EQ(v.kind, uint8_t(StepKind::Run));
    EXPECT_EQ(v.distanceM, 500u);
    EXPECT_EQ(v.remainingMs, 0u);
    EXPECT_EQ(f.press(200000), Cue::Station);
    v = f.r.view(f.m, 200000);
    EXPECT_EQ(v.station, 1);
    EXPECT_EQ(v.distanceM, 1000u);                // the SkiErg
    EXPECT_EQ(f.press(400000), Cue::Run);
    EXPECT_EQ(f.press(550000), Cue::Station);
    EXPECT_EQ(f.r.view(f.m, 550000).station, 2);
    EXPECT_EQ(f.press(600000), Cue::Done);
    ASSERT_EQ(f.m.lapCount(), 4);
    EXPECT_EQ(f.m.lapAt(1).station, 1);
    EXPECT_EQ(f.m.lapAt(3).station, 2);
    EXPECT_EQ(f.m.lapAt(2).kind, uint8_t(StepKind::Run));
    EXPECT_EQ(f.m.lapAt(2).round, 2);
}

TEST(PlanRunner, APlanSwitchesTheAutoLapOff)
{
    Fixture f;
    f.start(of(SessionType::Strength), 0, 60000);
    EXPECT_EQ(f.m.config().autoLapMs, 0u);
    f.run(0, 100000);
    EXPECT_EQ(f.m.lapCount(), 1);
}

TEST(PlanRunner, StartingAgainBeginsAFreshPlan)
{
    Fixture f;
    f.start(of(SessionType::Tabata));
    f.run(0, 240000);
    ASSERT_TRUE(f.r.done());
    f.m.stop(240000);
    f.cues.clear();
    f.start(of(SessionType::Amrap), 300000);
    EXPECT_FALSE(f.r.done());
    EXPECT_EQ(f.r.amrapRounds(), 0);
    EXPECT_EQ(f.m.lapCount(), 1);
    EXPECT_EQ(f.r.view(f.m, 310000).stepMs, 10000u);
}
