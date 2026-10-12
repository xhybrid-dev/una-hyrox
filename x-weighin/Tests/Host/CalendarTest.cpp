#include "Calendar.hpp"

#include <gtest/gtest.h>

using namespace XWeighIn;

TEST(Calendar, KnownDays)
{
    EXPECT_EQ(daysFromCivil(1970, 1, 1), 0);
    // 2026-10-12 12:00 UTC is 1791806400 s (Python's calendar.timegm).
    EXPECT_EQ(daysFromCivil(2026, 10, 12), 1791806400 / 86400);
    // HybridX Streak NOTES: day 20724 is 28 September 2026.
    EXPECT_EQ(daysFromCivil(2026, 9, 28), 20724);
}

TEST(Calendar, RoundTripsEveryAcceptedDay)
{
    for (int32_t day = firstValidDay(); day <= lastValidDay(); ++day) {
        const Date d = civilFromDays(day);
        ASSERT_EQ(daysFromCivil(d.year, d.month, d.day), day) << day;
    }
}

TEST(Calendar, LeapYears)
{
    EXPECT_TRUE(isValidDate(2028, 2, 29));
    EXPECT_FALSE(isValidDate(2026, 2, 29));
    EXPECT_TRUE(isValidDate(2000, 2, 29));     // divisible by 400
    EXPECT_FALSE(isValidDate(2026, 2, 30));
    EXPECT_FALSE(isValidDate(2026, 4, 31));
    EXPECT_TRUE(isValidDate(2026, 12, 31));
}

TEST(Calendar, AcceptedYearsOnly)
{
    EXPECT_TRUE(isValidDate(2000, 1, 1));
    EXPECT_TRUE(isValidDate(2099, 12, 31));
    EXPECT_FALSE(isValidDate(1999, 12, 31));
    EXPECT_FALSE(isValidDate(2100, 1, 1));
    EXPECT_FALSE(isValidDate(2026, 0, 1));
    EXPECT_FALSE(isValidDate(2026, 13, 1));
    EXPECT_FALSE(isValidDate(2026, 1, 0));
    EXPECT_EQ(firstValidDay(), daysFromCivil(2000, 1, 1));
    EXPECT_EQ(lastValidDay(), daysFromCivil(2099, 12, 31));
}
