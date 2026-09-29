/**
 * @file    FaceCycleTest.cpp
 * @brief   FaceCycle: paging the data screens, never landing on the map.
 */

#include <gtest/gtest.h>

#include "FaceCycle.hpp"

TEST(FaceCycle, PagesForwardsAndWrapsRoundTheDataScreensOnly)
{
    // map, nav, elevation, run, lap, status: the map is 0.
    EXPECT_EQ(FaceCycle::step(6, 0, 1, +1), 2);
    EXPECT_EQ(FaceCycle::step(6, 0, 4, +1), 5);
    EXPECT_EQ(FaceCycle::step(6, 0, 5, +1), 1);   // wraps past the map
}

TEST(FaceCycle, PagesBackwardsAndWrapsRoundTheDataScreensOnly)
{
    EXPECT_EQ(FaceCycle::step(6, 0, 3, -1), 2);
    EXPECT_EQ(FaceCycle::step(6, 0, 1, -1), 5);   // wraps past the map
}

TEST(FaceCycle, TheMapMayBeAnywhereInTheList)
{
    // intervals, map, nav, run: the map is 1.
    EXPECT_EQ(FaceCycle::step(4, 1, 0, +1), 2);
    EXPECT_EQ(FaceCycle::step(4, 1, 2, -1), 0);
    EXPECT_EQ(FaceCycle::step(4, 1, 3, +1), 0);
}

TEST(FaceCycle, NoMapMeansEveryScreenCycles)
{
    EXPECT_EQ(FaceCycle::step(3, 3, 2, +1), 0);
    EXPECT_EQ(FaceCycle::step(3, 3, 0, -1), 2);
}

TEST(FaceCycle, DegenerateLists)
{
    EXPECT_EQ(FaceCycle::step(0, 0, 0, +1), 0);
    EXPECT_EQ(FaceCycle::step(1, 0, 0, +1), 0);   // only the map
    EXPECT_EQ(FaceCycle::step(1, 1, 0, +1), 0);   // one data screen
    EXPECT_EQ(FaceCycle::step(2, 0, 1, +1), 1);   // map and one data screen
}
