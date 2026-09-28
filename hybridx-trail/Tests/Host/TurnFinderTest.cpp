/**
 * @file    TurnFinderTest.cpp
 * @brief   Trail::TurnFinder: where the route turns, and which way.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "TurnFinder.hpp"
#include "support/RunSim.hpp"

using RunSim::NE;
using Trail::GeoPoint;
using Trail::Turn;
using Trail::TurnFinder;

namespace
{

/// A route and its cumulative distances, as the navigator holds them.
struct Route {
    std::vector<GeoPoint> pts;
    std::vector<float>    cum;

    explicit Route(const std::vector<NE>& way, double spacingM = 10.0) : pts(RunSim::route(way, spacingM))
    {
        cum.push_back(0.0f);
        for (size_t i = 1; i < pts.size(); ++i) {
            cum.push_back(cum.back() + Trail::Geo::distanceM(pts[i - 1], pts[i]));
        }
    }

    Turn next(float from, float to) const
    {
        return TurnFinder::next(pts.data(), static_cast<uint16_t>(pts.size()), cum.data(), from, to);
    }
};

// North is n; east is e. North for 200 m, then east: a right turn.
const std::vector<NE> kRight { { 0, 0 }, { 200, 0 }, { 200, 200 } };
const std::vector<NE> kLeft { { 0, 0 }, { 200, 0 }, { 200, -200 } };

} // namespace

TEST(TurnFinder, AStraightRouteHasNoTurns)
{
    Route r({ { 0, 0 }, { 500, 0 } });
    EXPECT_FALSE(r.next(0, 500).valid);
}

TEST(TurnFinder, ARightAngleRightIsFoundWhereItIs)
{
    Route r(kRight);
    const Turn t = r.next(0, 400);
    ASSERT_TRUE(t.valid);
    EXPECT_NEAR(t.alongM, 200.0f, 10.0f);
    EXPECT_NEAR(t.angleDeg, 90, 15);
    EXPECT_STREQ(TurnFinder::name(t.angleDeg), "Right");
}

TEST(TurnFinder, ALeftIsNegative)
{
    Route r(kLeft);
    const Turn t = r.next(0, 400);
    ASSERT_TRUE(t.valid);
    EXPECT_NEAR(t.angleDeg, -90, 15);
    EXPECT_STREQ(TurnFinder::name(t.angleDeg), "Left");
}

TEST(TurnFinder, AGentleBendIsNotATurn)
{
    // 30 degrees over the corner: a bend.
    Route r({ { 0, 0 }, { 200, 0 }, { 373, 100 } });
    EXPECT_FALSE(r.next(0, 500).valid);
}

TEST(TurnFinder, AHairpinIsAUTurn)
{
    Route r({ { 0, 0 }, { 200, 0 }, { 200, -12 }, { 0, -12 } });
    const Turn t = r.next(0, 300);
    ASSERT_TRUE(t.valid);
    EXPECT_GE(std::abs(t.angleDeg), 145);
    EXPECT_STREQ(TurnFinder::name(t.angleDeg), "U-turn");
}

TEST(TurnFinder, SharpTurnsAreNamedSharp)
{
    // 130 degrees to the right.
    Route r({ { 0, 0 }, { 200, 0 }, { 200 - 100 * std::cos(50.0 * M_PI / 180.0), 100 * std::sin(50.0 * M_PI / 180.0) + 0 } });
    const Turn t = r.next(0, 400);
    ASSERT_TRUE(t.valid);
    EXPECT_TRUE(TurnFinder::isSharp(t.angleDeg));
    EXPECT_STREQ(TurnFinder::name(t.angleDeg), "Sharp right");
}

TEST(TurnFinder, OnlyTurnsAheadOfTheRunnerAndWithinTheLookahead)
{
    Route r(kRight);
    EXPECT_FALSE(r.next(0, 150).valid);     // the corner is 200 m along
    EXPECT_FALSE(r.next(230, 500).valid);   // and behind
    EXPECT_TRUE(r.next(190, 300).valid);
}

TEST(TurnFinder, TheSameTurnIsFoundWhereverTheRunnerIs)
{
    // The grid is fixed to the route, so the answer does not wobble as the
    // runner advances: the cue cannot flicker or fire twice.
    Route r(kRight);
    float first = -1.0f;
    for (float from = 0.0f; from < 199.0f; from += 1.0f) {
        const Turn t = r.next(from, from + 400.0f);
        ASSERT_TRUE(t.valid) << from;
        if (first < 0.0f) {
            first = t.alongM;
        }
        EXPECT_FLOAT_EQ(t.alongM, first) << from;
    }
}

TEST(TurnFinder, TheNextTurnAfterOnePassed)
{
    // Right at 200 m, then left at 400 m.
    Route r({ { 0, 0 }, { 200, 0 }, { 200, 200 }, { 400, 200 } });
    const Turn a = r.next(0, 600);
    ASSERT_TRUE(a.valid);
    EXPECT_GT(a.angleDeg, 0);
    const Turn b = r.next(a.alongM + 10.0f, 600);
    ASSERT_TRUE(b.valid);
    EXPECT_LT(b.angleDeg, 0);
    EXPECT_NEAR(b.alongM, 400.0f, 12.0f);
}

TEST(TurnFinder, NoTurnsRightAtTheStartOrTheFinish)
{
    // Too close to an end for a chord either side of it: not a turn, and no
    // bogus angle from the clamped end.
    Route r({ { 0, 0 }, { 5, 0 }, { 5, 300 } });   // a corner 5 m in
    EXPECT_FALSE(r.next(0, 100).valid);
    Route e({ { 0, 0 }, { 300, 0 }, { 300, 5 } }); // and 5 m from the end
    EXPECT_FALSE(e.next(200, 400).valid);
}

TEST(TurnFinder, DegenerateRoutes)
{
    EXPECT_FALSE(TurnFinder::next(nullptr, 0, nullptr, 0, 100).valid);
    Route r({ { 0, 0 }, { 5, 0 } });
    EXPECT_FALSE(r.next(0, 100).valid);
}
