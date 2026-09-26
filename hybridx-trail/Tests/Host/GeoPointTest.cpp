/**
 * @file    GeoPointTest.cpp
 * @brief   Distances in Trail::Geo against known answers.
 */

#include <gtest/gtest.h>

#include "GeoPoint.hpp"
#include "support/TestRoutes.hpp"

using Trail::GeoPoint;
namespace Geo = Trail::Geo;

TEST(GeoPoint, FromDegreesRoundsToTheNearestE7)
{
    const GeoPoint p = Geo::fromDegrees(54.45f, -3.05f);
    // A float holds 54.45 as 54.4500007..., so exact equality is not the test:
    // within one float step (about 4e-6 degrees at 54) is.
    EXPECT_NEAR(p.latE7, 544500000, 40);
    EXPECT_NEAR(p.lonE7, -30500000, 3);
    EXPECT_TRUE(Geo::valid(p));
    EXPECT_FALSE(Geo::valid(GeoPoint { 900000001, 0 }));
    EXPECT_FALSE(Geo::valid(GeoPoint { 0, -1800000001 }));
}

TEST(GeoPoint, OneKilometreNorth)
{
    double lat, lon;
    TestRoutes::offset(TestRoutes::kLat0, TestRoutes::kLon0, 1000.0, 0.0, lat, lon);
    const float d = Geo::distanceM(TestRoutes::point(TestRoutes::kLat0, TestRoutes::kLon0), TestRoutes::point(lat, lon));
    EXPECT_NEAR(d, 1000.0f, 0.5f);
}

TEST(GeoPoint, OneKilometreEastAtHighLatitude)
{
    double lat, lon;
    TestRoutes::offset(TestRoutes::kLat0, TestRoutes::kLon0, 0.0, 1000.0, lat, lon);
    const float d = Geo::distanceM(TestRoutes::point(TestRoutes::kLat0, TestRoutes::kLon0), TestRoutes::point(lat, lon));
    EXPECT_NEAR(d, 1000.0f, 0.5f);
}

TEST(GeoPoint, ShortStepsKeepTheirPrecision)
{
    // Two points 2 m apart at a longitude where a float's own step is ~1 m:
    // integer differencing keeps the answer right.
    double lat, lon;
    TestRoutes::offset(TestRoutes::kLat0, 179.9999, 1.2, 1.6, lat, lon);
    const float d = Geo::distanceM(TestRoutes::point(TestRoutes::kLat0, 179.9999), TestRoutes::point(lat, lon));
    EXPECT_NEAR(d, 2.0f, 0.02f);
}

TEST(GeoPoint, AcrossTheAntimeridian)
{
    const GeoPoint west { 0, 1799999000 };    //  179.9999
    const GeoPoint east { 0, -1799999000 };   // -179.9999
    EXPECT_NEAR(Geo::distanceM(west, east), 22.24f, 0.05f);
}

TEST(GeoPoint, DistanceToSegment)
{
    double nLat, nLon, eLat, eLon;
    TestRoutes::offset(TestRoutes::kLat0, TestRoutes::kLon0, 1000.0, 0.0, nLat, nLon);
    TestRoutes::offset(TestRoutes::kLat0, TestRoutes::kLon0, 500.0, 30.0, eLat, eLon);
    const GeoPoint a = TestRoutes::point(TestRoutes::kLat0, TestRoutes::kLon0);
    const GeoPoint b = TestRoutes::point(nLat, nLon);

    // Beside the middle of the segment: the perpendicular distance.
    EXPECT_NEAR(Geo::distanceToSegmentM(TestRoutes::point(eLat, eLon), a, b), 30.0f, 0.1f);
    // Past the end: the distance to the end point.
    double pLat, pLon;
    TestRoutes::offset(nLat, nLon, 40.0, 30.0, pLat, pLon);
    EXPECT_NEAR(Geo::distanceToSegmentM(TestRoutes::point(pLat, pLon), a, b), 50.0f, 0.1f);
    // A zero-length segment is a point.
    EXPECT_NEAR(Geo::distanceToSegmentM(b, a, a), 1000.0f, 0.5f);
}
