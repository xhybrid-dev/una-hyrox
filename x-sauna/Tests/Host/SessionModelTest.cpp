#include <gtest/gtest.h>

#include "SessionModel.hpp"

using namespace Sauna;

namespace {

SessionConfig cfg(Mode mode, bool rest)
{
    SessionConfig c;
    c.mode = mode;
    c.rest = rest;
    return c;
}

/// One sample a second from `from` to `to` (ms), same HR and temperature.
void feed(SessionModel &m, uint32_t from, uint32_t to, uint16_t hr, int32_t temp = kX100Invalid)
{
    // Offsets, not `t <= to`: the loop must survive the clock wrapping.
    for (uint32_t off = 0; off <= to - from; off += 1000) {
        m.addSample(from + off, hr, temp);
    }
}

// Both a plain start and one just before the 32-bit clock wraps.
constexpr uint32_t kStarts[] = {0u, 123456u, 0xFFFFFFFFu - 90000u};

} // namespace

TEST(SessionModel, StartsRunningInSaunaRoundOne)
{
    SessionModel m;
    EXPECT_EQ(m.state(), RunState::Idle);
    m.start(1000, cfg(Mode::SaunaPlunge, false));
    EXPECT_EQ(m.state(), RunState::Running);
    EXPECT_EQ(m.lapCount(), 1);
    EXPECT_EQ(m.phase(), Phase::Sauna);
    EXPECT_EQ(m.round(), 1);
}

TEST(SessionModel, SaunaPlungeAlternatesAndCountsRounds)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    const Phase want[] = {Phase::Plunge, Phase::Sauna, Phase::Plunge, Phase::Sauna};
    const uint8_t rounds[] = {1, 2, 2, 3};
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(m.lap(static_cast<uint32_t>((i + 1) * 1000)), LapResult::Ok);
        EXPECT_EQ(m.phase(), want[i]) << i;
        EXPECT_EQ(m.round(), rounds[i]) << i;
    }
    EXPECT_EQ(m.lapCount(), 5);
}

TEST(SessionModel, SaunaPlungeWithRestInsertsRest)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, true));
    m.lap(1000);
    EXPECT_EQ(m.phase(), Phase::Plunge);
    m.lap(2000);
    EXPECT_EQ(m.phase(), Phase::Rest);
    EXPECT_EQ(m.round(), 1);
    m.lap(3000);
    EXPECT_EQ(m.phase(), Phase::Sauna);
    EXPECT_EQ(m.round(), 2);
}

TEST(SessionModel, JustSaunaAlternatesSaunaAndRestWhateverTheRestSetting)
{
    for (bool rest : {true, false}) {
        SessionModel m;
        m.start(0, cfg(Mode::SaunaOnly, rest));
        m.lap(1000);
        EXPECT_EQ(m.phase(), Phase::Rest) << rest;
        EXPECT_EQ(m.round(), 1) << rest;
        m.lap(2000);
        EXPECT_EQ(m.phase(), Phase::Sauna) << rest;
        EXPECT_EQ(m.round(), 2) << rest;
        m.lap(3000);
        EXPECT_EQ(m.phase(), Phase::Rest) << rest;
        EXPECT_EQ(m.lapCount(), 4) << rest;
    }
}

TEST(SessionModel, ActiveTimeExcludesPausesAndElapsedDoesNot)
{
    for (uint32_t s : kStarts) {
        SessionModel m;
        m.start(s, cfg(Mode::SaunaPlunge, false));
        m.addSample(s + 10000, 0, kX100Invalid);
        EXPECT_TRUE(m.pause(s + 20000));
        EXPECT_EQ(m.activeMs(s + 50000), 20000u);       // frozen while paused
        EXPECT_EQ(m.elapsedMs(s + 50000), 50000u);
        EXPECT_TRUE(m.resume(s + 50000));
        EXPECT_EQ(m.activeMs(s + 60000), 30000u);
        EXPECT_EQ(m.lapActiveMs(s + 60000), 30000u);
        EXPECT_EQ(m.elapsedMs(s + 60000), 60000u);
    }
}

TEST(SessionModel, LapActiveTimeIsPerLap)
{
    for (uint32_t s : kStarts) {
        SessionModel m;
        m.start(s, cfg(Mode::SaunaPlunge, false));
        m.lap(s + 15000);
        m.lap(s + 20000);
        EXPECT_EQ(m.lapAt(0).activeMs, 15000u);
        EXPECT_EQ(m.lapAt(1).activeMs, 5000u);
        EXPECT_EQ(m.lapActiveMs(s + 26000), 6000u);
        EXPECT_EQ(m.activeMs(s + 26000), 26000u);
    }
}

TEST(SessionModel, PauseStateRefusesLapAndDoubleCalls)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    EXPECT_FALSE(m.resume(100));
    EXPECT_TRUE(m.pause(1000));
    EXPECT_FALSE(m.pause(2000));
    EXPECT_EQ(m.lap(3000), LapResult::NotRunning);
    EXPECT_EQ(m.lapCount(), 1);
}

TEST(SessionModel, LapBeforeStartAndAfterStopIsRefused)
{
    SessionModel m;
    EXPECT_EQ(m.lap(0), LapResult::NotRunning);
    EXPECT_EQ(m.lapCount(), 0);
    m.start(0, cfg(Mode::SaunaPlunge, false));
    EXPECT_TRUE(m.stop(5000));
    EXPECT_EQ(m.state(), RunState::Stopped);
    EXPECT_EQ(m.lap(6000), LapResult::NotRunning);
    EXPECT_FALSE(m.stop(7000));
}

TEST(SessionModel, ElapsedFreezesAtStop)
{
    for (uint32_t s : kStarts) {
        SessionModel m;
        m.start(s, cfg(Mode::SaunaPlunge, false));
        m.stop(s + 90000);
        EXPECT_EQ(m.elapsedMs(s + 500000), 90000u);
        EXPECT_EQ(m.activeMs(s + 500000), 90000u);
    }
}

TEST(SessionModel, LapCapRefusesThe65thAndChangesNothing)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    uint32_t t = 0;
    for (int i = 1; i < kMaxLaps; ++i) {
        t += 1000;
        ASSERT_EQ(m.lap(t), LapResult::Ok) << i;
    }
    EXPECT_EQ(m.lapCount(), kMaxLaps);
    const Phase p = m.phase();
    const uint8_t r = m.round();
    EXPECT_EQ(m.lap(t + 1000), LapResult::Full);
    EXPECT_EQ(m.lapCount(), kMaxLaps);
    EXPECT_EQ(m.phase(), p);
    EXPECT_EQ(m.round(), r);
    // The session carries on being measured.
    m.addSample(t + 2000, 100, 2000);
    EXPECT_EQ(m.current().hrCount, 1u);
}

TEST(SessionModel, HeartRateAverageMaxAndGaps)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.addSample(1000, 100, kX100Invalid);
    m.addSample(2000, 0, kX100Invalid);        // no reading: not a zero in the mean
    m.addSample(3000, 111, kX100Invalid);
    const LapStats &l = m.current();
    EXPECT_EQ(l.hrCount, 2u);
    EXPECT_EQ(l.hrAvg(), 106u);                // (100 + 111) / 2 = 105.5, rounded up
    EXPECT_EQ(l.hrMax, 111);
}

TEST(SessionModel, NoHeartRateMeansZeroNotGarbage)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.addSample(1000, 0, 2000);
    EXPECT_EQ(m.current().hrAvg(), 0);
    EXPECT_EQ(m.current().hrMax, 0);
    EXPECT_EQ(m.summary().hrAvg, 0);
}

TEST(SessionModel, TemperatureStatsPerLap)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.addSample(1000, 0, 3000);
    m.addSample(2000, 0, 3400);
    m.addSample(3000, 0, 3200);
    m.addSample(4000, 0, kX100Invalid);        // ignored
    const LapStats &l = m.current();
    EXPECT_EQ(l.tempMinX100, 3000);
    EXPECT_EQ(l.tempMaxX100, 3400);
    EXPECT_EQ(l.tempAvgX100(), 3200);
    EXPECT_EQ(l.tempCount, 3u);

    m.lap(5000);
    EXPECT_EQ(m.current().tempAvgX100(), kX100Invalid);
}

TEST(SessionModel, NegativeTemperatureAveragesCorrectly)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.addSample(1000, 0, -300);
    m.addSample(2000, 0, -400);
    EXPECT_EQ(m.current().tempMinX100, -400);
    EXPECT_EQ(m.current().tempMaxX100, -300);
    EXPECT_EQ(m.current().tempAvgX100(), -350);
}

TEST(SessionModel, SamplesWhilePausedDoNotCountInTheLap)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.addSample(1000, 90, 2000);
    m.pause(2000);
    m.addSample(3000, 150, 9000);
    EXPECT_EQ(m.current().hrCount, 1u);
    EXPECT_EQ(m.current().hrMax, 90);
    EXPECT_EQ(m.current().tempMaxX100, 2000);
}

TEST(SessionModel, SamplesBeforeStartAndAfterStopAreIgnored)
{
    SessionModel m;
    m.addSample(1000, 90, 2000);
    EXPECT_EQ(m.lapCount(), 0);
    m.start(2000, cfg(Mode::SaunaPlunge, false));
    m.stop(3000);
    m.addSample(4000, 90, 2000);
    EXPECT_EQ(m.current().hrCount, 0u);
}

TEST(SessionModel, RecoveryIsFedFromEachLapsEnd)
{
    for (uint32_t s : kStarts) {
        SessionModel m;
        m.start(s, cfg(Mode::SaunaPlunge, false));
        feed(m, s + 1000, s + 600000 / 10, 150);             // sauna, HR 150
        const uint32_t end = s + 60000;
        m.lap(end);                                          // 150 at the end
        EXPECT_EQ(m.lapAt(0).recovery.state, Recovery::State::Pending);
        feed(m, end + 1000, end + 59000, 130);               // still under a minute
        EXPECT_EQ(m.lapAt(0).recovery.state, Recovery::State::Pending);
        m.addSample(end + 60000, 120, kX100Invalid);
        EXPECT_EQ(m.lapAt(0).recovery.state, Recovery::State::Done);
        EXPECT_EQ(m.lapAt(0).recovery.drop, 30);
    }
}

TEST(SessionModel, ShortPhasesHaveOverlappingRecoveries)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    feed(m, 1000, 30000, 140);
    m.lap(30000);                                  // sauna ends at 140
    feed(m, 31000, 50000, 125);
    m.lap(50000);                                  // plunge ends at 125
    EXPECT_EQ(m.lapAt(0).recovery.state, Recovery::State::Pending);
    EXPECT_EQ(m.lapAt(1).recovery.state, Recovery::State::Pending);
    feed(m, 51000, 90000, 110);                    // 90000 = 60 s after the first end
    EXPECT_EQ(m.lapAt(0).recovery.state, Recovery::State::Done);
    EXPECT_EQ(m.lapAt(0).recovery.drop, 30);
    EXPECT_EQ(m.lapAt(1).recovery.state, Recovery::State::Pending);
    feed(m, 91000, 110000, 100);
    EXPECT_EQ(m.lapAt(1).recovery.state, Recovery::State::Done);
    EXPECT_EQ(m.lapAt(1).recovery.drop, 25);               // 125 at the end, 100 a minute on
}

TEST(SessionModel, LapWithAStaleHeartRateHasNoResult)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.addSample(1000, 140, kX100Invalid);
    m.lap(20000);                                  // the last HR is 19 s old
    EXPECT_EQ(m.lapAt(0).recovery.state, Recovery::State::NoResult);
}

TEST(SessionModel, StopGivesUpOnPendingRecoveries)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    feed(m, 1000, 30000, 140);
    m.lap(30000);
    m.stop(40000);
    EXPECT_EQ(m.lapAt(0).recovery.state, Recovery::State::NoResult);
}

TEST(SessionModel, RecoveryKeepsResolvingWhilePaused)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    feed(m, 1000, 30000, 140);
    m.lap(30000);
    m.pause(31000);
    m.addSample(90000, 118, kX100Invalid);
    EXPECT_EQ(m.lapAt(0).recovery.state, Recovery::State::Done);
    EXPECT_EQ(m.lapAt(0).recovery.drop, 22);
}

TEST(SessionModel, SummaryTotalsPhasesAndBestRecovery)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, true));
    feed(m, 1000, 30000, 140);
    m.lap(30000);                                  // plunge
    feed(m, 31000, 40000, 120);
    m.lap(40000);                                  // rest
    feed(m, 41000, 100000, 100);                   // resolves both recoveries
    m.lap(100000);                                 // sauna 2
    feed(m, 101000, 110000, 135);
    m.stop(110000);

    const SessionSummary s = m.summary();
    EXPECT_EQ(s.laps, 4);
    EXPECT_EQ(s.rounds, 2);
    EXPECT_EQ(s.saunaMs, 30000u + 10000u);
    EXPECT_EQ(s.plungeMs, 10000u);
    EXPECT_EQ(s.restMs, 60000u);
    EXPECT_EQ(s.hrMax, 140);
    EXPECT_TRUE(s.hasRecovery);
    EXPECT_EQ(s.bestDrop, 40);                     // 140 -> 100
}

TEST(SessionModel, SummaryWithNoResolvedRecovery)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.stop(1000);
    EXPECT_FALSE(m.summary().hasRecovery);
}

TEST(SessionModel, RestartClearsTheLastSession)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    feed(m, 1000, 5000, 130, 3000);
    m.lap(6000);
    m.stop(7000);
    m.start(100000, cfg(Mode::SaunaOnly, false));
    EXPECT_EQ(m.state(), RunState::Running);
    EXPECT_EQ(m.lapCount(), 1);
    EXPECT_EQ(m.current().hrCount, 0u);
    EXPECT_EQ(m.summary().hrMax, 0);
    EXPECT_EQ(m.activeMs(100000), 0u);
    EXPECT_EQ(m.config().mode, Mode::SaunaOnly);
}

TEST(SessionModel, LapAtOutOfRangeGivesTheLastLap)
{
    SessionModel m;
    m.start(0, cfg(Mode::SaunaPlunge, false));
    m.lap(1000);
    EXPECT_EQ(m.lapAt(200).phase, Phase::Plunge);
}

TEST(SessionModel, PhaseNames)
{
    EXPECT_STREQ(phaseName(Phase::Sauna), "SAUNA");
    EXPECT_STREQ(phaseName(Phase::Plunge), "PLUNGE");
    EXPECT_STREQ(phaseName(Phase::Rest), "REST");
}
