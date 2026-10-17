#include <gtest/gtest.h>

#include "Vo2Settings.hpp"

using namespace RunVo2::Settings;

TEST(Vo2Settings, BirthDateSeedsAndClamps)
{
    uint16_t y = 0;
    uint8_t  m = 0;
    seedBirth(y, m);
    EXPECT_EQ(y, kDefaultBirthYear);
    EXPECT_EQ(m, 1);
    y = 1986; m = 13;
    seedBirth(y, m);
    EXPECT_EQ(y, 1986);
    EXPECT_EQ(m, 1);

    EXPECT_EQ(stepMonth(1, -1), 1);
    EXPECT_EQ(stepMonth(12, +1), 12);
    EXPECT_EQ(stepMonth(5, +1), 6);
    EXPECT_EQ(stepYear(kMinBirthYear, -1), kMinBirthYear);
    EXPECT_EQ(stepYear(kMaxBirthYear, +1), kMaxBirthYear);
    EXPECT_EQ(stepYear(1986, -1), 1985);
    EXPECT_EQ(stepYear(0, +1), kDefaultBirthYear);
}

TEST(Vo2Settings, MaxHrStepsFromAutoToTheValueInUse)
{
    EXPECT_EQ(stepMaxHr(0, +1, 180), 180);         // auto -> the formula's value
    EXPECT_EQ(stepMaxHr(0, -1, 180), 0);           // stays auto
    EXPECT_EQ(stepMaxHr(0, +1, 0), kMinMaxHr);     // nothing in use: the bottom
    EXPECT_EQ(stepMaxHr(0, +1, 250), kMaxMaxHr);
    EXPECT_EQ(stepMaxHr(kMinMaxHr, -1, 180), 0);   // below the range: auto
    EXPECT_EQ(stepMaxHr(kMaxMaxHr, +1, 180), kMaxMaxHr);
    EXPECT_EQ(stepMaxHr(184, +1, 180), 185);
    EXPECT_EQ(stepMaxHr(184, -1, 180), 183);
}

TEST(Vo2Settings, RestHrStepsFromTheWatchFigure)
{
    EXPECT_EQ(stepRestHr(0, +1, 52), 52);
    EXPECT_EQ(stepRestHr(0, +1, 0), kMinRestHr);
    EXPECT_EQ(stepRestHr(kMinRestHr, -1, 52), 0);
    EXPECT_EQ(stepRestHr(kMaxRestHr, +1, 52), kMaxRestHr);
    EXPECT_EQ(stepRestHr(120, -1, 52), 0);         // out of range reads as off
}
