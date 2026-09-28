/**
 * @file    RouteTrackerTest.cpp
 * @brief   Trail::RouteTracker on the shapes that break "nearest point":
 *          loops, out-and-backs, figure-of-eights, mid-route starts, noise,
 *          wrong turns.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "CourseOverGround.hpp"
#include "RouteTracker.hpp"
#include "support/RunSim.hpp"

using RunSim::NE;
using Trail::CourseOverGround;
using Trail::GeoPoint;
using Trail::RouteTracker;

namespace
{

/// A route and its tracker, run fix by fix as the service does.
struct Track {
    std::vector<GeoPoint> pts;
    std::vector<float>    cum;
    RouteTracker          tracker;

    explicit Track(const std::vector<NE>& way, double spacingM = 25.0, float lengthM = 0.0f)
        : pts(RunSim::route(way, spacingM))
        , cum(pts.size())
        , tracker(pts.data(), static_cast<uint16_t>(pts.size()), cum.data(), lengthM)
    {
    }

    const RouteTracker::Position& fix(const GeoPoint& p) { return tracker.update(p); }

    /// Run every fix; return the along distance after each.
    std::vector<float> runAll(const std::vector<GeoPoint>& fixes)
    {
        std::vector<float> along;
        for (const GeoPoint& p : fixes) {
            along.push_back(fix(p).alongM);
        }
        return along;
    }
};

/// The largest gap between progress and the true distance run, for a run at
/// @p speed m/s along the route itself (fix i is i * speed along).
float worstError(const std::vector<float>& along, double speed, double length)
{
    float worst = 0.0f;
    for (size_t i = 0; i < along.size(); ++i) {
        const double truth = std::min(static_cast<double>(i) * speed, length);
        worst              = std::max(worst, static_cast<float>(std::fabs(along[i] - truth)));
    }
    return worst;
}

/// The largest backwards step in a sequence of along distances.
float worstBackstep(const std::vector<float>& along)
{
    float worst = 0.0f;
    for (size_t i = 1; i < along.size(); ++i) {
        worst = std::max(worst, along[i - 1] - along[i]);
    }
    return worst;
}

const std::vector<NE> kStraight { { 0, 0 }, { 1000, 0 } };
const std::vector<NE> kSquareLoop { { 0, 0 }, { 500, 0 }, { 500, 500 }, { 0, 500 }, { 0, 0 } };
const std::vector<NE> kOutAndBack { { 0, 0 }, { 1000, 0 }, { 0, 0 } };
// A small bow tie: the diagonals cross at (50, 50), 70 m and 311 m along,
// well inside one search window of each other.
const std::vector<NE> kBowTie { { 0, 0 }, { 100, 100 }, { 100, 0 }, { 0, 100 }, { 0, 0 } };

} // namespace

TEST(RouteTracker, LengthAndCumulativeDistances)
{
    Track t(kSquareLoop);
    EXPECT_NEAR(t.tracker.lengthM(), 2000.0f, 1.0f);
    EXPECT_NEAR(t.tracker.alongAtM(0), 0.0f, 0.01f);
    EXPECT_FALSE(t.tracker.position().everLocked);
    EXPECT_NEAR(t.tracker.position().remainingM, 2000.0f, 1.0f);
}

TEST(RouteTracker, StraightRunCountsDownToTheFinish)
{
    Track t(kStraight);
    const auto along = t.runAll(RunSim::run(kStraight, 3.0));
    const auto& p = t.tracker.position();
    EXPECT_TRUE(p.everLocked);
    EXPECT_TRUE(p.finished);
    EXPECT_NEAR(p.alongM, 1000.0f, 3.0f);
    EXPECT_NEAR(p.remainingM, 0.0f, 3.0f);
    EXPECT_LT(worstBackstep(along), 0.5f);
    // Half way really is half way.
    EXPECT_NEAR(along[along.size() / 2], 500.0f, 5.0f);
}

TEST(RouteTracker, ALoopStartsAtItsStartNotItsFinish)
{
    Track t(kSquareLoop);
    // Standing at the start/finish.
    const auto& p = t.fix(RunSim::at({ 2, 1 }));
    EXPECT_TRUE(p.everLocked);
    EXPECT_FALSE(p.finished);
    EXPECT_LT(p.alongM, 10.0f);
    EXPECT_GT(p.remainingM, 1990.0f);
}

TEST(RouteTracker, ALoopFinishesOnlyAtTheEnd)
{
    Track t(kSquareLoop);
    const auto fixes = RunSim::run(kSquareLoop, 3.0);
    bool finishedEarly = false;
    for (size_t i = 0; i < fixes.size(); ++i) {
        const auto& p = t.fix(fixes[i]);
        if (p.finished && i + 20 < fixes.size()) {
            finishedEarly = true;
        }
    }
    EXPECT_FALSE(finishedEarly);
    EXPECT_TRUE(t.tracker.position().finished);
    EXPECT_NEAR(t.tracker.position().alongM, 2000.0f, 5.0f);
}

TEST(RouteTracker, OutAndBackKeepsGoingForwardThroughTheTurnaround)
{
    Track      t(kOutAndBack);
    const auto along = t.runAll(RunSim::run(kOutAndBack, 3.0));
    // Both legs are the same path: nearest-anywhere would jump back. Expected
    // progress keeps the runner on the return leg.
    EXPECT_LT(worstBackstep(along), 1.0f);
    EXPECT_NEAR(along.back(), 2000.0f, 5.0f);
    EXPECT_TRUE(t.tracker.position().finished);
    // 100 m after the turnaround the runner is 1,100 m along, not 900.
    const size_t after = static_cast<size_t>(1100.0 / 3.0);
    EXPECT_NEAR(along[after], 1100.0f, 6.0f);
}

TEST(RouteTracker, FigureOfEightCrossingDoesNotJump)
{
    Track      t(kBowTie, 10.0);
    const auto along = t.runAll(RunSim::run(kBowTie, 2.0));
    EXPECT_LT(worstBackstep(along), 2.0f);
    EXPECT_NEAR(along.back(), static_cast<float>(RunSim::length(kBowTie)), 4.0f);
    // Crossing the middle the second time (on the second diagonal, ~311 m
    // along), the runner is not sent back to the first diagonal (~70 m).
    const size_t second = static_cast<size_t>(311.0 / 2.0);
    EXPECT_NEAR(along[second], 311.0f, 6.0f);
}

TEST(RouteTracker, StartingPartWayAlongLocksThere)
{
    Track t(kStraight);
    const auto& p = t.fix(RunSim::at({ 600, 5 }));
    EXPECT_TRUE(p.everLocked);
    EXPECT_NEAR(p.alongM, 600.0f, 2.0f);
    EXPECT_NEAR(p.offRouteM, 5.0f, 0.5f);
}

TEST(RouteTracker, FarFromTheRouteNothingLocks)
{
    Track t(kStraight);
    const auto& p = t.fix(RunSim::at({ 0, 500 }));
    EXPECT_FALSE(p.everLocked);
    EXPECT_FALSE(p.onRoute);
    EXPECT_NEAR(p.offRouteM, 500.0f, 1.0f);
    EXPECT_NEAR(p.remainingM, 1000.0f, 1.0f);
}

TEST(RouteTracker, NoisyGpsStaysOnTrack)
{
    Track      t(kOutAndBack);
    const auto along = t.runAll(RunSim::run(kOutAndBack, 3.0, 8.0));
    // Noise of +-8 m makes the reading wobble a little, never leap.
    EXPECT_LT(worstBackstep(along), 12.0f);
    EXPECT_NEAR(along.back(), 2000.0f, 15.0f);
    const size_t after = static_cast<size_t>(1300.0 / 3.0);
    EXPECT_NEAR(along[after], 1300.0f, 15.0f);
}

TEST(RouteTracker, AWrongTurnHoldsProgressThenRejoinsFurtherOn)
{
    Track t(kStraight);
    // On route to 300 m, then 200 m off east, then back on at 700 m.
    const std::vector<NE> wrong { { 0, 0 }, { 300, 0 }, { 400, 200 }, { 600, 200 }, { 700, 0 }, { 1000, 0 } };
    float offAt500 = 0.0f;
    float alongWhileOff = -1.0f;
    for (const GeoPoint& f : RunSim::run(wrong, 3.0)) {
        const auto& p = t.fix(f);
        if (!p.onRoute && alongWhileOff < 0.0f) {
            alongWhileOff = p.alongM;
        }
        if (!p.onRoute) {
            offAt500 = std::max(offAt500, p.offRouteM);
        }
    }
    EXPECT_GT(offAt500, 150.0f);
    // Progress held at the last on-route point (~330 m), not dragged along.
    EXPECT_NEAR(alongWhileOff, 330.0f, 40.0f);
    EXPECT_TRUE(t.tracker.position().finished);
    EXPECT_NEAR(t.tracker.position().alongM, 1000.0f, 3.0f);
}

TEST(RouteTracker, SkippingMostOfTheRouteIsNoFinish)
{
    // 300 m on the route, then 100 m off it all the way to 990 m, and back on
    // for the last 10 m. The tracker follows the rejoin (0 m to go), but 70 %
    // of the route was never run: that is not "route complete". The wrong
    // turn above skips 40 % and does finish.
    Track t(kStraight);
    const std::vector<NE> cut { { 0, 0 }, { 300, 0 }, { 300, 100 }, { 990, 100 }, { 990, 0 }, { 1000, 0 } };
    bool finished = false;
    for (const GeoPoint& f : RunSim::run(cut, 3.0)) {
        finished = finished || t.fix(f).finished;
    }
    EXPECT_FALSE(finished);
    EXPECT_NEAR(t.tracker.position().alongM, 1000.0f, 3.0f);
}

TEST(RouteTracker, LengthFromTheFullGpxScalesProgress)
{
    // The thinned route measures 1,000 m; the GPX said 1,050 m (corners cut).
    Track t(kStraight, 25.0, 1050.0f);
    EXPECT_NEAR(t.tracker.lengthM(), 1050.0f, 0.5f);
    const auto& p = t.fix(RunSim::at({ 500, 0 }));
    EXPECT_NEAR(p.alongM, 525.0f, 2.0f);
    EXPECT_NEAR(p.remainingM, 525.0f, 2.0f);
}

TEST(RouteTracker, ResetForgetsProgress)
{
    Track t(kStraight);
    t.fix(RunSim::at({ 600, 0 }));
    t.tracker.reset();
    EXPECT_FALSE(t.tracker.position().everLocked);
    EXPECT_NEAR(t.tracker.position().remainingM, 1000.0f, 1.0f);
}

TEST(RouteTracker, DegenerateRoutes)
{
    float    cum[1];
    GeoPoint one = RunSim::at({ 0, 0 });
    RouteTracker single(&one, 1, cum);
    EXPECT_TRUE(single.update(RunSim::at({ 10, 0 })).onRoute);
    EXPECT_FALSE(single.update(RunSim::at({ 500, 0 })).onRoute);

    RouteTracker none(nullptr, 0, nullptr);
    EXPECT_FALSE(none.update(one).everLocked);
    EXPECT_EQ(none.count(), 0u);
}

TEST(CourseOverGround, HoldsUntilTheRunnerHasMoved)
{
    CourseOverGround c;
    EXPECT_FALSE(c.update(RunSim::at({ 0, 0 })));
    EXPECT_FALSE(c.update(RunSim::at({ 3, 1 })));   // wander, not movement
    EXPECT_TRUE(c.update(RunSim::at({ 0, 12 })));   // 12 m east
    EXPECT_NEAR(c.headingDeg(), 90.0f, 1.0f);
    EXPECT_TRUE(c.update(RunSim::at({ -1, 13 })));  // stood still: heading held
    EXPECT_NEAR(c.headingDeg(), 90.0f, 1.0f);
    EXPECT_TRUE(c.update(RunSim::at({ -15, 12 })));   // now south
    EXPECT_NEAR(c.headingDeg(), 180.0f, 5.0f);
    c.reset();
    EXPECT_FALSE(c.valid());
}

TEST(RouteTracker, NoisyFigureOfEight)
{
    for (uint32_t seed : { 1u, 2u, 3u, 4u, 5u }) {
        Track      t(kBowTie, 10.0);
        const auto along = t.runAll(RunSim::run(kBowTie, 2.0, 5.0, seed));
        // Never on the wrong stroke (that would be ~240 m out); a fix inside
        // a sharp corner can project up to ~12 m back, which is geometry.
        EXPECT_LT(worstError(along, 2.0, RunSim::length(kBowTie)), 20.0f) << "seed " << seed;
        EXPECT_NEAR(along.back(), static_cast<float>(RunSim::length(kBowTie)), 10.0f) << "seed " << seed;
    }
}

TEST(RouteTracker, OutAndBackOnParallelPaths)
{
    // Out on one side of a stream, back on the other, 10 m apart.
    const std::vector<NE> way { { 0, 0 }, { 1000, 0 }, { 1000, 10 }, { 0, 10 } };
    for (uint32_t seed : { 7u, 8u, 9u }) {
        Track      t(way);
        const auto along = t.runAll(RunSim::run(way, 3.0, 6.0, seed));
        EXPECT_LT(worstError(along, 3.0, 2010.0), 20.0f) << "seed " << seed;
        EXPECT_NEAR(along.back(), 2010.0f, 12.0f) << "seed " << seed;
    }
}

TEST(RouteTracker, StandingAtTheTurnaroundThenGoingBack)
{
    Track t(kOutAndBack);
    for (const GeoPoint& f : RunSim::run({ { 0, 0 }, { 1000, 0 } }, 3.0)) {
        t.fix(f);
    }
    // A minute's rest at the turnaround, with GPS wander.
    RunSim::Noise noise(99u);
    for (int i = 0; i < 60; ++i) {
        t.fix(RunSim::at({ 1000 + noise.next(3.0), noise.next(3.0) }));
    }
    // Then back down: the runner must come out on the return leg.
    for (const GeoPoint& f : RunSim::run({ { 1000, 0 }, { 700, 0 } }, 3.0)) {
        t.fix(f);
    }
    EXPECT_NEAR(t.tracker.position().alongM, 1300.0f, 10.0f);
}

namespace
{
/// The SDK simulator's 400 m stadium track (GpsStepCounterSimulator.cpp),
/// @p d metres into a lap: north along x = n, east = e.
NE stadium(double d)
{
    const double s = 84.39, r = 36.5, c = TestRoutes::kPi * r;
    d              = std::fmod(d, 2 * s + 2 * c);
    if (d < s) {
        return { r, d };
    }
    if (d < s + c) {
        const double a = (d - s) / r;
        return { r * std::cos(a), s + r * std::sin(a) };
    }
    if (d < 2 * s + c) {
        return { -r, s - (d - s - c) };
    }
    const double a = (d - 2 * s - c) / r;
    return { -r * std::cos(a), -r * std::sin(a) };
}
} // namespace

TEST(RouteTracker, DriftingAwayNearAReturnLegNeverJumpsToIt)
{
    // Found in the simulator (NOTES, T2): an out-and-back 10 m wide, and a
    // runner lapping a track that leaves it at the first bend. Drifting 56 m
    // away, the return leg 545 m ahead was within reach and was taken; back
    // near the start the tracker then "finished". Now: progress holds while
    // off, rejoins at the start when the runner is really back on the route,
    // and never finishes.
    std::vector<NE> way;
    for (int x = 0; x < 390; x += 5) {
        way.push_back({ 36.5, static_cast<double>(x) });
    }
    for (int x = 385; x >= 0; x -= 5) {
        way.push_back({ 26.5, static_cast<double>(x) });
    }
    Track t(way, 5.0);
    float maxAlong = 0.0f;
    for (int i = 0; i < 400; ++i) {   // five laps of the track
        const auto& p = t.fix(RunSim::at(stadium(i * 5.5)));
        maxAlong      = std::max(maxAlong, p.alongM);
        ASSERT_FALSE(p.finished) << "fix " << i;
    }
    EXPECT_LT(maxAlong, 200.0f);   // never past the first bend (~140 m)
}

TEST(RouteTracker, TheWrongTurnRouteIsNeverFinishedFromAnywhereOnTheTrack)
{
    // Found in the simulator (NOTES, T3): the same route and track, but the
    // run started with the runner elsewhere on the track, and nine seconds
    // after going off course at the bend the watch said "route complete".
    // Start the run at every point of the lap, with the simulator's GPS
    // noise (1.5 m), and lap for three laps: the route (the first straight,
    // then 300 m on east and back) is never run, so never finished.
    std::vector<NE> way;
    for (int x = 0; x < 390; x += 5) {
        way.push_back({ 36.5, static_cast<double>(x) });
    }
    for (int x = 385; x >= 0; x -= 5) {
        way.push_back({ 26.5, static_cast<double>(x) });
    }
    for (int start = 0; start < 400; start += 5) {
        Track        t(way, 5.0);
        RunSim::Noise noise(static_cast<uint32_t>(start) + 1u);
        for (int i = 0; i < 240; ++i) {   // three laps
            NE p = stadium(start + i * 5.5);
            p.n += noise.next(1.5);
            p.e += noise.next(1.5);
            ASSERT_FALSE(t.fix(RunSim::at(p)).finished) << "start " << start << " m, fix " << i;
        }
    }
}

TEST(RouteTracker, TheFinishNeedsTheRunnerAtTheFinish)
{
    // A loop's finish is 45 m from where the runner stands: near enough to be
    // "on" the route by the 60 m rule, not near enough to have finished it.
    Track t(kStraight);
    for (const GeoPoint& f : RunSim::run({ { 0, 0 }, { 950, 0 } }, 3.0)) {
        t.fix(f);
    }
    EXPECT_FALSE(t.fix(RunSim::at({ 990, 45 })).finished);
    EXPECT_TRUE(t.fix(RunSim::at({ 998, 3 })).finished);
}

TEST(RouteTracker, ThreeLapsOfTheSimulatorTrack)
{
    // The route the simulator demo uses (Tools/TestRoutes/make_sim_routes.py).
    std::vector<NE> way;
    for (double d = 0.0; d < 3 * 400.0 - 2.0; d += 5.0) {
        way.push_back(stadium(d));
    }
    way.push_back(stadium(0.0));
    Track              t(way, 5.0);
    std::vector<float> along;
    for (int i = 0; i * 5.5 < 3 * 399.9; ++i) {
        along.push_back(t.fix(RunSim::at(stadium(i * 5.5))).alongM);
    }
    // Laps are the same place: expected progress keeps the right lap.
    EXPECT_LT(worstBackstep(along), 3.0f);
    EXPECT_NEAR(along[static_cast<size_t>(500.0 / 5.5)], 500.0f, 10.0f);   // lap 2, not lap 1
    EXPECT_TRUE(t.fix(RunSim::at(stadium(0.0))).finished);
}
