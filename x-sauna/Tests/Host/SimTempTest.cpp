#include <gtest/gtest.h>

#include "SimTemp.hpp"

using Sauna::kSimTempPeriodS;
using Sauna::syntheticTempX100;

TEST(SimTemp, StartsInARoomAndClimbs)
{
    EXPECT_EQ(syntheticTempX100(0), 2200);
    EXPECT_EQ(syntheticTempX100(29), 2200);
    EXPECT_EQ(syntheticTempX100(30), 2200);
    EXPECT_GT(syntheticTempX100(200), 2200);
}

TEST(SimTemp, PeaksAtEightyFiveAndHolds)
{
    EXPECT_EQ(syntheticTempX100(330), 8500);
    EXPECT_EQ(syntheticTempX100(389), 8500);
}

TEST(SimTemp, RisesNeverFallsOnTheClimb)
{
    for (uint32_t t = 31; t < 330; ++t) {
        ASSERT_GE(syntheticTempX100(t), syntheticTempX100(t - 1)) << t;
    }
}

TEST(SimTemp, DropsToTwelveThenHolds)
{
    EXPECT_EQ(syntheticTempX100(449), 1200);   // the drop lands on the cold value
    EXPECT_EQ(syntheticTempX100(450), 1200);
    EXPECT_EQ(syntheticTempX100(599), 1200);
}

TEST(SimTemp, NeverRisesOnTheDrop)
{
    for (uint32_t t = 391; t < 450; ++t) {
        ASSERT_LE(syntheticTempX100(t), syntheticTempX100(t - 1)) << t;
    }
}

TEST(SimTemp, RepeatsEveryPeriod)
{
    for (uint32_t t = 0; t < kSimTempPeriodS; t += 37) {
        EXPECT_EQ(syntheticTempX100(t), syntheticTempX100(t + kSimTempPeriodS)) << t;
        EXPECT_EQ(syntheticTempX100(t), syntheticTempX100(t + 7 * kSimTempPeriodS)) << t;
    }
}

TEST(SimTemp, StaysInAPlausibleRange)
{
    for (uint32_t t = 0; t < 2 * kSimTempPeriodS; ++t) {
        const int32_t v = syntheticTempX100(t);
        ASSERT_GE(v, 1200) << t;
        ASSERT_LE(v, 8500) << t;
    }
}
