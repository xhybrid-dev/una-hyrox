#include <gtest/gtest.h>

#include "HrZones.hpp"

using namespace Intervals;

TEST(HrZones, ZeroCountIsAlwaysZone0)
{
    const uint8_t thresholds[] = { 100, 120, 140 };
    EXPECT_EQ(zoneOf(150.0f, thresholds, 0), 0u);
}

TEST(HrZones, NonPositiveHrIsZone0)
{
    const uint8_t thresholds[] = { 100, 120, 140 };
    EXPECT_EQ(zoneOf(0.0f, thresholds, 3), 0u);
    EXPECT_EQ(zoneOf(-5.0f, thresholds, 3), 0u);
}

TEST(HrZones, BelowFirstThresholdIsZone0)
{
    const uint8_t thresholds[] = { 100, 120, 140 };
    EXPECT_EQ(zoneOf(99.0f, thresholds, 3), 0u);
}

TEST(HrZones, ExactlyOnAThresholdIsNotYetTheNextZone)
{
    // hr > threshold[i], not >=.
    const uint8_t thresholds[] = { 100, 120, 140 };
    EXPECT_EQ(zoneOf(100.0f, thresholds, 3), 0u);
    EXPECT_EQ(zoneOf(100.5f, thresholds, 3), 1u);
}

TEST(HrZones, ClimbsThroughEveryZone)
{
    const uint8_t thresholds[] = { 100, 120, 140 };
    EXPECT_EQ(zoneOf(110.0f, thresholds, 3), 1u);
    EXPECT_EQ(zoneOf(130.0f, thresholds, 3), 2u);
    EXPECT_EQ(zoneOf(150.0f, thresholds, 3), 3u);
}

TEST(HrZones, CountAboveTheCeilingIsClamped)
{
    // 7 thresholds max (kMaxHrThresholds); a bogus larger count must not
    // read past the array.
    const uint8_t thresholds[kMaxHrThresholds] = { 10, 20, 30, 40, 50, 60, 70 };
    EXPECT_EQ(zoneOf(200.0f, thresholds, 250), kMaxHrThresholds);
}

TEST(HrZones, SixThresholdsGiveSevenZones)
{
    const uint8_t thresholds[] = { 10, 20, 30, 40, 50, 60 };
    EXPECT_EQ(zoneOf(5.0f, thresholds, 6), 0u);
    EXPECT_EQ(zoneOf(65.0f, thresholds, 6), 6u);
}
