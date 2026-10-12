#include "WeighProfile.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

using namespace XWeighIn;

namespace
{
// UTC instants, from Python's calendar.timegm.
constexpr std::time_t k2026Jan1      = 1767225600;  // 2026-01-01 00:00:00
constexpr std::time_t k2025Dec31Last = 1767225599;  // 2025-12-31 23:59:59
constexpr std::time_t k2026Apr30Last = 1777593599;  // 2026-04-30 23:59:59
constexpr std::time_t k2026May1      = 1777593600;  // 2026-05-01 00:00:00
constexpr std::time_t k2026Oct12Noon = 1791806400;  // 2026-10-12 12:00:00
constexpr std::time_t k2026Dec31Last = 1798761599;  // 2026-12-31 23:59:59
}  // namespace

TEST(ClockIsSet, FloorIsTheFirstOf2026)
{
    EXPECT_TRUE(clockIsSet(k2026Jan1));
    EXPECT_FALSE(clockIsSet(k2025Dec31Last));
    EXPECT_FALSE(clockIsSet(0));
    EXPECT_FALSE(clockIsSet(-1));
}

TEST(AgeAt, WholeYearsTurningInTheBirthMonth)
{
    // Born May 1986: 39 until the end of April 2026, 40 from 1 May.
    EXPECT_EQ(ageAt(1986, 5, k2026Apr30Last), 39);
    EXPECT_EQ(ageAt(1986, 5, k2026May1), 40);
    EXPECT_EQ(ageAt(1986, 5, k2026Oct12Noon), 40);
}

TEST(AgeAt, MonthZeroCountsAsJanuary)
{
    EXPECT_EQ(ageAt(1986, 0, k2026Jan1), 40);
    EXPECT_EQ(ageAt(1986, 1, k2026Jan1), 40);
    // Out of range months are treated as January too.
    EXPECT_EQ(ageAt(1986, 13, k2026Jan1), 40);
}

TEST(AgeAt, DecemberBirthdayOnTheLastDay)
{
    EXPECT_EQ(ageAt(1990, 12, k2026Dec31Last), 36);
    EXPECT_EQ(ageAt(1990, 12, k2026Oct12Noon), 35);
}

TEST(AgeAt, ZeroWhenUnknownOrImplausible)
{
    EXPECT_EQ(ageAt(0, 5, k2026Oct12Noon), 0);        // birth year not set
    EXPECT_EQ(ageAt(1986, 5, k2025Dec31Last), 0);     // clock not set
    EXPECT_EQ(ageAt(1986, 5, 0), 0);
    EXPECT_EQ(ageAt(2026, 1, k2026Oct12Noon), 0);     // age 0
    EXPECT_EQ(ageAt(2030, 1, k2026Oct12Noon), 0);     // in the future
    EXPECT_EQ(ageAt(1900, 1, k2026Oct12Noon), 0);     // over 120
    EXPECT_EQ(ageAt(1906, 1, k2026Oct12Noon), 120);   // exactly 120 is kept
}

TEST(KgToHundredths, RoundsToNearest)
{
    EXPECT_EQ(kgToHundredths(82.3f), 8230);
    // 80.7f is 80.6999969...: truncation would give 8069.
    EXPECT_EQ(kgToHundredths(80.7f), 8070);
    EXPECT_EQ(kgToHundredths(72.345f), 7235);
    EXPECT_EQ(kgToHundredths(72.344f), 7234);
    EXPECT_EQ(kgToHundredths(0.01f), 1);
    EXPECT_EQ(kgToHundredths(500.0f), 50000);
}

TEST(KgToHundredths, NotSetForNonsense)
{
    EXPECT_EQ(kgToHundredths(0.0f), 0);
    EXPECT_EQ(kgToHundredths(-70.0f), 0);
    EXPECT_EQ(kgToHundredths(500.01f), 0);
    EXPECT_EQ(kgToHundredths(1e30f), 0);
    EXPECT_EQ(kgToHundredths(std::numeric_limits<float>::quiet_NaN()), 0);
    EXPECT_EQ(kgToHundredths(std::numeric_limits<float>::infinity()), 0);
    // Too small to show as a hundredth: rounds to 0, which reads "not set".
    EXPECT_EQ(kgToHundredths(0.004f), 0);
}
