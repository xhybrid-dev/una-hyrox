/**
 * @file    MapViewTest.cpp
 * @brief   Trail::MapView: route to screen pixels, turned, scaled, clipped.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <vector>

#include "MapView.hpp"
#include "support/RunSim.hpp"

using RunSim::NE;
using Trail::GeoPoint;
using Trail::MapView;
using Trail::ScreenPoint;

namespace
{

MapView::View viewAt(const NE& centre, float metresPerPx, float rotationDeg = 0.0f)
{
    MapView::View v;
    v.centre      = RunSim::at(centre);
    v.metresPerPx = metresPerPx;
    v.rotationDeg = rotationDeg;
    return v;
}

/// Frames are 2 KB: keep them off the test's stack, as the GUI will.
std::unique_ptr<MapView::Frame> project(const std::vector<GeoPoint>& pts, const MapView::View& v)
{
    auto f = std::make_unique<MapView::Frame>();
    MapView::project(pts.data(), static_cast<uint16_t>(pts.size()), v, *f);
    return f;
}

bool allOnScreen(const MapView::Frame& f, const MapView::View& v)
{
    for (uint16_t i = 0; i < f.count; ++i) {
        if (f.points[i].x < 0 || f.points[i].y < 0 || f.points[i].x > v.width || f.points[i].y > v.height) {
            return false;
        }
    }
    return true;
}

} // namespace

TEST(MapView, NorthUpARouteNorthGoesUpTheScreen)
{
    // 1 km north from the runner, at 5 m/px: leaves the top of the screen.
    const auto pts = RunSim::route({ { 0, 0 }, { 1000, 0 } });
    const auto v   = viewAt({ 0, 0 }, 5.0f);
    const auto f   = project(pts, v);
    ASSERT_EQ(f->runs, 1u);
    EXPECT_FALSE(f->truncated);
    const ScreenPoint first = f->points[f->runStart[0]];
    const ScreenPoint last  = f->points[f->runStart[0] + f->runLength[0] - 1];
    EXPECT_EQ(first.x, 120);
    EXPECT_EQ(first.y, 120);
    EXPECT_EQ(last.x, 120);
    EXPECT_EQ(last.y, 0);   // clipped at the top edge
    EXPECT_TRUE(allOnScreen(*f, v));
}

TEST(MapView, HeadingUpTurnsTheRoute)
{
    // Running east, heading-up: a route east goes up; north goes left.
    const auto east  = RunSim::route({ { 0, 0 }, { 0, 300 } });
    const auto north = RunSim::route({ { 0, 0 }, { 300, 0 } });
    const auto v     = viewAt({ 0, 0 }, 5.0f, 90.0f);
    const auto fe    = project(east, v);
    const auto fn    = project(north, v);
    const ScreenPoint e = fe->points[fe->count - 1];
    const ScreenPoint n = fn->points[fn->count - 1];
    EXPECT_NEAR(e.x, 120, 1);
    EXPECT_NEAR(e.y, 60, 1);   // 300 m at 5 m/px = 60 px up
    EXPECT_NEAR(n.x, 60, 1);
    EXPECT_NEAR(n.y, 120, 1);
}

TEST(MapView, ARouteThatLeavesAndComesBackIsTwoLines)
{
    // Up past the top edge, across, and back down onto the screen.
    const auto pts = RunSim::route({ { 0, -50 }, { 1000, -50 }, { 1000, 50 }, { 0, 50 } });
    const auto v   = viewAt({ 0, 0 }, 5.0f);
    const auto f   = project(pts, v);
    ASSERT_EQ(f->runs, 2u);
    EXPECT_TRUE(allOnScreen(*f, v));
    // The first run leaves at the top, the second enters there.
    EXPECT_EQ(f->points[f->runStart[0] + f->runLength[0] - 1].y, 0);
    EXPECT_EQ(f->points[f->runStart[1]].y, 0);
}

TEST(MapView, ARouteEntirelyOffScreenDrawsNothing)
{
    const auto pts = RunSim::route({ { 5000, 5000 }, { 6000, 5000 } });
    const auto f   = project(pts, viewAt({ 0, 0 }, 5.0f));
    EXPECT_EQ(f->runs, 0u);
    EXPECT_EQ(f->count, 0u);
}

TEST(MapView, DensePointsAreThinnedToWhatTheScreenCanShow)
{
    // 2,000 points a metre apart, zoomed out to 10 m/px: 200 px of line.
    std::vector<GeoPoint> pts;
    for (int i = 0; i < 2000; ++i) {
        pts.push_back(RunSim::at({ static_cast<double>(i) - 1000.0, 0 }));
    }
    const auto f = project(pts, viewAt({ 0, 0 }, 10.0f));
    ASSERT_EQ(f->runs, 1u);
    EXPECT_LT(f->count, 120u);
    EXPECT_GT(f->count, 50u);
    EXPECT_FALSE(f->truncated);
}

TEST(MapView, ABusyRouteIsCutShortNotOverflowed)
{
    // A zig-zag with ~2,000 corners all on screen at 1 m/px.
    std::vector<NE> zig;
    for (int i = 0; i < 2000; ++i) {
        zig.push_back({ static_cast<double>(i % 100) - 50.0, (i % 2 ? 40.0 : -40.0) + (i / 100) });
    }
    std::vector<GeoPoint> pts;
    for (const NE& p : zig) {
        pts.push_back(RunSim::at(p));
    }
    const auto v = viewAt({ 0, 0 }, 1.0f);
    const auto f = project(pts, v);
    EXPECT_TRUE(f->truncated);
    EXPECT_LE(f->count, MapView::kMaxPoints);
    EXPECT_TRUE(allOnScreen(*f, v));
}

TEST(MapView, ManyExitsStopAtTheRunLimit)
{
    // A comb: in and out of the screen 30 times.
    std::vector<NE> comb;
    for (int i = 0; i < 30; ++i) {
        const double e = -140.0 + i * 10.0;
        comb.push_back({ -2000, e });
        comb.push_back({ 2000, e });
    }
    std::vector<GeoPoint> pts;
    for (const NE& p : comb) {
        pts.push_back(RunSim::at(p));
    }
    const auto f = project(pts, viewAt({ 0, 0 }, 1.5f));
    EXPECT_EQ(f->runs, MapView::kMaxRuns);
    EXPECT_TRUE(f->truncated);
}

TEST(MapView, MarkersKnowWhenTheyAreOffScreen)
{
    const auto  v = viewAt({ 0, 0 }, 5.0f);
    ScreenPoint p;
    EXPECT_TRUE(MapView::toScreen(RunSim::at({ 100, 100 }), v, p));
    EXPECT_NEAR(p.x, 140, 1);
    EXPECT_NEAR(p.y, 100, 1);
    EXPECT_FALSE(MapView::toScreen(RunSim::at({ 50000, 0 }), v, p));
    EXPECT_EQ(p.x, 120);
    EXPECT_LT(p.y, 0);   // clamped, not wrapped
}

TEST(MapView, FitShowsTheWholeRouteInTheCircle)
{
    const auto pts = RunSim::route({ { 0, 0 }, { 3000, 1000 }, { 5000, -2000 }, { 0, -4000 } }, 50.0);
    const auto v   = MapView::fit(pts.data(), static_cast<uint16_t>(pts.size()), 240, 240, 12);
    for (const GeoPoint& g : pts) {
        ScreenPoint p;
        ASSERT_TRUE(MapView::toScreen(g, v, p));
        const float r = std::hypot(static_cast<float>(p.x - 120), static_cast<float>(p.y - 120));
        EXPECT_LE(r, 108.5f);
    }
    // And fills it: some point is near the edge of the circle.
    float maxR = 0.0f;
    for (const GeoPoint& g : pts) {
        ScreenPoint p;
        MapView::toScreen(g, v, p);
        maxR = std::max(maxR, std::hypot(static_cast<float>(p.x - 120), static_cast<float>(p.y - 120)));
    }
    EXPECT_GT(maxR, 105.0f);
}

TEST(MapView, ZoomLevels)
{
    EXPECT_FLOAT_EQ(MapView::zoomScale(0, 100), 1.0f);
    EXPECT_FLOAT_EQ(MapView::zoomScale(2, 100), 5.0f);
    EXPECT_FLOAT_EQ(MapView::zoomScale(99, 100), 25.0f);   // clamped to the widest
}
