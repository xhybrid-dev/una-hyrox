/**
 * @file    HeadingFusionTest.cpp
 * @brief   Trail::HeadingFusion: GPS heading when running, compass when slow.
 */

#include <gtest/gtest.h>

#include <cmath>

#include "GeoPoint.hpp"
#include "HeadingFusion.hpp"

using Trail::HeadingFusion;

namespace
{
/// Angle difference, degrees, -180..180.
float diff(float a, float b)
{
    return Trail::Geo::wrap180(a - b);
}
} // namespace

TEST(HeadingFusion, NothingKnownIsNoHeading)
{
    HeadingFusion h;
    h.update(false, 0.0f, 0.0f, false, 0.0f);
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(h.source(), HeadingFusion::Source::None);
}

TEST(HeadingFusion, RunningUsesTheGpsHeadingWhateverTheCompassSays)
{
    HeadingFusion h;
    for (int i = 0; i < 5; ++i) {
        h.update(true, 90.0f, 3.0f, true, 200.0f);   // a swinging wrist
    }
    EXPECT_EQ(h.source(), HeadingFusion::Source::Gps);
    EXPECT_FLOAT_EQ(h.headingDeg(), 90.0f);
}

TEST(HeadingFusion, StandingUsesTheCompass)
{
    HeadingFusion h;
    for (int i = 0; i < 10; ++i) {
        h.update(false, 0.0f, 0.0f, true, 200.0f);
    }
    EXPECT_EQ(h.source(), HeadingFusion::Source::Compass);
    EXPECT_NEAR(h.headingDeg(), 200.0f, 0.5f);
}

TEST(HeadingFusion, TurningOnTheSpotFollowsTheCompass)
{
    HeadingFusion h;
    for (int i = 0; i < 10; ++i) {
        h.update(true, 90.0f, 0.0f, true, 90.0f);   // stopped, facing east, GPS holds east
    }
    EXPECT_NEAR(h.headingDeg(), 90.0f, 1.0f);
    for (int i = 0; i < 20; ++i) {
        h.update(true, 90.0f, 0.0f, true, 180.0f);   // turns to face south
    }
    EXPECT_NEAR(h.headingDeg(), 180.0f, 2.0f);
    EXPECT_EQ(h.source(), HeadingFusion::Source::Compass);
}

TEST(HeadingFusion, LearnsTheDeclinationWhileRunning)
{
    HeadingFusion h;
    // Running due east (GPS 90) with the compass reading 80: 10 degrees to add.
    for (int i = 0; i < 200; ++i) {
        h.update(true, 90.0f, 3.0f, true, 80.0f);
    }
    EXPECT_TRUE(h.offsetKnown());
    EXPECT_NEAR(h.offsetDeg(), 10.0f, 0.5f);
    // Stopped, the same compass reading now gives true east.
    for (int i = 0; i < 30; ++i) {
        h.update(true, 90.0f, 0.0f, true, 80.0f);
    }
    EXPECT_NEAR(h.headingDeg(), 90.0f, 1.0f);
}

TEST(HeadingFusion, TheOffsetIsNotLearntBelowRunningSpeed)
{
    HeadingFusion h;
    for (int i = 0; i < 100; ++i) {
        h.update(true, 90.0f, 1.9f, true, 20.0f);   // a walk: GPS heading used, nothing learnt
    }
    EXPECT_FALSE(h.offsetKnown());
    EXPECT_EQ(h.source(), HeadingFusion::Source::Gps);
}

TEST(HeadingFusion, TheCompassAveragesAcrossNorth)
{
    HeadingFusion h;
    for (int i = 0; i < 40; ++i) {
        h.update(false, 0.0f, 0.0f, true, i % 2 ? 359.0f : 1.0f);
    }
    EXPECT_LT(std::fabs(diff(h.headingDeg(), 0.0f)), 3.0f);   // not 180
}

TEST(HeadingFusion, NoFlickerAtTheSpeedThreshold)
{
    HeadingFusion h;
    h.update(true, 90.0f, 3.0f, true, 100.0f);
    EXPECT_EQ(h.source(), HeadingFusion::Source::Gps);
    h.update(true, 90.0f, 1.5f, true, 100.0f);   // slowing, between the two thresholds
    EXPECT_EQ(h.source(), HeadingFusion::Source::Gps);
    h.update(true, 90.0f, 1.0f, true, 100.0f);   // below kStillMps
    EXPECT_EQ(h.source(), HeadingFusion::Source::Compass);
    h.update(true, 90.0f, 1.5f, true, 100.0f);   // speeding up, not yet moving
    EXPECT_EQ(h.source(), HeadingFusion::Source::Compass);
    h.update(true, 90.0f, 2.0f, true, 100.0f);
    EXPECT_EQ(h.source(), HeadingFusion::Source::Gps);
}

TEST(HeadingFusion, WithoutACompassTheGpsHeadingIsHeldWhileStanding)
{
    HeadingFusion h;
    h.update(true, 45.0f, 3.0f, false, 0.0f);
    h.update(true, 45.0f, 0.0f, false, 0.0f);
    EXPECT_TRUE(h.valid());
    EXPECT_FLOAT_EQ(h.headingDeg(), 45.0f);
}

TEST(HeadingFusion, ResetKeepsTheLearntOffset)
{
    HeadingFusion h;
    for (int i = 0; i < 200; ++i) {
        h.update(true, 90.0f, 3.0f, true, 80.0f);
    }
    h.reset();
    EXPECT_FALSE(h.valid());
    EXPECT_TRUE(h.offsetKnown());
}
