// The mountain ladder: where a number of achieved weeks puts the climber.

#include <gtest/gtest.h>

#include "Summits.hpp"

using namespace Streak;

TEST(Summits, LadderIsConsistent)
{
    uint16_t floor = 0;
    for (uint8_t i = 0; i < kClimbCount; ++i) {
        EXPECT_EQ(kClimbs[i].summitAt - floor, kClimbs[i].steps) << kClimbs[i].name;
        floor = kClimbs[i].summitAt;
    }
    EXPECT_EQ(kClimbs[0].summitAt, 4);
    EXPECT_EQ(kClimbs[1].summitAt, 12);
    EXPECT_EQ(kClimbs[2].summitAt, 26);
    EXPECT_EQ(kClimbs[3].summitAt, 52);
    EXPECT_EQ(kClimbs[4].summitAt, 104);
}

TEST(Summits, ClimbForWalksTheLadder)
{
    ClimbPosition p = climbFor(0);
    EXPECT_EQ(p.climb, 0);
    EXPECT_EQ(p.stepsClimbed, 0);
    EXPECT_EQ(p.steps, 4);

    p = climbFor(3);                  // one below Arthur's Seat
    EXPECT_EQ(p.climb, 0);
    EXPECT_EQ(p.stepsClimbed, 3);

    p = climbFor(4);                  // summited: at the foot of Snowdon
    EXPECT_EQ(p.climb, 1);
    EXPECT_EQ(p.stepsClimbed, 0);
    EXPECT_EQ(p.steps, 8);

    p = climbFor(25);                 // Ben Nevis, one step from the top
    EXPECT_EQ(p.climb, 2);
    EXPECT_EQ(p.stepsClimbed, 13);
    EXPECT_EQ(p.steps, 14);
}

TEST(Summits, EverestRepeats)
{
    ClimbPosition p = climbFor(103);
    EXPECT_EQ(p.climb, 4);
    EXPECT_EQ(p.ascent, 1);
    EXPECT_EQ(p.stepsClimbed, 51);

    p = climbFor(104);
    EXPECT_EQ(p.climb, 4);
    EXPECT_EQ(p.ascent, 2);
    EXPECT_EQ(p.stepsClimbed, 0);
    EXPECT_EQ(p.steps, 52);

    p = climbFor(104 + 52 + 5);
    EXPECT_EQ(p.ascent, 3);
    EXPECT_EQ(p.stepsClimbed, 5);
}

TEST(Summits, StepsNeverReachTheTopWithinAClimb)
{
    for (uint32_t w = 0; w < 400; ++w) {
        const ClimbPosition p = climbFor(w);
        EXPECT_LT(p.stepsClimbed, p.steps) << w;
    }
}
