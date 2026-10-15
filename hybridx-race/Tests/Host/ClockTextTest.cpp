/**
 * @file ClockTextTest.cpp
 * @brief The status face's clock follows the watch's 12/24-hour setting.
 */

#include <gtest/gtest.h>

#include "ClockText.hpp"

#include <string>

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

// -- deltaText -----------------------------------------------------------------------------

namespace
{
std::string delta(int32_t ms)
{
    char buf[12];
    Race::deltaText(buf, sizeof(buf), ms);
    return buf;
}
}  // namespace

TEST(DeltaText, BehindIsPlusAndAheadIsMinus)
{
    EXPECT_EQ(delta(23000), "+0:23");
    EXPECT_EQ(delta(-15000), "-0:15");
    EXPECT_EQ(delta(65000), "+1:05");
    EXPECT_EQ(delta(-605000), "-10:05");
}

TEST(DeltaText, RoundsToTheNearestSecondAndNeverShowsASignedZero)
{
    EXPECT_EQ(delta(0), "0:00");
    EXPECT_EQ(delta(499), "0:00");
    EXPECT_EQ(delta(-499), "0:00");
    EXPECT_EQ(delta(500), "+0:01");
    EXPECT_EQ(delta(-500), "-0:01");
    EXPECT_EQ(delta(1499), "+0:01");
}

TEST(DeltaText, AnHourOrMoreGainsAnHoursField)
{
    EXPECT_EQ(delta(3599 * 1000), "+59:59");
    EXPECT_EQ(delta(3600 * 1000), "+1:00:00");
    EXPECT_EQ(delta(-(3723 * 1000)), "-1:02:03");
}

TEST(DeltaText, TheExtremesFitTwelveBytesAndNeverOverflow)
{
    // INT32_MIN negated overflows a 32-bit int: it is handled in 64 bits.
    char buf[12];
    Race::deltaText(buf, sizeof(buf), INT32_MIN);
    EXPECT_EQ(std::string(buf).substr(0, 1), "-");
    EXPECT_LT(std::string(buf).size(), sizeof(buf));
    Race::deltaText(buf, sizeof(buf), INT32_MAX);
    EXPECT_EQ(std::string(buf).substr(0, 1), "+");
    EXPECT_LT(std::string(buf).size(), sizeof(buf));
}

TEST(DeltaText, ASmallBufferIsNeverOverrun)
{
    char tiny[4] = {'x', 'x', 'x', 'x'};
    Race::deltaText(tiny, 3u, 65000);
    EXPECT_EQ(tiny[3], 'x');   // snprintf wrote at most 3 bytes
    Race::deltaText(nullptr, 0u, 1);   // must not crash
}
