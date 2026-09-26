/**
 * @file    RouteMathTest.cpp
 * @brief   Trail::RouteMath::nearest: how far from the line.
 */

#include <gtest/gtest.h>

#include <vector>

#include "RouteMath.hpp"
#include "support/TestRoutes.hpp"

using Trail::GeoPoint;
namespace RouteMath = Trail::RouteMath;

namespace
{
GeoPoint at(double northM, double eastM)
{
    double lat, lon;
    TestRoutes::offset(TestRoutes::kLat0, TestRoutes::kLon0, northM, eastM, lat, lon);
    return TestRoutes::point(lat, lon);
}
} // namespace

TEST(RouteMath, EmptyRouteHasNoAnswer)
{
    EXPECT_FALSE(RouteMath::nearest(nullptr, 0, at(0, 0)).valid);
}

TEST(RouteMath, OnePointRoute)
{
    const GeoPoint only = at(0, 0);
    const auto     n    = RouteMath::nearest(&only, 1, at(30, 40));
    ASSERT_TRUE(n.valid);
    EXPECT_NEAR(n.distanceM, 50.0f, 0.1f);
}

TEST(RouteMath, OnTheLineBesideItAndPastTheEnd)
{
    // An L: 1 km north, then 1 km east.
    const std::vector<GeoPoint> route { at(0, 0), at(1000, 0), at(1000, 1000) };

    auto n = RouteMath::nearest(route.data(), 3, at(500, 0));
    EXPECT_NEAR(n.distanceM, 0.0f, 0.1f);
    EXPECT_EQ(n.segment, 0u);

    n = RouteMath::nearest(route.data(), 3, at(500, 60));   // 60 m east of the first leg
    EXPECT_NEAR(n.distanceM, 60.0f, 0.2f);
    EXPECT_EQ(n.segment, 0u);

    n = RouteMath::nearest(route.data(), 3, at(1080, 500));   // 80 m north of the second leg
    EXPECT_NEAR(n.distanceM, 80.0f, 0.2f);
    EXPECT_EQ(n.segment, 1u);

    n = RouteMath::nearest(route.data(), 3, at(1000, 1030));   // 30 m past the finish
    EXPECT_NEAR(n.distanceM, 30.0f, 0.2f);
}

TEST(RouteMath, ACrossingRouteTakesTheNearerLeg)
{
    // A figure of X: two legs crossing at (500, 500).
    const std::vector<GeoPoint> route { at(0, 0), at(1000, 1000), at(1000, 0), at(0, 1000) };
    const auto                  n = RouteMath::nearest(route.data(), 4, at(500, 510));
    EXPECT_LT(n.distanceM, 8.0f);   // 10 m east of the crossing: ~7.07 m from either diagonal
}
