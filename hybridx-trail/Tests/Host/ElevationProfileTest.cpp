/**
 * @file    ElevationProfileTest.cpp
 * @brief   Trail::ElevationProfile: the profile, the climb left, the next climb.
 */

#include <gtest/gtest.h>

#include <vector>

#include "ElevationProfile.hpp"

using Trail::ElevationProfile;

namespace
{

/// A 1,000 m route with a point every 100 m: flat at 100 m for 300 m, up 50 m
/// over the next 300 m, flat at 150 m, then down 60 m to the end.
struct Sample {
    std::vector<float>    cum;
    std::vector<int16_t>  ele;   // half metres
    std::vector<uint16_t> asc;

    Sample()
    {
        const float elev[] = { 100, 100, 100, 100, 117, 133, 150, 150, 150, 120, 90 };
        uint16_t    climb  = 0;
        for (int i = 0; i < 11; ++i) {
            cum.push_back(100.0f * static_cast<float>(i));
            ele.push_back(static_cast<int16_t>(elev[i] * 2));
            if (i > 0 && elev[i] > elev[i - 1]) {
                climb = static_cast<uint16_t>(climb + (elev[i] - elev[i - 1]) + 0.5f);
            }
            asc.push_back(climb);
        }
    }

    ElevationProfile build(bool hasEle = true) const
    {
        ElevationProfile p;
        p.build(cum.data(), ele.data(), asc.data(), static_cast<uint16_t>(cum.size()), hasEle, 1000.0f);
        return p;
    }
};

} // namespace

TEST(ElevationProfile, ARouteWithoutElevationHasNoProfile)
{
    Sample s;
    const ElevationProfile p = s.build(false);
    EXPECT_FALSE(p.valid());
    EXPECT_FLOAT_EQ(p.ascentLeftM(0), 0.0f);
    EXPECT_FALSE(p.nextClimb(0).found);
}

TEST(ElevationProfile, ElevationAlongTheRoute)
{
    Sample s;
    const ElevationProfile p = s.build();
    ASSERT_TRUE(p.valid());
    EXPECT_NEAR(p.elevationM(0), 100.0f, 0.6f);
    EXPECT_NEAR(p.elevationM(500), 133.0f + (150.0f - 133.0f) * 0.0f, 3.0f);
    EXPECT_NEAR(p.elevationM(700), 150.0f, 0.6f);
    EXPECT_NEAR(p.elevationM(1000), 90.0f, 0.6f);
    EXPECT_NEAR(p.minM(), 90.0f, 0.6f);
    EXPECT_NEAR(p.maxM(), 150.0f, 0.6f);
}

TEST(ElevationProfile, ClimbLeft)
{
    Sample s;
    const ElevationProfile p = s.build();
    EXPECT_NEAR(p.totalAscentM(), 50.0f, 1.5f);
    EXPECT_NEAR(p.ascentLeftM(0), 50.0f, 1.5f);
    EXPECT_NEAR(p.ascentLeftM(400), 33.0f, 2.0f);       // 17 m of the climb (which starts at 300 m) is done
    EXPECT_NEAR(p.ascentLeftM(600), 0.0f, 2.0f);
    EXPECT_NEAR(p.ascentLeftM(1000), 0.0f, 0.5f);
}

TEST(ElevationProfile, TheNextClimb)
{
    Sample s;
    const ElevationProfile p = s.build();
    const ElevationProfile::Climb c = p.nextClimb(0);
    ASSERT_TRUE(c.found);
    EXPECT_NEAR(c.startAheadM, 300.0f, 25.0f);
    EXPECT_NEAR(c.riseM, 50.0f, 3.0f);
    EXPECT_NEAR(c.lengthM, 300.0f, 30.0f);
}

TEST(ElevationProfile, AlreadyOnTheClimbMeasuresFromHere)
{
    Sample s;
    const ElevationProfile p = s.build();
    const ElevationProfile::Climb c = p.nextClimb(450);
    ASSERT_TRUE(c.found);
    EXPECT_FLOAT_EQ(c.startAheadM, 0.0f);
    EXPECT_LT(c.riseM, 50.0f);
    EXPECT_GT(c.riseM, 20.0f);
}

TEST(ElevationProfile, NoClimbAfterTheLastOne)
{
    Sample s;
    const ElevationProfile p = s.build();
    EXPECT_FALSE(p.nextClimb(700).found);   // flat, then down
    EXPECT_FALSE(p.nextClimb(1000).found);
}

TEST(ElevationProfile, ASmallRiseIsNotAClimb)
{
    Sample s;
    for (auto& e : s.ele) {
        e = static_cast<int16_t>(200 + (&e - &s.ele[0]) * 2);   // 1 m per 100 m
    }
    const ElevationProfile p = s.build();
    EXPECT_FALSE(p.nextClimb(0).found);
}

TEST(ElevationProfile, ADipInsideAClimbDoesNotEndIt)
{
    Sample s;
    // Up 20, a 5 m dip, up 20 more: one 40 m climb.
    const float elev[] = { 100, 100, 100, 120, 115, 135, 135, 135, 135, 135, 135 };
    for (int i = 0; i < 11; ++i) {
        s.ele[static_cast<size_t>(i)] = static_cast<int16_t>(elev[i] * 2);
    }
    const ElevationProfile p = s.build();
    const ElevationProfile::Climb c = p.nextClimb(0);
    ASSERT_TRUE(c.found);
    EXPECT_NEAR(c.riseM, 35.0f, 4.0f);
}
