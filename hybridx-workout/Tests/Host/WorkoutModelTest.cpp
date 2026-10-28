#include <gtest/gtest.h>

#include "WorkoutModel.hpp"

using namespace Workout;

namespace {

WorkoutConfig cfg(uint32_t autoLapMs = 0, uint32_t weightG = 80000)
{
    WorkoutConfig c;
    c.autoLapMs = autoLapMs;
    c.weightG   = weightG;
    return c;
}

/// One sample a second from `from` to `to` (ms), same HR. Returns the auto laps taken.
int feed(WorkoutModel &m, uint32_t from, uint32_t to, uint16_t hr)
{
    int laps = 0;
    // Offsets, not `t <= to`: the loop must survive the clock wrapping.
    for (uint32_t off = 0; off <= to - from; off += 1000) {
        laps += m.addSample(from + off, hr) ? 1 : 0;
    }
    return laps;
}

// Both a plain start and one just before the 32-bit clock wraps.
constexpr uint32_t kStarts[] = {0u, 123456u, 0xFFFFFFFFu - 90000u};

} // namespace

TEST(WorkoutModel, StartsRunningInLapOne)
{
    WorkoutModel m;
    EXPECT_EQ(m.state(), RunState::Idle);
    m.start(1000, cfg());
    EXPECT_EQ(m.state(), RunState::Running);
    EXPECT_EQ(m.lapCount(), 1);
    EXPECT_EQ(m.zone(), 0);
}

TEST(WorkoutModel, ActiveTimeExcludesPausesAndElapsedDoesNot)
{
    for (uint32_t s : kStarts) {
        WorkoutModel m;
        m.start(s, cfg());
        m.addSample(s + 10000, 0);
        EXPECT_TRUE(m.pause(s + 20000));
        EXPECT_EQ(m.activeMs(s + 50000), 20000u);       // frozen while paused
        EXPECT_EQ(m.elapsedMs(s + 50000), 50000u);
        EXPECT_TRUE(m.resume(s + 50000));
        EXPECT_EQ(m.activeMs(s + 60000), 30000u);
        EXPECT_EQ(m.lapActiveMs(s + 60000), 30000u);
        EXPECT_EQ(m.elapsedMs(s + 60000), 60000u);
    }
}

TEST(WorkoutModel, LapActiveTimeIsPerLap)
{
    for (uint32_t s : kStarts) {
        WorkoutModel m;
        m.start(s, cfg());
        EXPECT_EQ(m.lap(s + 15000), LapResult::Ok);
        EXPECT_EQ(m.lap(s + 20000), LapResult::Ok);
        EXPECT_EQ(m.lapAt(0).activeMs, 15000u);
        EXPECT_EQ(m.lapAt(1).activeMs, 5000u);
        EXPECT_FALSE(m.lapAt(0).autoEnded);
        EXPECT_EQ(m.lapActiveMs(s + 26000), 6000u);
        EXPECT_EQ(m.activeMs(s + 26000), 26000u);
    }
}

TEST(WorkoutModel, PauseStateRefusesLapAndDoubleCalls)
{
    WorkoutModel m;
    m.start(0, cfg());
    EXPECT_FALSE(m.resume(100));
    EXPECT_TRUE(m.pause(1000));
    EXPECT_FALSE(m.pause(2000));
    EXPECT_EQ(m.lap(3000), LapResult::NotRunning);
    EXPECT_EQ(m.lapCount(), 1);
}

TEST(WorkoutModel, LapBeforeStartAndAfterStopIsRefused)
{
    WorkoutModel m;
    EXPECT_EQ(m.lap(0), LapResult::NotRunning);
    EXPECT_EQ(m.lapCount(), 0);
    m.start(0, cfg());
    EXPECT_TRUE(m.stop(5000));
    EXPECT_EQ(m.state(), RunState::Stopped);
    EXPECT_EQ(m.lap(6000), LapResult::NotRunning);
    EXPECT_FALSE(m.stop(7000));
}

TEST(WorkoutModel, ElapsedFreezesAtStop)
{
    for (uint32_t s : kStarts) {
        WorkoutModel m;
        m.start(s, cfg());
        m.stop(s + 90000);
        EXPECT_EQ(m.elapsedMs(s + 500000), 90000u);
        EXPECT_EQ(m.activeMs(s + 500000), 90000u);
    }
}

TEST(WorkoutModel, LapCapRefusesThe65thAndChangesNothing)
{
    WorkoutModel m;
    m.start(0, cfg());
    uint32_t t = 0;
    for (int i = 1; i < kMaxLaps; ++i) {
        t += 1000;
        ASSERT_EQ(m.lap(t), LapResult::Ok) << i;
    }
    EXPECT_EQ(m.lapCount(), kMaxLaps);
    EXPECT_EQ(m.lap(t + 1000), LapResult::Full);
    EXPECT_EQ(m.lapCount(), kMaxLaps);
    // The workout carries on being measured.
    m.addSample(t + 2000, 100);
    EXPECT_EQ(m.current().hrCount, 1u);
}

TEST(WorkoutModel, HeartRateAverageMaxAndGaps)
{
    WorkoutModel m;
    m.start(0, cfg());
    m.addSample(1000, 100);
    m.addSample(2000, 0);        // no reading: not a zero in the mean
    m.addSample(3000, 111);
    const LapStats &l = m.current();
    EXPECT_EQ(l.hrCount, 2u);
    EXPECT_EQ(l.hrAvg(), 106u);  // (100 + 111) / 2 = 105.5, rounded up
    EXPECT_EQ(l.hrMax, 111);
    EXPECT_EQ(m.summary().hrAvg, 106u);
    EXPECT_EQ(m.summary().hrMax, 111u);
}

TEST(WorkoutModel, NoHeartRateMeansZeroNotGarbage)
{
    WorkoutModel m;
    m.start(0, cfg());
    m.addSample(1000, 0);
    EXPECT_EQ(m.current().hrAvg(), 0);
    EXPECT_EQ(m.current().hrMax, 0);
    EXPECT_EQ(m.summary().hrAvg, 0);
}

TEST(WorkoutModel, SamplesWhilePausedAndOutsideTheWorkoutAreIgnored)
{
    WorkoutModel m;
    EXPECT_FALSE(m.addSample(0, 120));
    m.start(0, cfg());
    m.pause(1000);
    m.addSample(2000, 150);
    EXPECT_EQ(m.current().hrCount, 0u);
    m.resume(3000);
    m.stop(4000);
    m.addSample(5000, 150);
    EXPECT_EQ(m.current().hrCount, 0u);
    EXPECT_EQ(m.summary().zoneMs[0], 2000u);   // the two running seconds, with no heart rate
}

// --- Zones -------------------------------------------------------------------

TEST(HrZones, TheExamplesRuleOnItsDefaultThresholds)
{
    HrZones z;   // 95, 114, 133, 152, 171, 190
    EXPECT_EQ(z.zoneOf(0), 0);
    EXPECT_EQ(z.zoneOf(95), 0);      // at a threshold is still below it
    EXPECT_EQ(z.zoneOf(96), 1);
    EXPECT_EQ(z.zoneOf(114), 1);
    EXPECT_EQ(z.zoneOf(115), 2);
    EXPECT_EQ(z.zoneOf(140), 3);
    EXPECT_EQ(z.zoneOf(160), 4);
    EXPECT_EQ(z.zoneOf(185), 5);
    EXPECT_EQ(z.zoneOf(220), 5);     // above the last threshold: capped at 5
}

TEST(HrZones, FewerThresholdsAndNone)
{
    HrZones z;
    z.count = 2;
    EXPECT_EQ(z.zoneOf(200), 2);
    z.count = 0;
    EXPECT_EQ(z.zoneOf(200), 0);
    z.count = 40;                    // more than there are: only the six are read
    EXPECT_EQ(z.zoneOf(200), 5);
}

TEST(WorkoutModel, TimeGoesToTheZoneKnownWhenItBegan)
{
    for (uint32_t s : kStarts) {
        WorkoutModel m;
        m.start(s, cfg());
        feed(m, s, s + 59000, 140);           // zone 3 from the start
        feed(m, s + 60000, s + 120000, 0);    // the second at 59-60 s is still zone 3
        const SessionSummary sum = m.summary();
        EXPECT_EQ(sum.zoneMs[3], 60000u);
        EXPECT_EQ(sum.zoneMs[0], 60000u);
        EXPECT_EQ(sum.zoneMs[1] + sum.zoneMs[2] + sum.zoneMs[4] + sum.zoneMs[5], 0u);
        EXPECT_EQ(m.zone(), 0);
    }
}

// --- Calories ----------------------------------------------------------------

TEST(Calories, AHandWorkedTwoMinutes)
{
    // 80 kg: 60 s in zone 3 (7 MET) = 7 x 80 x 60 / 3600 = 9.333 kcal, and 60 s
    // below zone 1 (1 MET) = 1.333 kcal: 10.667 in all, 106.67 tenths -> 107.
    // The resting part is 1 MET for all 120 s: 2.667 kcal -> 27 tenths.
    WorkoutModel m;
    m.start(0, cfg(0, 80000));
    feed(m, 0, 59000, 140);
    feed(m, 60000, 120000, 0);
    const SessionSummary sum = m.summary();
    EXPECT_EQ(sum.kcalX10, 107u);
    EXPECT_EQ(sum.restingKcalX10, 27u);
    EXPECT_EQ(m.lapKcalX10(0), 107u);
    EXPECT_EQ(m.lapRestingKcalX10(0), 27u);
}

TEST(Calories, EveryZonesMetAndTheFallbackWeight)
{
    // An hour in each zone at the example's 75 kg default: MET x 75 kcal.
    const uint32_t hour = 3600000u;
    const uint32_t want[kZoneCount] = {750, 1875, 3375, 5250, 7500, 9375};   // tenths
    for (uint8_t z = 0; z < kZoneCount; ++z) {
        uint32_t ms[kZoneCount] = {};
        ms[z] = hour;
        EXPECT_EQ(caloriesX10(ms, kDefaultWeightG), want[z]) << int(z);
    }
    EXPECT_EQ(restingCaloriesX10(hour, kDefaultWeightG), 750u);
}

TEST(Calories, NothingIsNothingAndTheLongestDayDoesNotOverflow)
{
    uint32_t none[kZoneCount] = {};
    EXPECT_EQ(caloriesX10(none, 80000), 0u);
    uint32_t all[kZoneCount] = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
    EXPECT_GT(caloriesX10(all, 300000), 0u);   // 300 kg, every counter full: no wrap to a small number
    EXPECT_GT(restingCaloriesX10(UINT32_MAX, 300000), 0u);
}

TEST(Calories, PausedTimeBurnsNothing)
{
    WorkoutModel m;
    m.start(0, cfg(0, 80000));
    m.addSample(0, 140);
    m.pause(30000);
    m.resume(3630000);       // an hour's pause
    m.addSample(3660000, 140);
    m.stop(3660000);
    EXPECT_EQ(m.summary().zoneMs[3], 60000u);
}

// --- Auto lap ----------------------------------------------------------------

TEST(AutoLap, EndsTheLapAtItsActiveTime)
{
    for (uint32_t s : kStarts) {
        WorkoutModel m;
        m.start(s, cfg(60000));
        EXPECT_EQ(feed(m, s, s + 130000, 120), 2);
        EXPECT_EQ(m.lapCount(), 3);
        EXPECT_TRUE(m.lapAt(0).autoEnded);
        EXPECT_EQ(m.lapAt(0).activeMs, 60000u);
        EXPECT_EQ(m.lapAt(1).activeMs, 60000u);
        EXPECT_FALSE(m.lapAt(2).autoEnded);
    }
}

TEST(AutoLap, PausesDoNotCountAndAButtonLapRestartsIt)
{
    WorkoutModel m;
    m.start(0, cfg(60000));
    feed(m, 0, 30000, 120);
    m.pause(30000);
    m.resume(90000);
    EXPECT_EQ(feed(m, 91000, 119000, 120), 0);   // 59 s active
    EXPECT_EQ(m.lap(119500), LapResult::Ok);
    EXPECT_EQ(feed(m, 120000, 178000, 120), 0);  // 58.5 s into the new lap
    EXPECT_TRUE(m.addSample(180000, 120));
    EXPECT_EQ(m.lapCount(), 3);
    EXPECT_FALSE(m.lapAt(0).autoEnded);
    EXPECT_TRUE(m.lapAt(1).autoEnded);
}

TEST(AutoLap, OffNeverLapsAndAFullWorkoutStopsLapping)
{
    WorkoutModel off;
    off.start(0, cfg(0));
    EXPECT_EQ(feed(off, 0, 600000, 120), 0);
    EXPECT_EQ(off.lapCount(), 1);

    WorkoutModel full;
    full.start(0, cfg(1000));
    feed(full, 0, 100000, 120);
    EXPECT_EQ(full.lapCount(), kMaxLaps);
}

TEST(WorkoutModel, RestartClearsTheLastWorkout)
{
    WorkoutModel m;
    m.start(0, cfg());
    feed(m, 1000, 5000, 130);
    m.lap(6000);
    m.stop(7000);
    m.start(100000, cfg(300000));
    EXPECT_EQ(m.state(), RunState::Running);
    EXPECT_EQ(m.lapCount(), 1);
    EXPECT_EQ(m.current().hrCount, 0u);
    EXPECT_EQ(m.summary().hrMax, 0);
    EXPECT_EQ(m.summary().kcalX10, 0u);
    EXPECT_EQ(m.activeMs(100000), 0u);
    EXPECT_EQ(m.zone(), 0);
    EXPECT_EQ(m.config().autoLapMs, 300000u);
}

TEST(WorkoutModel, LapAtOutOfRangeGivesTheLastLap)
{
    WorkoutModel m;
    m.start(0, cfg());
    m.lap(1000);
    m.addSample(2000, 99);
    EXPECT_EQ(m.lapAt(200).hrMax, 99);
}
