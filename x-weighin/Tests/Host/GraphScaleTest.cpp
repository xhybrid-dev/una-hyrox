#include "GraphScale.hpp"

#include <gtest/gtest.h>

using namespace XWeighIn;

TEST(GraphScale, PaddedAndSnapped)
{
    const uint16_t w[3] = { 8000, 0, 8100 };
    const uint16_t t[3] = { 8020, 8040, 8060 };
    Range r;
    ASSERT_TRUE(rangeFor(w, t, 3, r));
    EXPECT_EQ(r.lo, 7950);   // 80.00 - 0.20, down to 79.50
    EXPECT_EQ(r.hi, 8150);   // 81.00 + 0.20, up to 81.50
}

TEST(GraphScale, AtLeastTwoKilograms)
{
    const uint16_t w[2] = { 8000, 8010 };
    Range r;
    ASSERT_TRUE(rangeFor(w, nullptr, 2, r));
    EXPECT_EQ(r.hi - r.lo, kGraphMinSpan);
    EXPECT_LE(r.lo, 8000 - kGraphPad);
    EXPECT_GE(r.hi, 8010 + kGraphPad);
    EXPECT_EQ(r.lo % kGraphSnap, 0);
}

TEST(GraphScale, WideRanges)
{
    const uint16_t w[2] = { 7000, 9000 };
    Range r;
    ASSERT_TRUE(rangeFor(w, nullptr, 2, r));
    EXPECT_EQ(r.lo, 6950);
    EXPECT_EQ(r.hi, 9050);
}

TEST(GraphScale, NothingToDraw)
{
    const uint16_t w[3] = { 0, 0, 0 };
    Range r;
    EXPECT_FALSE(rangeFor(w, w, 3, r));
    EXPECT_FALSE(rangeFor(w, nullptr, 0, r));
}

TEST(GraphScale, NeverBelowZero)
{
    const uint16_t w[1] = { 50 };   // 0.5 kg: absurd, but must not go negative
    Range r;
    ASSERT_TRUE(rangeFor(w, nullptr, 1, r));
    EXPECT_EQ(r.lo, 0);
    EXPECT_EQ(r.hi - r.lo, kGraphMinSpan);
}

TEST(GraphScale, RowsAndColumns)
{
    Range r;
    r.lo = 8000;
    r.hi = 8200;
    EXPECT_EQ(yFor(8200, r, 60, 101), 60);
    EXPECT_EQ(yFor(8000, r, 60, 101), 160);
    EXPECT_EQ(yFor(8100, r, 60, 101), 110);
    EXPECT_EQ(yFor(9000, r, 60, 101), 60);    // clamped
    EXPECT_EQ(yFor(7000, r, 60, 101), 160);

    EXPECT_EQ(xFor(0, 30, 28, 184), 28);
    EXPECT_EQ(xFor(29, 30, 28, 184), 211);
    EXPECT_EQ(xFor(40, 30, 28, 184), 211);    // clamped to the last slot
    EXPECT_EQ(xFor(0, 1, 28, 184), 28 + 92);  // a single day sits in the middle
}

TEST(GraphScale, Snapping)
{
    EXPECT_EQ(floorSnap(7980), 7950);
    EXPECT_EQ(floorSnap(7950), 7950);
    EXPECT_EQ(floorSnap(-10), -50);
    EXPECT_EQ(ceilSnap(8120), 8150);
    EXPECT_EQ(ceilSnap(8150), 8150);
}
