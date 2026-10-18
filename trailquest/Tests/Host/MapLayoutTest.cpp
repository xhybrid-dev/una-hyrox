// The map screen's layout: one shape for all five stages, every place inside
// the round face and clear of the text rows, places well apart, and no path
// drawn across a place it does not join.

#include <gtest/gtest.h>

#include "Layout.hpp"
#include "TextFit.hpp"
#include "data/MapLayout.hpp"
#include "data/Stages.hpp"

using namespace TrailQuest;
using MapLayout::Point;

namespace
{

int64_t sq(int64_t v) { return v * v; }

/// Squared distance from @p p to the segment @p a-@p b, times |ab|^2 (to stay in integers).
bool segmentClears(Point a, Point b, Point p, int32_t clearance)
{
    const int64_t abx = b.x - a.x, aby = b.y - a.y;
    const int64_t apx = p.x - a.x, apy = p.y - a.y;
    const int64_t len2 = abx * abx + aby * aby;
    int64_t t = apx * abx + apy * aby;   // projection, scaled by len2
    if (t < 0) {
        t = 0;
    } else if (t > len2) {
        t = len2;
    }
    // Closest point = a + ab * t / len2; compare distances scaled by len2.
    const int64_t cx = a.x * len2 + abx * t;
    const int64_t cy = a.y * len2 + aby * t;
    const int64_t dx = p.x * len2 - cx;
    const int64_t dy = p.y * len2 - cy;
    return dx * dx + dy * dy >= sq(clearance) * len2 * len2;
}

} // namespace

TEST(MapLayout, EveryStageHasTheShapeTheLayoutDraws)
{
    const StageDef& first = kStages[0];
    ASSERT_EQ(first.nodeCount, MapLayout::kPlaces);
    for (const StageDef& st : kStages) {
        ASSERT_EQ(st.nodeCount, first.nodeCount) << st.name;
        for (uint8_t n = 0; n < st.nodeCount; ++n) {
            ASSERT_EQ(st.nodes[n].exitCount, first.nodes[n].exitCount) << st.name << " " << st.nodes[n].name;
            for (uint8_t e = 0; e < st.nodes[n].exitCount; ++e) {
                EXPECT_EQ(st.nodes[n].exits[e].to, first.nodes[n].exits[e].to) << st.name << " " << st.nodes[n].name;
            }
        }
    }
}

TEST(MapLayout, EveryPlaceIsOnTheFaceAndClearOfTheText)
{
    for (uint8_t n = 0; n < MapLayout::kPlaces; ++n) {
        const Point p = MapLayout::point(n);
        // The icon's farthest corner inside the circle, with a margin.
        const int64_t dx = (p.x < 120 ? p.x - MapLayout::kIconHalf : p.x + MapLayout::kIconHalf) - 120;
        const int64_t dy = (p.y < 120 ? p.y - MapLayout::kIconHalf : p.y + MapLayout::kIconHalf) - 120;
        EXPECT_LE(dx * dx + dy * dy, sq(TextFit::kRadius - 6)) << int(n);
        EXPECT_GE(p.y - MapLayout::kIconHalf, Layout::kMapTop) << int(n);
        EXPECT_LE(p.y + MapLayout::kIconHalf, Layout::kMapBottom) << int(n);
    }
}

TEST(MapLayout, PlacesAreWellApart)
{
    for (uint8_t a = 0; a < MapLayout::kPlaces; ++a) {
        for (uint8_t b = a + 1; b < MapLayout::kPlaces; ++b) {
            const Point p = MapLayout::point(a);
            const Point q = MapLayout::point(b);
            EXPECT_GE(sq(p.x - q.x) + sq(p.y - q.y), sq(28)) << int(a) << "-" << int(b);
        }
    }
}

TEST(MapLayout, NoPathRunsOverAPlaceItDoesNotJoin)
{
    const StageDef& st = kStages[0];
    for (uint8_t n = 0; n < st.nodeCount; ++n) {
        for (uint8_t e = 0; e < st.nodes[n].exitCount; ++e) {
            const uint8_t to = st.nodes[n].exits[e].to;
            for (uint8_t other = 0; other < st.nodeCount; ++other) {
                if (other == n || other == to) {
                    continue;
                }
                EXPECT_TRUE(segmentClears(MapLayout::point(n), MapLayout::point(to), MapLayout::point(other),
                                          MapLayout::kIconHalf + 4))
                    << int(n) << "->" << int(to) << " passes " << int(other);
            }
        }
    }
}

TEST(MapLayout, AnIndexOutOfRangeGivesTheStart)
{
    EXPECT_EQ(MapLayout::point(200).x, MapLayout::point(0).x);
    EXPECT_EQ(MapLayout::point(200).y, MapLayout::point(0).y);
}
