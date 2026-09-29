/**
 * @file    MapZoomTest.cpp
 * @brief   MapZoom: stepping the run map in and out, stopping at the ends.
 */

#include <gtest/gtest.h>

#include "MapZoom.hpp"

TEST(MapZoom, StartsCloseIn)
{
    EXPECT_EQ(MapZoom::kRadiiM[MapZoom::kDefault], 150u);
}

TEST(MapZoom, LevelsGetWiderAndEndWithTheWholeRoute)
{
    for (uint8_t i = 1; i < MapZoom::kFixed; ++i) {
        EXPECT_GT(MapZoom::kRadiiM[i], MapZoom::kRadiiM[i - 1]);
    }
    EXPECT_EQ(MapZoom::kWhole, MapZoom::kFixed);
    EXPECT_EQ(MapZoom::kLevels, MapZoom::kFixed + 1);
}

TEST(MapZoom, StepsInAndOutOneAtATime)
{
    EXPECT_EQ(MapZoom::in(MapZoom::kDefault), MapZoom::kDefault - 1);
    EXPECT_EQ(MapZoom::out(MapZoom::kDefault), MapZoom::kDefault + 1);
    EXPECT_EQ(MapZoom::out(MapZoom::in(4)), 4);
}

TEST(MapZoom, StopsAtBothEnds)
{
    EXPECT_EQ(MapZoom::in(0), 0);
    EXPECT_EQ(MapZoom::out(MapZoom::kWhole), MapZoom::kWhole);
    uint8_t level = MapZoom::kDefault;
    for (int i = 0; i < 40; ++i) {
        level = MapZoom::out(level);
    }
    EXPECT_EQ(level, MapZoom::kWhole);
    for (int i = 0; i < 40; ++i) {
        level = MapZoom::in(level);
    }
    EXPECT_EQ(level, 0);
}
