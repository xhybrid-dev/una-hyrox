#include <gtest/gtest.h>

#include "SessionFit.hpp"

using namespace Sauna;

namespace {

SessionConfig cfg(Mode mode, bool rest)
{
    SessionConfig c;
    c.mode = mode;
    c.rest = rest;
    return c;
}

void feed(SessionModel &m, uint32_t from, uint32_t to, uint16_t hr, int32_t temp = kX100Invalid)
{
    for (uint32_t off = 0; off <= to - from; off += 1000) {
        m.addSample(from + off, hr, temp);
    }
}

/// A messy session: awkward millisecond boundaries and a pause in the middle.
void build(SessionModel &m, uint32_t s)
{
    m.start(s, cfg(Mode::SaunaPlunge, true));
    feed(m, s + 1000, s + 30000, 120);
    m.lap(s + 30400);                          // sauna 30.4 s
    feed(m, s + 31000, s + 40000, 110);
    m.pause(s + 41700);
    m.resume(s + 52900);                       // 11.2 s paused in the plunge
    m.lap(s + 60300);                          // plunge: 29.9 s wall, 18.7 s active
    feed(m, s + 61000, s + 130000, 100);       // rest: its recovery resolves
    m.lap(s + 130900);
    m.stop(s + 140100);
}

} // namespace

TEST(SessionModelPause, PausedTimeIsKeptPerLap)
{
    SessionModel m;
    build(m, 0);
    EXPECT_EQ(m.lapAt(0).pausedMs, 0u);
    EXPECT_EQ(m.lapAt(1).pausedMs, 11200u);
    EXPECT_EQ(m.lapAt(1).activeMs, 60300u - 30400u - 11200u);
}

TEST(SessionModelPause, StoppingWhilePausedCountsThePauseInTheLap)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.pause(10000);
    m.stop(25000);
    EXPECT_EQ(m.lapAt(0).pausedMs, 15000u);
    EXPECT_EQ(m.lapAt(0).activeMs, 10000u);
}

TEST(SessionFit, LapsChainAndSumToTheSession)
{
    for (uint32_t s : {0u, 5000u, 0xFFFFFFFFu - 70000u}) {
        SessionModel m;
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

TEST(SessionFit, LapFieldsCarryPhaseRoundAndHeartRate)
{
    SessionModel m;
    build(m, 0);
    FitLap laps[kMaxLaps];
    FitSession session;
    buildFit(m, laps, kMaxLaps, session);
    EXPECT_EQ(laps[0].phase, static_cast<uint8_t>(Phase::Sauna));
    EXPECT_EQ(laps[1].phase, static_cast<uint8_t>(Phase::Plunge));
    EXPECT_EQ(laps[2].phase, static_cast<uint8_t>(Phase::Rest));
    EXPECT_EQ(laps[3].phase, static_cast<uint8_t>(Phase::Sauna));
    EXPECT_EQ(laps[3].round, 2);
    EXPECT_EQ(laps[0].hrMax, 120);
    EXPECT_EQ(laps[0].hrAvg, 120);
    EXPECT_EQ(laps[3].hrAvg, 0);               // the last lap had no samples
    EXPECT_EQ(session.hrMax, 120);
}

TEST(SessionFit, RecoveryOnlyWhereItResolved)
{
    SessionModel m;
    build(m, 0);
    FitLap laps[kMaxLaps];
    FitSession session;
    buildFit(m, laps, kMaxLaps, session);
    EXPECT_TRUE(laps[0].hasRecovery);          // sauna ended at 120, 100 a minute on
    EXPECT_EQ(laps[0].recoveryBpm, 20);
    EXPECT_FALSE(laps[3].hasRecovery);         // the last lap never ended
    EXPECT_EQ(laps[3].recoveryBpm, 0);
}

TEST(SessionFit, ModeAndRestAreRecordedAsUsed)
{
    FitLap laps[kMaxLaps];
    FitSession session;
    SessionModel m;
    m.start(0, cfg(Mode::SaunaOnly, false));
    m.stop(1000);
    buildFit(m, laps, kMaxLaps, session);
    EXPECT_EQ(session.mode, static_cast<uint8_t>(Mode::SaunaOnly));
    EXPECT_EQ(session.rest, 1);                // Just sauna always rests

    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.stop(1000);
    buildFit(m, laps, kMaxLaps, session);
    EXPECT_EQ(session.mode, static_cast<uint8_t>(Mode::SaunaPlunge));
    EXPECT_EQ(session.rest, 0);
}

TEST(SessionFit, CapLimitsTheLapsWritten)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
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
    SessionModel m;                            // never started
    FitLap laps[2];
    FitSession session;
    EXPECT_EQ(buildFit(m, laps, 2, session), 0);
    EXPECT_EQ(session.elapsedSec, 0u);
}
