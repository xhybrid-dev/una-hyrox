#include "BodyMetrics.hpp"

#include <gtest/gtest.h>

using namespace XWeighIn;

TEST(Bmi, KilogramsPerSquareMetreInTenths)
{
    // 82 / 1.82^2 = 24.755...
    EXPECT_EQ(bmiTenths(8200, 182), 248);
    // 70 / 1.75^2 = 22.857...
    EXPECT_EQ(bmiTenths(7000, 175), 229);
    // 100 / 2.00^2 = 25.0 exactly
    EXPECT_EQ(bmiTenths(10000, 200), 250);
}

TEST(Bmi, ZeroWhenNotSet)
{
    EXPECT_EQ(bmiTenths(0, 182), 0);
    EXPECT_EQ(bmiTenths(-8200, 182), 0);
    EXPECT_EQ(bmiTenths(50001, 182), 0);
    EXPECT_EQ(bmiTenths(8200, 0), 0);
    EXPECT_EQ(bmiTenths(8200, kMinHeightCm - 1), 0);
    EXPECT_EQ(bmiTenths(8200, kMaxHeightCm + 1), 0);
}

TEST(Bmr, MifflinStJeor)
{
    // 10 x 82 + 6.25 x 182 - 5 x 40 = 1757.5; + 5 for men, - 161 for women.
    EXPECT_EQ(bmrKcal(8200, 182, 40, 2), 1763);   // 1762.5, rounded up
    EXPECT_EQ(bmrKcal(8200, 182, 40, 1), 1597);   // 1596.5, rounded up
    // 10 x 60 + 6.25 x 165 - 5 x 30 - 161 = 1320.25
    EXPECT_EQ(bmrKcal(6000, 165, 30, 1), 1320);
    // A weight in hundredths: 10 x 82.34 + 1137.5 - 200 + 5 = 1765.9
    EXPECT_EQ(bmrKcal(8234, 182, 40, 2), 1766);
}

TEST(Bmr, ZeroWhenAnythingIsNotSet)
{
    EXPECT_EQ(bmrKcal(0, 182, 40, 2), 0);
    EXPECT_EQ(bmrKcal(8200, 0, 40, 2), 0);
    EXPECT_EQ(bmrKcal(8200, 182, 0, 2), 0);
    EXPECT_EQ(bmrKcal(8200, 182, 40, 0), 0);
    EXPECT_EQ(bmrKcal(8200, 182, 40, 3), 0);
}
