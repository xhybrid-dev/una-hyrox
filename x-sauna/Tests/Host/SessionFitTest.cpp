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

TEST(SessionFit, ModeIsRecordedAsUsed)
{
    FitLap laps[kMaxLaps];
    FitSession session;
    SessionModel m;
    m.start(0, cfg(Mode::SaunaOnly, false));
    m.stop(1000);
    buildFit(m, laps, kMaxLaps, session);
    EXPECT_EQ(session.mode, static_cast<uint8_t>(Mode::SaunaOnly));

    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.stop(1000);
    buildFit(m, laps, kMaxLaps, session);
    EXPECT_EQ(session.mode, static_cast<uint8_t>(Mode::SaunaPlunge));
}

TEST(SessionFit, PhaseTotalsAndRoundsComeFromTheLapsWritten)
{
    SessionModel m;
    build(m, 0);
    FitLap laps[kMaxLaps];
    FitSession session;
    const uint8_t n = buildFit(m, laps, kMaxLaps, session);
    uint32_t sauna = 0, plunge = 0, rest = 0;
    for (uint8_t i = 0; i < n; ++i) {
        (laps[i].phase == static_cast<uint8_t>(Phase::Sauna)    ? sauna
         : laps[i].phase == static_cast<uint8_t>(Phase::Plunge) ? plunge
                                                                : rest) += laps[i].activeSec;
    }
    EXPECT_EQ(session.saunaSec, sauna);        // exactly the laps' whole seconds
    EXPECT_EQ(session.plungeSec, plunge);
    EXPECT_EQ(session.saunaSec + session.plungeSec + rest, session.timerSec);
    EXPECT_EQ(session.rounds, 2);              // two Sauna laps
    EXPECT_GT(session.plungeSec, 0u);
    EXPECT_EQ(session.saunaMinutes, detail::wholeMinutes(sauna));
    EXPECT_EQ(session.plungeSeconds, plunge);
}

TEST(SessionFit, JustSaunaHasNoPlungeTime)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaOnly, false));
    m.lap(600000);                             // sauna 10 min
    m.lap(900000);                             // rest 5 min
    m.stop(1500000);                           // sauna 10 min
    FitLap laps[kMaxLaps];
    FitSession session;
    buildFit(m, laps, kMaxLaps, session);
    EXPECT_EQ(session.rounds, 2);
    EXPECT_EQ(session.saunaSec, 1200u);
    EXPECT_EQ(session.saunaMinutes, 20u);
    EXPECT_EQ(session.plungeSec, 0u);
    EXPECT_EQ(session.plungeSeconds, 0u);
}

TEST(SessionFit, WholeMinutesRoundHalfUpAndSaturate)
{
    EXPECT_EQ(detail::wholeMinutes(0), 0u);
    EXPECT_EQ(detail::wholeMinutes(29), 0u);
    EXPECT_EQ(detail::wholeMinutes(30), 1u);        // half up
    EXPECT_EQ(detail::wholeMinutes(89), 1u);
    EXPECT_EQ(detail::wholeMinutes(90), 2u);
    EXPECT_EQ(detail::wholeMinutes(5628), 94u);     // Jon's sauna laps, 8 Nov: 93.8 min
    EXPECT_EQ(detail::wholeMinutes(UINT32_MAX), 0xFFFEu);
    EXPECT_EQ(detail::clampU16(185), 185u);         // and his plunges, in seconds
    EXPECT_EQ(detail::clampU16(0xFFFF), 0xFFFEu);   // never FIT's invalid value
}

TEST(SessionFit, LongPlungeSecondsSaturate)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.lap(60000);                              // sauna 1 min
    m.stop(60000 + 70000u * 1000u);            // a 70,000 s "plunge"
    FitLap laps[kMaxLaps];
    FitSession session;
    buildFit(m, laps, kMaxLaps, session);
    EXPECT_EQ(session.plungeSec, 70000u);
    EXPECT_EQ(session.plungeSeconds, 0xFFFEu);
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
