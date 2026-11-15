#include "Date.hpp"

#include <gtest/gtest.h>

using namespace XTraining;

TEST(Date, KnownDays)
{
    EXPECT_EQ(Date::toDayNum(20000101u), 0);
    EXPECT_EQ(Date::weekday(20000101u), Weekday::Sat);
    EXPECT_EQ(Date::weekday(20261115u), Weekday::Sun);
    EXPECT_EQ(Date::weekday(20261116u), Weekday::Mon);
    EXPECT_EQ(Date::weekday(20261227u), Weekday::Sun);
    EXPECT_EQ(Date::weekday(20240229u), Weekday::Thu);
}

TEST(Date, RoundTripEveryDayFor200Years)
{
    DayNum n = Date::toDayNum(20000101u);
    Ymd last = 0;
    for (Ymd d = 20000101u; d <= 21991231u;) {
        ASSERT_TRUE(Date::valid(d)) << d;
        ASSERT_EQ(Date::toDayNum(d), n) << d;
        ASSERT_EQ(Date::fromDayNum(n), d) << n;
        ASSERT_GT(d, last);
        last = d;
        ++n;
        d = Date::fromDayNum(n);
    }
    EXPECT_EQ(last, 21991231u);
}

TEST(Date, Validity)
{
    EXPECT_TRUE(Date::valid(20240229u));
    EXPECT_FALSE(Date::valid(20230229u));
    EXPECT_FALSE(Date::valid(21000229u));  // not a leap year
    EXPECT_TRUE(Date::valid(20000229u));   // a leap year
    EXPECT_FALSE(Date::valid(20261131u));
    EXPECT_FALSE(Date::valid(20261300u));
    EXPECT_FALSE(Date::valid(0u));
    EXPECT_FALSE(Date::valid(19991231u));
}

TEST(Date, MondaysAndAdding)
{
    const DayNum sun = Date::toDayNum(20261115u);
    EXPECT_EQ(Date::fromDayNum(Date::mondayOf(sun)), 20261109u);
    EXPECT_EQ(Date::fromDayNum(Date::mondayOf(sun + 1)), 20261116u);
    EXPECT_EQ(Date::addDays(20261228u, 7), 20270104u);
    EXPECT_EQ(Date::addDays(20240301u, -1), 20240229u);
}
