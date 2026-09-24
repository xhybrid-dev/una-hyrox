// The trail up every mountain: on the rock, on the screen, always climbing.

#include <gtest/gtest.h>

#include "gui/summit/SummitGeometry.hpp"

using namespace Summit;

namespace
{

bool insideMainPeak(const Mountain& m, Pt p)
{
    const Peak& k = m.main;
    if (p.y < k.apex.y || p.y > k.left.y) {
        return false;
    }
    const double t  = double(p.y - k.apex.y) / double(k.left.y - k.apex.y);
    const double xl = k.apex.x + (k.left.x - k.apex.x) * t;
    const double xr = k.apex.x + (k.right.x - k.apex.x) * t;
    return p.x >= xl && p.x <= xr;
}

bool insideSafeCircle(Pt p)
{
    const int dx = p.x - kCentreX;
    const int dy = p.y - kCentreY;
    return dx * dx + dy * dy <= kSafeRadius * kSafeRadius;
}

constexpr Streak::Shape kShapes[] = { Streak::Shape::Crag, Streak::Shape::Pyramid, Streak::Shape::Massif,
                                      Streak::Shape::Dome, Streak::Shape::Spire };

} // namespace

TEST(SummitGeometry, EveryStepOfEveryClimbIsOnTheRockAndOnScreen)
{
    for (uint8_t c = 0; c < Streak::kClimbCount; ++c) {
        const Mountain m = mountainFor(Streak::kClimbs[c].shape);
        const Trail    t = trailFor(m);
        const uint32_t steps = Streak::kClimbs[c].steps;

        Pt prev = stepPosition(t, 0, steps);
        for (uint32_t s = 0; s <= steps; ++s) {
            for (uint32_t f = 0; f < 256; f += 51) {
                const Pt p = stepPosition(t, s, steps, f);
                EXPECT_TRUE(insideMainPeak(m, p)) << Streak::kClimbs[c].name << " step " << s;
                EXPECT_TRUE(insideSafeCircle(p)) << Streak::kClimbs[c].name << " step " << s
                                                 << " at " << p.x << "," << p.y;
                EXPECT_LE(p.y, prev.y) << Streak::kClimbs[c].name << " step " << s << " goes downhill";
                prev = p;
                if (s == steps) {
                    break;
                }
            }
        }
    }
}

TEST(SummitGeometry, TrailEndsJustUnderTheApex)
{
    for (Streak::Shape shape : kShapes) {
        const Mountain m   = mountainFor(shape);
        const Trail    t   = trailFor(m);
        const Pt       top = stepPosition(t, 5, 5);
        EXPECT_EQ(top.x, m.main.apex.x);
        EXPECT_GT(top.y, m.main.apex.y);
        EXPECT_LE(top.y - m.main.apex.y, 12);
    }
}

TEST(SummitGeometry, EachStepIsAVisibleMove)
{
    // Steps are spaced evenly *along* the trail, so across a switchback bend
    // two neighbours can be closer in a straight line. What must hold is that
    // the climber (radius 6, Climber.cpp) never lands on its own last
    // position: every move is at least its diameter. Arthur's Seat, the
    // smallest hill, is the tightest case.
    //
    // That holds on the first two climbs, which draw a dot per step
    // (Arthur's Seat 4, Snowdon 8): each week is at least half a climber's
    // width in a straight line and a climber's width along the path. On the
    // big mountains a week is a few pixels (Everest: 52 of them), and the
    // growing lime trail carries the progress (DESIGN.md "The scene"); there,
    // each step need only move forward.
    constexpr int kClimberDiameter = 12;
    for (uint8_t c = 0; c < Streak::kClimbCount; ++c) {
        const Mountain m = mountainFor(Streak::kClimbs[c].shape);
        const Trail    t = trailFor(m);
        const uint32_t steps = Streak::kClimbs[c].steps;
        const bool     dotted = steps <= 8;
        for (uint32_t s = 0; s < steps; ++s) {
            const Pt a = stepPosition(t, s, steps);
            const Pt b = stepPosition(t, s + 1, steps);
            const int dx = b.x - a.x, dy = b.y - a.y;
            const int minMove = dotted ? kClimberDiameter / 2 : 1;
            EXPECT_GE(dx * dx + dy * dy, minMove * minMove) << Streak::kClimbs[c].name << " step " << s;
        }
        if (dotted) {
            EXPECT_GE(t.along[t.count - 1] / 16 / int32_t(steps), kClimberDiameter) << Streak::kClimbs[c].name;
        }
    }
}

TEST(SummitGeometry, ShapesAreDistinct)
{
    for (size_t i = 0; i < 5; ++i) {
        for (size_t j = i + 1; j < 5; ++j) {
            const Mountain a = mountainFor(kShapes[i]);
            const Mountain b = mountainFor(kShapes[j]);
            const bool same = a.main.apex.x == b.main.apex.x && a.main.apex.y == b.main.apex.y &&
                              a.main.left.x == b.main.left.x && a.snowPercent == b.snowPercent;
            EXPECT_FALSE(same) << i << " vs " << j;
        }
    }
}

TEST(SummitGeometry, ScalingKeepsTheBase)
{
    const Mountain m = mountainFor(Streak::Shape::Massif, 200, 150);
    EXPECT_EQ(m.main.left.y, 200);
    EXPECT_EQ(m.main.right.y, 200);
    EXPECT_LT(m.main.apex.y, 200 - 76);   // taller than at 100 %
}
