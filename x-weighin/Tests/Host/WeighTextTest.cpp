#include "WeighText.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

using namespace XWeighIn;

namespace
{
std::string kg(int32_t hundredths, uint8_t decimals)
{
    char buf[16];
    formatKg(hundredths, decimals, buf, sizeof(buf));
    return buf;
}
}  // namespace

TEST(FormatKg, TwoDecimals)
{
    EXPECT_EQ(kg(8230, 2), "82.30");
    EXPECT_EQ(kg(8205, 2), "82.05");
    EXPECT_EQ(kg(1, 2), "0.01");
    EXPECT_EQ(kg(50000, 2), "500.00");
    // Anything above 2 is taken as 2.
    EXPECT_EQ(kg(8230, 9), "82.30");
}

TEST(FormatKg, OneDecimalRoundsHalfUp)
{
    EXPECT_EQ(kg(8230, 1), "82.3");
    EXPECT_EQ(kg(8234, 1), "82.3");
    EXPECT_EQ(kg(8235, 1), "82.4");
    EXPECT_EQ(kg(8295, 1), "83.0");
    EXPECT_EQ(kg(4, 1), "0.0");
    EXPECT_EQ(kg(5, 1), "0.1");
}

TEST(FormatKg, NoDecimalsRoundsHalfUp)
{
    EXPECT_EQ(kg(8249, 0), "82");
    EXPECT_EQ(kg(8250, 0), "83");
}

TEST(FormatKg, NotSetIsDashes)
{
    EXPECT_EQ(kg(0, 2), "--");
    EXPECT_EQ(kg(-100, 1), "--");
    EXPECT_EQ(kg(10000000, 2), "--");
    EXPECT_EQ(kg(9999999, 2), "99999.99");
}

TEST(FormatKg, BoundedBuffers)
{
    char buf[4];
    std::memset(buf, 'x', sizeof(buf));
    // "82.30" needs 6 bytes: truncated, terminated, and the full length reported.
    EXPECT_EQ(formatKg(8230, 2, buf, sizeof(buf)), 5u);
    EXPECT_STREQ(buf, "82.");

    char one[1] = { 'x' };
    formatKg(8230, 2, one, sizeof(one));
    EXPECT_EQ(one[0], '\0');

    EXPECT_EQ(formatKg(8230, 2, nullptr, 8), 0u);
    char untouched[2] = { 'x', 'y' };
    EXPECT_EQ(formatKg(8230, 2, untouched, 0), 0u);
    EXPECT_EQ(untouched[0], 'x');
}

namespace
{
std::string change(int32_t hundredths, uint8_t decimals)
{
    char buf[16];
    formatChange(hundredths, decimals, buf, sizeof(buf));
    return buf;
}
}  // namespace

TEST(FormatChange, SignedWithoutJudgement)
{
    EXPECT_EQ(change(40, 1), "+0.4");
    EXPECT_EQ(change(-40, 1), "-0.4");
    EXPECT_EQ(change(-123, 2), "-1.23");
    EXPECT_EQ(change(123, 2), "+1.23");
    EXPECT_EQ(change(12345, 1), "+123.5");
    EXPECT_EQ(change(150, 0), "+2");
}

TEST(FormatChange, RoundsHalfAwayFromZero)
{
    EXPECT_EQ(change(5, 1), "+0.1");
    EXPECT_EQ(change(-5, 1), "-0.1");
    EXPECT_EQ(change(-35, 1), "-0.4");
}

TEST(FormatChange, NothingHasNoSign)
{
    EXPECT_EQ(change(0, 1), "0.0");
    EXPECT_EQ(change(4, 1), "0.0");
    EXPECT_EQ(change(-4, 1), "0.0");
    EXPECT_EQ(change(-49, 0), "0");
    EXPECT_EQ(change(0, 2), "0.00");
}

TEST(FormatChange, OutOfRangeAndBuffers)
{
    EXPECT_EQ(change(10000000, 1), "--");
    EXPECT_EQ(change(-10000000, 1), "--");
    char buf[3];
    EXPECT_EQ(formatChange(-123, 2, buf, sizeof(buf)), 5u);
    EXPECT_STREQ(buf, "-1");
    EXPECT_EQ(formatChange(40, 1, nullptr, 8), 0u);
}

TEST(SexLabel, FemaleMaleOrNothing)
{
    EXPECT_STREQ(sexLabel(1), "female");
    EXPECT_STREQ(sexLabel(2), "male");
    EXPECT_EQ(sexLabel(0), nullptr);
    EXPECT_EQ(sexLabel(3), nullptr);
    EXPECT_EQ(sexLabel(255), nullptr);
}
