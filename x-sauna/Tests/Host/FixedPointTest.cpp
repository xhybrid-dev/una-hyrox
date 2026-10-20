#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <limits>
#include <string>

#include "FixedPoint.hpp"

using Sauna::formatX100;
using Sauna::kX100Invalid;
using Sauna::toX100;

namespace {
std::string fmt(int32_t x100, uint8_t decimals = 2)
{
    char buf[24];
    formatX100(buf, sizeof(buf), x100, decimals);
    return buf;
}
} // namespace

TEST(FixedPoint, RoundsHalfAwayFromZero)
{
    EXPECT_EQ(toX100(23.45f), 2345);
    EXPECT_EQ(toX100(0.0f), 0);
    EXPECT_EQ(toX100(-0.004f), 0);
    EXPECT_EQ(toX100(0.005f), 1);
    EXPECT_EQ(toX100(-0.005f), -1);
    EXPECT_EQ(toX100(-12.5f), -1250);
    EXPECT_EQ(toX100(85.0f), 8500);
}

TEST(FixedPoint, NotANumberIsInvalid)
{
    EXPECT_EQ(toX100(std::numeric_limits<float>::quiet_NaN()), kX100Invalid);
}

TEST(FixedPoint, InfinityAndHugeValuesSaturate)
{
    const float inf = std::numeric_limits<float>::infinity();
    EXPECT_EQ(toX100(inf), 2000000000);
    EXPECT_EQ(toX100(-inf), -2000000000);
    EXPECT_EQ(toX100(3.0e38f), 2000000000);
    EXPECT_EQ(toX100(-3.0e38f), -2000000000);
    EXPECT_NE(toX100(-3.0e38f), kX100Invalid);
}

TEST(FixedPoint, FormatsTwoOneAndNoDecimals)
{
    EXPECT_EQ(fmt(2345), "23.45");
    EXPECT_EQ(fmt(2345, 1), "23.5");        // 23.45 rounds up
    EXPECT_EQ(fmt(2344, 1), "23.4");
    EXPECT_EQ(fmt(2350, 0), "24");
    EXPECT_EQ(fmt(2349, 0), "23");
    EXPECT_EQ(fmt(7, 2), "0.07");
    EXPECT_EQ(fmt(5, 1), "0.1");
    EXPECT_EQ(fmt(100, 2), "1.00");
    EXPECT_EQ(fmt(0, 2), "0.00");
    EXPECT_EQ(fmt(0, 0), "0");
}

TEST(FixedPoint, NegativeValuesKeepTheirSign)
{
    EXPECT_EQ(fmt(-2345), "-23.45");
    EXPECT_EQ(fmt(-50), "-0.50");
    EXPECT_EQ(fmt(-2345, 1), "-23.5");
    EXPECT_EQ(fmt(-1250, 0), "-13");
}

TEST(FixedPoint, NoNegativeZero)
{
    EXPECT_EQ(fmt(-4, 1), "0.0");
    EXPECT_EQ(fmt(-49, 0), "0");
}

TEST(FixedPoint, InvalidIsTwoDashes)
{
    EXPECT_EQ(fmt(kX100Invalid), "--");
    EXPECT_EQ(fmt(kX100Invalid, 0), "--");
}

TEST(FixedPoint, ExtremesFormatWithoutOverflow)
{
    EXPECT_EQ(fmt(INT32_MAX, 2), "21474836.47");
    EXPECT_EQ(fmt(INT32_MIN + 1, 2), "-21474836.47");
    EXPECT_EQ(fmt(INT32_MAX, 0), "21474836");
}

TEST(FixedPoint, DecimalsAboveTwoActAsTwo)
{
    EXPECT_EQ(fmt(2345, 9), "23.45");
}

TEST(FixedPoint, CutsShortAndTerminatesInASmallBuffer)
{
    char buf[4];
    const size_t n = formatX100(buf, sizeof(buf), 2345, 2);
    EXPECT_EQ(n, 3u);
    EXPECT_STREQ(buf, "23.");

    char one[1] = {'x'};
    EXPECT_EQ(formatX100(one, sizeof(one), 2345, 2), 0u);
    EXPECT_EQ(one[0], '\0');
}

TEST(FixedPoint, ZeroCapacityAndNullWriteNothing)
{
    char c = 'x';
    EXPECT_EQ(formatX100(&c, 0, 100), 0u);
    EXPECT_EQ(c, 'x');
    EXPECT_EQ(formatX100(nullptr, 8, 100), 0u);
}

TEST(FixedPoint, NeverWritesPastTheBuffer)
{
    // A guard byte after the buffer must survive every size.
    for (size_t cap = 1; cap <= 16; ++cap) {
        char region[32];
        std::memset(region, 'G', sizeof(region));
        formatX100(region, cap, -2147483647, 2);
        for (size_t i = cap; i < sizeof(region); ++i) {
            ASSERT_EQ(region[i], 'G') << "cap " << cap << " index " << i;
        }
        ASSERT_LT(std::strlen(region), cap);
    }
}
