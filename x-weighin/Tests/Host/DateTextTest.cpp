#include "DateText.hpp"

#include <gtest/gtest.h>

#include <string>

using namespace XWeighIn;

namespace
{
const int32_t kMon12Oct = daysFromCivil(2026, 10, 12);

std::string shortDate(int32_t day)
{
    char buf[24];
    formatShortDate(day, buf, sizeof(buf));
    return buf;
}

std::string ago(int32_t today, int32_t day)
{
    char buf[24];
    agoText(today, day, buf, sizeof(buf));
    return buf;
}
}  // namespace

TEST(DateText, Weekdays)
{
    EXPECT_EQ(weekday(0), 3);              // 1 January 1970, a Thursday
    EXPECT_EQ(weekday(kMon12Oct), 0);      // a Monday
    EXPECT_STREQ(weekdayShort(kMon12Oct + 6), "Sun");
    EXPECT_STREQ(weekdayShort(-1), "Wed"); // 31 December 1969
}

TEST(DateText, ShortDates)
{
    EXPECT_EQ(shortDate(kMon12Oct), "Mon 12 Oct");
    EXPECT_EQ(shortDate(daysFromCivil(2026, 10, 5)), "Mon 5 Oct");
    EXPECT_EQ(shortDate(daysFromCivil(2027, 1, 1)), "Fri 1 Jan");
    char buf[16];
    formatDayMonth(kMon12Oct, buf, sizeof(buf));
    EXPECT_STREQ(buf, "12 Oct");
    EXPECT_STREQ(monthShort(0), "---");
    EXPECT_STREQ(monthShort(13), "---");
}

TEST(DateText, Ago)
{
    EXPECT_EQ(ago(kMon12Oct, kMon12Oct), "today");
    EXPECT_EQ(ago(kMon12Oct, kMon12Oct - 1), "yesterday");
    EXPECT_EQ(ago(kMon12Oct, kMon12Oct - 3), "3 days ago");
    EXPECT_EQ(ago(kMon12Oct, kMon12Oct - 40), "40 days ago");
    EXPECT_EQ(ago(kMon12Oct, kMon12Oct + 1), "Tue 13 Oct");   // the clock went back
}

TEST(DateText, SmallBuffers)
{
    char buf[4];
    formatShortDate(kMon12Oct, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Mon");
    EXPECT_EQ(agoText(kMon12Oct, kMon12Oct, nullptr, 8), 0u);
    EXPECT_EQ(formatShortDate(kMon12Oct, nullptr, 8), 0u);
}
