// Weekly periods: the arithmetic every boundary decision rests on.

#include <gtest/gtest.h>

#include "WeekMath.hpp"

using namespace Streak::WeekMath;

TEST(WeekMath, DaysFromCivilKnownDates)
{
    EXPECT_EQ(daysFromCivil(1970, 1, 1), 0);
    EXPECT_EQ(daysFromCivil(1969, 12, 31), -1);
    EXPECT_EQ(daysFromCivil(2000, 3, 1), 11017);
    EXPECT_EQ(daysFromCivil(2024, 2, 29), 19782);
    EXPECT_EQ(daysFromCivil(2024, 3, 1), 19783);
    EXPECT_EQ(daysFromCivil(2026, 9, 24), 20720);
}

TEST(WeekMath, WeekdaysMatchTheCalendar)
{
    EXPECT_EQ(weekdayOf(daysFromCivil(1970, 1, 1)), 4);   // Thursday
    EXPECT_EQ(weekdayOf(daysFromCivil(2026, 9, 21)), 1);  // Monday
    EXPECT_EQ(weekdayOf(daysFromCivil(2026, 9, 27)), 0);  // Sunday
    EXPECT_EQ(weekdayOf(daysFromCivil(1969, 12, 28)), 0); // a Sunday before the epoch
}

// The Python check from the exploration pass (hybridx-streak/docs/PLAN.md 6.1),
// ported: for every week-start day, over 900 days spanning 29 February 2024
// and two year-ends, the period changes exactly on the start day and nowhere
// else, and increases by one each time.
TEST(WeekMath, PeriodChangesExactlyOnTheStartDay)
{
    const int32_t first = daysFromCivil(2023, 12, 1);
    for (uint8_t start = 0; start < 7; ++start) {
        for (int32_t d = first; d < first + 900; ++d) {
            const int32_t prev = periodOf(d - 1, start);
            const int32_t cur  = periodOf(d, start);
            if (weekdayOf(d) == start) {
                EXPECT_EQ(cur, prev + 1) << "start " << int(start) << " day " << d;
            } else {
                EXPECT_EQ(cur, prev) << "start " << int(start) << " day " << d;
            }
        }
    }
}

TEST(WeekMath, MondayWeekOfTheExplorationPass)
{
    // Mon 21 and Sun 27 September 2026 share a week; Mon 28 starts the next.
    const int32_t mon = daysFromCivil(2026, 9, 21);
    EXPECT_EQ(periodOf(mon, kMonday), periodOf(mon + 6, kMonday));
    EXPECT_EQ(periodOf(mon + 7, kMonday), periodOf(mon, kMonday) + 1);
    EXPECT_EQ(periodStartDay(periodOf(mon + 3, kMonday), kMonday), mon);
}

TEST(WeekMath, DaysLeftCountsToday)
{
    const int32_t mon = daysFromCivil(2026, 9, 21);
    EXPECT_EQ(daysLeftInPeriod(mon, kMonday), 7);
    EXPECT_EQ(daysLeftInPeriod(mon + 3, kMonday), 4);   // Thursday
    EXPECT_EQ(daysLeftInPeriod(mon + 6, kMonday), 1);   // Sunday: last day
    EXPECT_EQ(daysLeftInPeriod(mon + 6, kSunday), 7);   // ...but first of a Sunday week
}

TEST(WeekMath, FloorDivisionRoundsDown)
{
    EXPECT_EQ(floorDiv(-1, 7), -1);
    EXPECT_EQ(floorDiv(-7, 7), -1);
    EXPECT_EQ(floorDiv(-8, 7), -2);
    EXPECT_EQ(floorMod(-1, 7), 6);
}
