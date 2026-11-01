#include <gtest/gtest.h>

#include "WorkoutFit.hpp"
#include "PlanRunner.hpp"
#include "ActivityLink.hpp"

#include <cstring>

using namespace Workout;

namespace {

WorkoutConfig cfg(uint32_t autoLapMs = 0)
{
    WorkoutConfig c;
    c.weightG   = 80000;
    c.autoLapMs = autoLapMs;
    return c;
}

void feed(WorkoutModel &m, uint32_t from, uint32_t to, uint16_t hr)
{
    for (uint32_t off = 0; off <= to - from; off += 1000) {
        m.addSample(from + off, hr);
    }
}

/// A messy session: awkward millisecond boundaries and a pause in the middle.
void build(WorkoutModel &m, uint32_t s)
{
    m.start(s, cfg());
    feed(m, s + 1000, s + 30000, 120);
    m.lap(s + 30400);                          // lap 1: 30.4 s
    feed(m, s + 31000, s + 40000, 110);
    m.pause(s + 41700);
    m.resume(s + 52900);                       // 11.2 s paused in lap 2
    m.lap(s + 60300);                          // lap 2: 29.9 s wall, 18.7 s active
    feed(m, s + 61000, s + 130000, 100);
    m.lap(s + 130900);
    m.stop(s + 140100);
}

} // namespace

TEST(SessionModelPause, PausedTimeIsKeptPerLap)
{
    WorkoutModel m;
    build(m, 0);
    EXPECT_EQ(m.lapAt(0).pausedMs, 0u);
    EXPECT_EQ(m.lapAt(1).pausedMs, 11200u);
    EXPECT_EQ(m.lapAt(1).activeMs, 60300u - 30400u - 11200u);
}

TEST(SessionModelPause, StoppingWhilePausedCountsThePauseInTheLap)
{
    WorkoutModel m;
    m.start(0, cfg());
    m.pause(10000);
    m.stop(25000);
    EXPECT_EQ(m.lapAt(0).pausedMs, 15000u);
    EXPECT_EQ(m.lapAt(0).activeMs, 10000u);
}

TEST(SessionFit, LapsChainAndSumToTheSession)
{
    for (uint32_t s : {0u, 5000u, 0xFFFFFFFFu - 70000u}) {
        WorkoutModel m;
        build(m, s);
        FitLap laps[kMaxLaps];
        FitSession session;
        const uint8_t n = buildFit(m, laps, kMaxLaps, session);
        ASSERT_EQ(n, 4);
        uint32_t cursor = 0, active = 0;
        for (uint8_t i = 0; i < n; ++i) {
            EXPECT_EQ(laps[i].startSec, cursor) << int(i);
            cursor += laps[i].elapsedSec;
            active += laps[i].activeSec;
        }
        EXPECT_EQ(session.elapsedSec, cursor);
        EXPECT_EQ(session.timerSec, active);
        EXPECT_EQ(session.elapsedSec, m.elapsedMs(s + 999999) / 1000u);   // 140.1 s -> 140
        EXPECT_LE(session.timerSec, session.elapsedSec);
        EXPECT_GT(session.elapsedSec - session.timerSec, 10u);            // the pause shows
    }
}

TEST(SessionFit, LapFieldsCarryHeartRateAndCalories)
{
    WorkoutModel m;
    build(m, 0);
    FitLap laps[kMaxLaps];
    FitSession session;
    buildFit(m, laps, kMaxLaps, session);
    EXPECT_EQ(laps[0].hrMax, 120);
    EXPECT_EQ(laps[0].hrAvg, 120);
    EXPECT_EQ(laps[3].hrAvg, 0);               // the last lap had no samples
    EXPECT_EQ(session.hrMax, 120);
    // Lap 1, 80 kg: 1 s with no zone yet, then 120 bpm is zone 2 (4.5 MET) for
    // 29.4 s: (1 x 1.0 + 29.4 x 4.5) x 80 / 3600 = 2.96 kcal -> 3.
    EXPECT_EQ(laps[0].kcal, 3);
    EXPECT_EQ(laps[0].restingKcal, 1);         // 30.4 x 80 / 3600 = 0.68
    EXPECT_FALSE(laps[0].autoEnded);
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        sum += laps[i].kcal;
    }
    EXPECT_NEAR(static_cast<double>(session.kcal), static_cast<double>(sum), 2.0);   // rounded per lap
    EXPECT_GT(session.restingKcal, 0);
    EXPECT_LT(session.restingKcal, session.kcal);
}

TEST(SessionFit, AutoLapsAreMarked)
{
    WorkoutModel m;
    m.start(0, cfg(10000));
    feed(m, 0, 25000, 100);
    m.stop(25000);
    FitLap laps[kMaxLaps];
    FitSession session;
    ASSERT_EQ(buildFit(m, laps, kMaxLaps, session), 3);
    EXPECT_TRUE(laps[0].autoEnded);
    EXPECT_TRUE(laps[1].autoEnded);
    EXPECT_FALSE(laps[2].autoEnded);
}

TEST(SessionFit, CaloriesAreWholeAndCapped)
{
    EXPECT_EQ(detail::kcalOf(0), 0);
    EXPECT_EQ(detail::kcalOf(14), 1);
    EXPECT_EQ(detail::kcalOf(15), 2);
    EXPECT_EQ(detail::kcalOf(UINT32_MAX), 65534);   // never FIT's invalid 65535
}

TEST(SessionFit, CapLimitsTheLapsWritten)
{
    WorkoutModel m;
    m.start(0, cfg());
    for (uint32_t t = 1; t <= 5; ++t) {
        m.lap(t * 1000);
    }
    FitLap laps[3];
    FitSession session;
    EXPECT_EQ(buildFit(m, laps, 3, session), 3);
    EXPECT_EQ(session.lapCount, 3);
}

TEST(SessionFit, EmptySessionHasNoLaps)
{
    WorkoutModel m;                            // never started
    FitLap laps[2];
    FitSession session;
    EXPECT_EQ(buildFit(m, laps, 2, session), 0);
    EXPECT_EQ(session.elapsedSec, 0u);
}

TEST(SessionFit, EachKindOfWorkoutHasItsSport)
{
    // The public FIT profile's numbers (fitdecode 0.11.0; WorkoutFit.hpp).
    struct Want { SessionType t; uint8_t sport, sub; const char *name; };
    const Want want[] = {
        {SessionType::Plain,    0,  0,  "Workout"},
        {SessionType::Strength, 10, 20, "Strength"},
        {SessionType::Emom,     62, 74, "EMOM"},
        {SessionType::Amrap,    62, 73, "AMRAP"},
        {SessionType::Tabata,   62, 75, "Tabata"},
        {SessionType::Custom,   62, 70, "Intervals"},
        {SessionType::Circuit,  10, 26, "Circuit"},
    };
    for (const Want &w : want) {
        const FitSport s = fitSportOf(w.t);
        EXPECT_EQ(s.sport, w.sport) << w.name;
        EXPECT_EQ(s.subSport, w.sub) << w.name;
        EXPECT_STREQ(s.profileName, w.name);
        EXPECT_LE(std::strlen(s.profileName), 15u) << w.name;   // ActivityWriter::kProfileNameMax
    }
    EXPECT_TRUE(writesSets(SessionType::Strength));
    for (SessionType t : {SessionType::Plain, SessionType::Emom, SessionType::Amrap, SessionType::Tabata,
                          SessionType::Custom, SessionType::Circuit}) {
        EXPECT_FALSE(writesSets(t)) << int(t);
    }
}

TEST(SessionFit, TheLinkSportCodesAreTheLinksOwn)
{
    EXPECT_EQ(linkSportCodeOf(SessionType::Strength), uint8_t(HybridXLink::Sport::Strength));
    EXPECT_EQ(linkSportCodeOf(SessionType::Circuit), uint8_t(HybridXLink::Sport::Hybrid));
    for (SessionType t : {SessionType::Plain, SessionType::Emom, SessionType::Amrap, SessionType::Tabata,
                          SessionType::Custom}) {
        EXPECT_EQ(linkSportCodeOf(t), uint8_t(HybridXLink::Sport::Workout)) << int(t);
    }
}

TEST(SessionFit, LapsCarryTheirStepKind)
{
    WorkoutModel m;
    PlanRunner r;
    PlanConfig plan;
    plan.type = SessionType::Strength;
    plan.sets = 2;
    r.start(plan, m, 0, cfg());
    m.addSample(30000, 120);
    r.press(m, 30000);        // set 1 done: rest
    m.addSample(40000, 120);
    r.press(m, 40000);        // rest skipped: set 2
    m.addSample(70000, 120);
    r.press(m, 70000);        // set 2 done: the end
    m.stop(80000);
    FitLap laps[kMaxLaps];
    FitSession session;
    ASSERT_EQ(buildFit(m, laps, kMaxLaps, session), 3);
    EXPECT_EQ(laps[0].kind, uint8_t(StepKind::Work));
    EXPECT_EQ(laps[1].kind, uint8_t(StepKind::Rest));
    EXPECT_EQ(laps[2].kind, uint8_t(StepKind::Work));
    EXPECT_EQ(laps[1].activeSec, 10u);
    // The plan paused the model at its end; the stop's 10 s is the last lap's pause.
    EXPECT_EQ(laps[2].activeSec, 30u);
    EXPECT_EQ(laps[2].elapsedSec, 40u);
}
