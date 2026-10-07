/**
 * @file ClockTextTest.cpp
 * @brief The status face's clock follows the watch's 12/24-hour setting.
 */

#include <gtest/gtest.h>

#include "ClockText.hpp"

namespace
{

std::string clock(uint8_t h, uint8_t m, bool twelve)
{
    char buf[8];
    Race::clockText(buf, sizeof(buf), h, m, twelve);
    return buf;
}

}  // namespace

TEST(ClockText, TwentyFourHourKeepsTheLeadingZero)
{
    EXPECT_EQ(clock(0u, 0u, false), "00:00");
    EXPECT_EQ(clock(7u, 5u, false), "07:05");
    EXPECT_EQ(clock(23u, 59u, false), "23:59");
}

TEST(ClockText, TwelveHourReadsTwelveAtMidnightAndNoon)
{
    EXPECT_EQ(clock(0u, 30u, true), "12:30");
    EXPECT_EQ(clock(12u, 0u, true), "12:00");
}

TEST(ClockText, TwelveHourDropsTheLeadingZeroAndWrapsTheAfternoon)
{
    EXPECT_EQ(clock(7u, 5u, true), "7:05");
    EXPECT_EQ(clock(13u, 5u, true), "1:05");
    EXPECT_EQ(clock(23u, 59u, true), "11:59");
}

TEST(ClockText, ASmallBufferIsNeverOverrun)
{
    char buf[3] = { 'x', 'x', 'x' };
    Race::clockText(buf, sizeof(buf), 23u, 59u, false);
    EXPECT_EQ(std::string(buf), "23");
    Race::clockText(nullptr, 8u, 1u, 2u, true);  // must not crash
}
