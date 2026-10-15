#include <gtest/gtest.h>

#include "RowCounter.hpp"

using Knitting::CounterState;
using Knitting::RowCounter;

TEST(RowCounter, StartsAtSetOneWithNoRows)
{
    RowCounter c;
    EXPECT_EQ(c.state().rows, 0u);
    EXPECT_EQ(c.state().set, 1u);
    EXPECT_EQ(c.state().total, 0u);
}

TEST(RowCounter, EightRowsFourTimes)
{
    RowCounter c;
    for (int set = 1; set <= 4; ++set) {
        for (int row = 1; row <= 8; ++row) {
            EXPECT_TRUE(c.countRow());
            EXPECT_EQ(c.state().rows, static_cast<uint32_t>(row));
        }
        EXPECT_EQ(c.state().set, static_cast<uint32_t>(set));
        if (set < 4) {
            EXPECT_TRUE(c.nextSet());
            EXPECT_EQ(c.state().rows, 0u);
        }
    }
    EXPECT_EQ(c.state().total, 32u);
}

TEST(RowCounter, NextSetOnAnEmptySetDoesNothing)
{
    RowCounter c;
    EXPECT_FALSE(c.nextSet());
    EXPECT_EQ(c.state().set, 1u);
    c.countRow();
    EXPECT_TRUE(c.nextSet());
    EXPECT_FALSE(c.nextSet());      // a double press
    EXPECT_EQ(c.state().set, 2u);
}

TEST(RowCounter, ResetReturnsToTheStart)
{
    RowCounter c;
    c.countRow();
    c.nextSet();
    c.countRow();
    c.reset();
    EXPECT_EQ(c.state().rows, 0u);
    EXPECT_EQ(c.state().set, 1u);
    EXPECT_EQ(c.state().total, 0u);
}

TEST(RowCounter, RowsSaturateAtTheLimit)
{
    RowCounter c;
    for (uint32_t i = 0; i < Knitting::kMaxRows; ++i) {
        ASSERT_TRUE(c.countRow());
    }
    EXPECT_FALSE(c.countRow());
    EXPECT_EQ(c.state().rows, Knitting::kMaxRows);
    EXPECT_EQ(c.state().total, Knitting::kMaxRows);
}

TEST(RowCounter, TotalSaturatesAcrossSets)
{
    CounterState s;
    s.rows = 5;
    s.set = 7;
    s.total = Knitting::kMaxTotal;
    RowCounter c(s);
    EXPECT_FALSE(c.countRow());
    EXPECT_EQ(c.state().rows, 5u);
    EXPECT_EQ(c.state().total, Knitting::kMaxTotal);
}

TEST(RowCounter, SetsSaturateAtTheLimit)
{
    CounterState s;
    s.rows = 1;
    s.set = Knitting::kMaxSets;
    RowCounter c(s);
    EXPECT_FALSE(c.nextSet());
    EXPECT_EQ(c.state().set, Knitting::kMaxSets);
    EXPECT_EQ(c.state().rows, 1u);
}

TEST(RowCounter, AStateFromAFileIsClamped)
{
    CounterState s;
    s.rows = 5000;
    s.set = 0;
    s.total = 4000000000u;
    RowCounter c(s);
    EXPECT_EQ(c.state().rows, Knitting::kMaxRows);
    EXPECT_EQ(c.state().set, 1u);
    EXPECT_EQ(c.state().total, Knitting::kMaxTotal);
}

TEST(RowCounter, ResetSaysWhetherAnythingChanged)
{
    RowCounter c;
    EXPECT_FALSE(c.reset());        // already at the start: nothing to save
    c.countRow();
    EXPECT_TRUE(c.reset());
    EXPECT_FALSE(c.reset());
}
