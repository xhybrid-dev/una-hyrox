/**
 * @file    RouteBuilderTest.cpp
 * @brief   Trail::RouteBuilder: any length of file into a fixed array.
 */

#include <gtest/gtest.h>

#include <vector>

#include "GpxReader.hpp"
#include "RouteBuilder.hpp"
#include "support/TestRoutes.hpp"

using Trail::GeoPoint;
using Trail::GpxReader;
using Trail::PointKind;
using Trail::RouteBuilder;
namespace Geo = Trail::Geo;

namespace
{
void build(RouteBuilder& b, const std::string& gpx)
{
    GpxReader r(b);
    b.reset();
    r.feed(gpx.data(), gpx.size());
    b.finish();
}

double circumference(double r)
{
    return 2 * TestRoutes::kPi * r;
}
} // namespace

TEST(RouteBuilder, ALoopFitsWithLengthAndClimbFromEveryPoint)
{
    std::vector<GeoPoint> store(2000);
    RouteBuilder          b(store.data(), 2000);
    build(b, TestRoutes::loopTrack(5000, 1600.0));

    EXPECT_EQ(b.rawPoints(), 5001u);
    EXPECT_NEAR(static_cast<double>(b.lengthM()), circumference(1600.0), 5.0);   // 10,053 m
    EXPECT_TRUE(b.hasElevation());
    EXPECT_NEAR(static_cast<double>(b.ascentM()), 80.0, 5.0);
    EXPECT_NEAR(static_cast<double>(b.descentM()), 80.0, 5.0);

    // 5,001 points ~2 m apart at a 10 m spacing: about 1,005 kept.
    EXPECT_EQ(b.spacingM(), 10u);
    EXPECT_GT(b.count(), 990u);
    EXPECT_LT(b.count(), 1010u);
    // First and last kept, and they are the loop's start (a closed loop).
    EXPECT_EQ(b.points()[0].latE7, 544500000);
    EXPECT_LT(Geo::distanceM(b.points()[0], b.points()[b.count() - 1]), 1.0f);
    // Kept points are at least the spacing apart (the last may be closer).
    for (uint16_t i = 1; i + 1 < b.count(); ++i) {
        ASSERT_GE(Geo::distanceM(b.points()[i - 1], b.points()[i]), 10.0f) << i;
    }
}

TEST(RouteBuilder, ALongRouteDoublesTheSpacingToFit)
{
    std::vector<GeoPoint> store(300);
    RouteBuilder          b(store.data(), 300);
    build(b, TestRoutes::loopTrack(5000, 1600.0));

    EXPECT_LE(b.count(), 300u);
    EXPECT_GT(b.count(), 150u);           // not over-thinned: at least half full
    EXPECT_EQ(b.spacingM(), 40u);         // 10 -> 20 -> 40 m: 10 km / 40 m = ~251 points
    EXPECT_NEAR(static_cast<double>(b.lengthM()), circumference(1600.0), 5.0);   // unchanged by thinning
    EXPECT_LT(Geo::distanceM(b.points()[0], b.points()[b.count() - 1]), 1.0f);
}

TEST(RouteBuilder, KeepsTheEndEvenWhenItIsCloseToTheLastKeptPoint)
{
    std::vector<GeoPoint> store(10);
    RouteBuilder          b(store.data(), 10);
    b.reset();
    double lat, lon;
    b.point(TestRoutes::point(TestRoutes::kLat0, TestRoutes::kLon0), false, 0, PointKind::Track);
    TestRoutes::offset(TestRoutes::kLat0, TestRoutes::kLon0, 30, 0, lat, lon);
    b.point(TestRoutes::point(lat, lon), false, 0, PointKind::Track);
    TestRoutes::offset(TestRoutes::kLat0, TestRoutes::kLon0, 33, 0, lat, lon);
    b.point(TestRoutes::point(lat, lon), false, 0, PointKind::Track);
    b.finish();
    ASSERT_EQ(b.count(), 3u);
    EXPECT_EQ(b.points()[2].latE7, TestRoutes::point(lat, lon).latE7);
    EXPECT_EQ(b.lengthM(), 33u);
}

TEST(RouteBuilder, AFullArrayStillEndsAtTheEnd)
{
    std::vector<GeoPoint> store(4);
    RouteBuilder          b(store.data(), 4);
    b.reset();
    double lat, lon;
    for (int i = 0; i <= 40; ++i) {   // 41 points 100 m apart in a line: 4 kept at most
        TestRoutes::offset(TestRoutes::kLat0, TestRoutes::kLon0, 100.0 * i, 0, lat, lon);
        b.point(TestRoutes::point(lat, lon), false, 0, PointKind::Route);
    }
    b.finish();
    EXPECT_LE(b.count(), 4u);
    EXPECT_EQ(b.points()[b.count() - 1].latE7, TestRoutes::point(lat, lon).latE7);
    EXPECT_EQ(b.points()[0].latE7, 544500000);
}

TEST(RouteBuilder, UsesTheFirstKindOnly)
{
    std::vector<GeoPoint> store(10);
    RouteBuilder          b(store.data(), 10);
    b.reset();
    b.point(GeoPoint { 544500000, -30500000 }, false, 0, PointKind::Route);
    b.point(GeoPoint { 544510000, -30500000 }, false, 0, PointKind::Route);
    b.point(GeoPoint { 100000000, 100000000 }, false, 0, PointKind::Track);
    b.finish();
    EXPECT_EQ(b.rawPoints(), 2u);
    EXPECT_EQ(b.ignoredPoints(), 1u);
    EXPECT_EQ(b.count(), 2u);
}

TEST(RouteBuilder, ElevationNoiseInsideTheBandIsNotClimb)
{
    std::vector<GeoPoint> store(100);
    RouteBuilder          b(store.data(), 100);
    b.reset();
    for (int i = 0; i < 100; ++i) {
        // +-2 m of noise on flat ground, then one real 20 m step.
        const int32_t ele = (i % 2 ? 200 : -200) + (i >= 90 ? 2000 : 0);
        b.point(GeoPoint { 544500000 + i * 1000, -30500000 }, true, 10000 + ele, PointKind::Track);
    }
    b.finish();
    // The first point (98 m) is the reference; +-2 m never leaves the band.
    // The step's first point (118 m) is 20 m up; its +-2 m wobble is inside the band.
    EXPECT_EQ(b.ascentM(), 20u);
    EXPECT_EQ(b.descentM(), 0u);
}

TEST(RouteBuilder, EmptyAndSinglePoint)
{
    std::vector<GeoPoint> store(10);
    RouteBuilder          b(store.data(), 10);
    b.reset();
    b.finish();
    EXPECT_EQ(b.count(), 0u);
    EXPECT_EQ(b.lengthM(), 0u);
    b.point(GeoPoint { 544500000, -30500000 }, false, 0, PointKind::Track);
    b.finish();
    EXPECT_EQ(b.count(), 1u);
}
