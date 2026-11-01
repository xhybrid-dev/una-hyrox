// HybridX Workout: the kinds of workout and their steps (SessionPlan, W1).

#include <gtest/gtest.h>

#include <cstring>

#include "SessionPlan.hpp"

// Race's own station table, to check the circuit's copy against it. Test only:
// the apps stay independent at build time.
#include "../../../hybridx-race/Software/Libs/Header/RaceData.hpp"

using namespace Workout;

namespace {

PlanConfig of(SessionType t)
{
    PlanConfig c;
    c.type = t;
    return c;
}

} // namespace

TEST(SessionPlan, PlainHasNoSteps)
{
    EXPECT_EQ(stepCount(of(SessionType::Plain)), 0);
    EXPECT_EQ(plannedMs(of(SessionType::Plain)), 0u);
}

TEST(SessionPlan, StrengthIsSetsWithRestsBetween)
{
    const PlanConfig c = of(SessionType::Strength);   // 3 sets, 60 s rest, 5 min cap
    ASSERT_EQ(stepCount(c), 5);
    const StepKind kinds[] = {StepKind::Work, StepKind::Rest, StepKind::Work, StepKind::Rest, StepKind::Work};
    const uint8_t rounds[] = {1, 1, 2, 2, 3};
    for (uint8_t i = 0; i < 5; ++i) {
        const Step s = stepAt(c, i);
        EXPECT_EQ(s.kind, kinds[i]) << int(i);
        EXPECT_EQ(s.round, rounds[i]) << int(i);
        EXPECT_EQ(s.rounds, 3);
        if (s.kind == StepKind::Work) {
            EXPECT_FALSE(s.timed());
            EXPECT_EQ(s.capMs, 300000u);
            EXPECT_EQ(s.endsAfterMs(), 300000u);
        } else {
            EXPECT_TRUE(s.timed());
            EXPECT_EQ(s.durationMs, 60000u);
        }
    }
    EXPECT_EQ(plannedMs(c), 0u);   // open sets: no planned length
}

TEST(SessionPlan, EmomIsTimedMinutes)
{
    PlanConfig c = of(SessionType::Emom);
    ASSERT_EQ(stepCount(c), 10);
    for (uint8_t i = 0; i < 10; ++i) {
        const Step s = stepAt(c, i);
        EXPECT_EQ(s.kind, StepKind::Work);
        EXPECT_EQ(s.durationMs, 60000u);
        EXPECT_EQ(s.round, i + 1);
        EXPECT_EQ(s.rounds, 10);
    }
    EXPECT_EQ(plannedMs(c), 600000u);
    c.emomMin = 60;
    EXPECT_EQ(stepCount(c), 60);
    EXPECT_EQ(plannedMs(c), 3600000u);
}

TEST(SessionPlan, AmrapIsOneTimedCap)
{
    const PlanConfig c = of(SessionType::Amrap);
    ASSERT_EQ(stepCount(c), 1);
    EXPECT_EQ(stepAt(c, 0).durationMs, 12u * 60000u);
    EXPECT_EQ(plannedMs(c), 720000u);
}

TEST(SessionPlan, TabataIsEightTwentyTensWithNoLastRest)
{
    const PlanConfig c = of(SessionType::Tabata);
    ASSERT_EQ(stepCount(c), 15);
    for (uint8_t i = 0; i < 15; ++i) {
        const Step s = stepAt(c, i);
        EXPECT_EQ(s.kind, i % 2 ? StepKind::Rest : StepKind::Work) << int(i);
        EXPECT_EQ(s.durationMs, i % 2 ? 10000u : 20000u) << int(i);
        EXPECT_EQ(s.round, i / 2 + 1) << int(i);
        EXPECT_EQ(s.rounds, 8);
    }
    EXPECT_EQ(plannedMs(c), 8u * 20000u + 7u * 10000u);   // 3:50
}

TEST(SessionPlan, CustomUsesItsOwnWorkRestAndRounds)
{
    PlanConfig c = of(SessionType::Custom);   // 40 / 20 x 6
    ASSERT_EQ(stepCount(c), 11);
    EXPECT_EQ(stepAt(c, 0).durationMs, 40000u);
    EXPECT_EQ(stepAt(c, 1).durationMs, 20000u);
    EXPECT_EQ(stepAt(c, 10).kind, StepKind::Work);
    EXPECT_EQ(stepAt(c, 10).round, 6);
    EXPECT_EQ(plannedMs(c), 6u * 40000u + 5u * 20000u);
    c.rounds = 1;
    EXPECT_EQ(stepCount(c), 1);
}

TEST(SessionPlan, CircuitIsRunThenStationInRaceOrder)
{
    PlanConfig c = of(SessionType::Circuit);   // 8 rounds, 1 km runs
    ASSERT_EQ(stepCount(c), 16);
    for (uint8_t r = 1; r <= 8; ++r) {
        const Step run = stepAt(c, static_cast<uint8_t>(2 * (r - 1)));
        const Step st  = stepAt(c, static_cast<uint8_t>(2 * (r - 1) + 1));
        EXPECT_EQ(run.kind, StepKind::Run);
        EXPECT_EQ(run.distanceM, 1000u);
        EXPECT_EQ(run.round, r);
        EXPECT_FALSE(run.timed());
        EXPECT_EQ(run.endsAfterMs(), 0u);
        EXPECT_EQ(st.kind, StepKind::Station);
        EXPECT_EQ(st.station, r);
        EXPECT_EQ(st.distanceM, circuitStation(r).distanceM);
    }
    c.circuitRounds = 2;
    c.runM = 500;
    EXPECT_EQ(stepCount(c), 4);
    EXPECT_EQ(stepAt(c, 2).distanceM, 500u);
    EXPECT_EQ(stepAt(c, 3).station, 2);
}

TEST(SessionPlan, TheCircuitStationsMatchRaces)
{
    ASSERT_EQ(kCircuitStationCount, Race::kStationCount);
    for (uint8_t i = 0; i < kCircuitStationCount; ++i) {
        EXPECT_STREQ(kCircuitStations[i].name, Race::kStations[i].name) << int(i);
        EXPECT_STREQ(kCircuitStations[i].work, Race::kStations[i].work) << int(i);
        EXPECT_STREQ(kCircuitStations[i].brief, Race::kStations[i].brief) << int(i);
        EXPECT_EQ(kCircuitStations[i].distanceM, Race::kStations[i].distanceM) << int(i);
    }
    EXPECT_STREQ(circuitStation(0).name, "SKIERG");    // out of range: the first
    EXPECT_STREQ(circuitStation(9).name, "SKIERG");
}

TEST(SessionPlan, ClampPullsEverythingIntoRange)
{
    PlanConfig c;
    c.type          = static_cast<SessionType>(42);
    c.sets          = 0;
    c.setRestSec    = 5000;
    c.setCapSec     = 0;
    c.emomMin       = 200;
    c.amrapMin      = 0;
    c.workSec       = 0;
    c.restSec       = 0;
    c.rounds        = 255;
    c.circuitRounds = 0;
    c.runM          = 950;
    const PlanConfig k = clamp(c);
    EXPECT_EQ(k.type, SessionType::Plain);
    EXPECT_EQ(k.sets, kSetsMin);
    EXPECT_EQ(k.setRestSec, kSetRestSecMax);
    EXPECT_EQ(k.setCapSec, kSetCapSecMin);
    EXPECT_EQ(k.emomMin, kEmomMinMax);
    EXPECT_EQ(k.amrapMin, kAmrapMinMin);
    EXPECT_EQ(k.workSec, kWorkSecMin);
    EXPECT_EQ(k.restSec, kRestSecMin);
    EXPECT_EQ(k.rounds, kRoundsMax);
    EXPECT_EQ(k.circuitRounds, kCircuitRoundsMin);
    EXPECT_EQ(k.runM, 900u);   // down to a 100 m step
}

TEST(SessionPlan, NoPlanHasMoreStepsThanTheModelHasLaps)
{
    for (uint8_t t = 0; t < kSessionTypeCount; ++t) {
        PlanConfig c;
        c.type = static_cast<SessionType>(t);
        c.sets = kSetsMax;
        c.emomMin = kEmomMinMax;
        c.rounds = kRoundsMax;
        c.circuitRounds = kCircuitRoundsMax;
        EXPECT_LE(stepCount(c), kMaxLaps) << int(t);
    }
}

TEST(SessionPlan, OutOfRangeStepIsAPlainOpenWork)
{
    const PlanConfig c = of(SessionType::Tabata);
    const Step s = stepAt(c, 15);
    EXPECT_EQ(s.kind, StepKind::Work);
    EXPECT_EQ(s.durationMs, 0u);
    EXPECT_EQ(s.round, 0);
}
