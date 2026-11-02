// HybridX activity link: a session's training load (TrainingLoad.hpp).

#include <gtest/gtest.h>

#include "TrainingLoad.hpp"

using namespace HybridXLink;

namespace {
// The watch's default thresholds (the SDK Workout example's), as every app falls back to.
constexpr uint8_t kTh[6] = {95, 114, 133, 152, 171, 190};
} // namespace

TEST(TrainingLoad, TheZoneRuleIsTheApps)
{
    EXPECT_EQ(loadZoneOf(0, kTh, 6), 0);      // no heart rate
    EXPECT_EQ(loadZoneOf(95, kTh, 6), 0);     // at the first threshold: not above it
    EXPECT_EQ(loadZoneOf(96, kTh, 6), 1);
    EXPECT_EQ(loadZoneOf(133, kTh, 6), 2);
    EXPECT_EQ(loadZoneOf(134, kTh, 6), 3);
    EXPECT_EQ(loadZoneOf(172, kTh, 6), 5);
    EXPECT_EQ(loadZoneOf(250, kTh, 6), 5);    // above the sixth: still zone 5
    EXPECT_EQ(loadZoneOf(150, nullptr, 6), 0);
    EXPECT_EQ(loadZoneOf(150, kTh, 0), 0);    // no thresholds: no zone
}

TEST(TrainingLoad, EdwardsCountsZoneTimesMinutes)
{
    // An hour in each zone: 60, 120, 180, 240, 300.
    for (uint8_t z = 1; z <= 5; ++z) {
        uint32_t sec[kLoadZones] = {};
        sec[z] = 3600;
        EXPECT_EQ(edwardsLoad(sec), 60u * z) << int(z);
    }
    // Time below zone 1 counts nothing.
    uint32_t easy[kLoadZones] = {3600, 0, 0, 0, 0, 0};
    EXPECT_EQ(edwardsLoad(easy), 0);
}

TEST(TrainingLoad, JonsRideOfTheFirstOfNovember)
{
    // The UNA app's zones for the 17:38 ride: 6:41 in zone 1, 1:28 in 2, 4:25 in 3,
    // 3:56 in 4 (the rest below zone 1): 401 + 176 + 795 + 944 point-minutes... in seconds.
    uint32_t sec[kLoadZones] = {0, 401, 88, 265, 236, 0};
    // (401 x 1 + 88 x 2 + 265 x 3 + 236 x 4) / 60 = 2316 / 60 = 38.6, rounded to 39.
    EXPECT_EQ(edwardsLoad(sec), 39);
}

TEST(TrainingLoad, MillisecondsGiveTheSameAsSeconds)
{
    uint32_t sec[kLoadZones] = {120, 401, 88, 265, 236, 30};
    uint32_t ms[kLoadZones];
    for (uint8_t z = 0; z < kLoadZones; ++z) {
        ms[z] = sec[z] * 1000u;
    }
    EXPECT_EQ(edwardsLoadMs(ms), edwardsLoad(sec));
}

TEST(TrainingLoad, RoundsToTheNearestAndCaps)
{
    uint32_t sec[kLoadZones] = {0, 29, 0, 0, 0, 0};
    EXPECT_EQ(edwardsLoad(sec), 0);   // 0.48
    sec[1] = 30;
    EXPECT_EQ(edwardsLoad(sec), 1);   // 0.5
    uint32_t huge[kLoadZones] = {0, 0, 0, 0, 0, UINT32_MAX};
    EXPECT_EQ(edwardsLoad(huge), kMaxLoad);
    EXPECT_EQ(edwardsLoadMs(huge), kMaxLoad);
}

TEST(TrainingLoad, TheMeterCountsSecondsInTheirZones)
{
    LoadMeter m;
    m.setZones(kTh, 6);
    for (int i = 0; i < 600; ++i) {
        m.addSecond(140);   // zone 3
    }
    for (int i = 0; i < 300; ++i) {
        m.addSecond(0);     // a strap dropout: zone 0, nothing
    }
    for (int i = 0; i < 120; ++i) {
        m.addSecond(180);   // zone 5
    }
    EXPECT_EQ(m.secondsIn(3), 600u);
    EXPECT_EQ(m.secondsIn(0), 300u);
    EXPECT_EQ(m.secondsIn(5), 120u);
    EXPECT_EQ(m.load(), 40u);   // 10 min x 3 + 2 min x 5
    m.reset();
    EXPECT_EQ(m.load(), 0u);
    EXPECT_EQ(m.secondsIn(3), 0u);
}

TEST(TrainingLoad, AMeterWithNoZonesCountsNothing)
{
    LoadMeter m;                 // setZones never called
    m.addSecond(180);
    EXPECT_EQ(m.load(), 0u);
    m.setZones(kTh, 99);         // more than there are: the first six
    m.addSecond(250);
    EXPECT_EQ(m.secondsIn(5), 1u);
    EXPECT_EQ(m.secondsIn(9), 0u);
}
