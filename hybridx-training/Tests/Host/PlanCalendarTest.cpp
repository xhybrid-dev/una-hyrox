#include "PlanCalendar.hpp"

#include <gtest/gtest.h>

using namespace XTraining;
using Status = Calendar::Status;

TEST(PlanCalendar, FirstStart)
{
    // Week of Monday 16 November 2026.
    EXPECT_EQ(PlanCalendar::firstStart(20261116u), 20261116u);  // Mon
    EXPECT_EQ(PlanCalendar::firstStart(20261118u), 20261116u);  // Wed: this week
    EXPECT_EQ(PlanCalendar::firstStart(20261119u), 20261123u);  // Thu: next Monday
    EXPECT_EQ(PlanCalendar::firstStart(20261122u), 20261123u);  // Sun
    EXPECT_EQ(PlanCalendar::firstStart(20261231u), 20270104u);  // across the year
    EXPECT_EQ(PlanCalendar::firstStart(0u), 0u);
}

TEST(PlanCalendar, NoRaceRunsTheWholePlan)
{
    const Calendar c = PlanCalendar::make({20261115u, 0, 10, 6});  // a Sunday
    EXPECT_EQ(c.status, Status::Ok);
    EXPECT_EQ(c.start, 20261116u);
    EXPECT_EQ(c.firstWeek, 0);
    EXPECT_EQ(c.weeks, 10);
}

TEST(PlanCalendar, CountedBackFromTheRace)
{
    // Half on Sunday 7 March 2027, 12 weeks: starts Monday 14 December.
    const Calendar c = PlanCalendar::make({20261115u, 20270307u, 12, 8});
    EXPECT_EQ(c.status, Status::Ok);
    EXPECT_EQ(c.start, 20261214u);
    EXPECT_EQ(c.weeks, 12);
    const DayPlace race = PlanCalendar::locate(c, 20270307u);
    EXPECT_EQ(race.where, DayPlace::Where::In);
    EXPECT_EQ(race.planWeek, 11);
    EXPECT_EQ(race.weekday, 6);
    // A Saturday race is in the same last week.
    const Calendar sat = PlanCalendar::make({20261115u, 20270306u, 12, 8});
    EXPECT_EQ(sat.start, 20261214u);
    EXPECT_EQ(PlanCalendar::locate(sat, 20270306u).planWeek, 11);
    EXPECT_EQ(PlanCalendar::locate(c, 20261213u).where, DayPlace::Where::Before);
    EXPECT_EQ(PlanCalendar::locate(c, 20270308u).where, DayPlace::Where::After);
}

TEST(PlanCalendar, BaseWeeksDroppedWhenTheRaceIsClose)
{
    // 10K on Sunday 10 January 2027, set up on Sunday 15 November: the plan can
    // start on the 16th, which leaves 8 weeks of 10.
    const Calendar c = PlanCalendar::make({20261115u, 20270110u, 10, 6});
    EXPECT_EQ(c.status, Status::Truncated);
    EXPECT_EQ(c.start, 20261116u);
    EXPECT_EQ(c.firstWeek, 2);
    EXPECT_EQ(c.weeks, 8);
    const DayPlace first = PlanCalendar::locate(c, 20261117u);
    EXPECT_EQ(first.planWeek, 2);
    EXPECT_EQ(first.runWeek, 0);
    EXPECT_EQ(PlanCalendar::locate(c, 20270110u).planWeek, 9);
}

TEST(PlanCalendar, TooCloseAndGone)
{
    // Four weeks out for a 10K with a minimum of 6.
    const Calendar close = PlanCalendar::make({20261116u, 20261213u, 10, 6});
    EXPECT_EQ(close.status, Status::TooClose);
    EXPECT_EQ(close.weeks, 4);
    EXPECT_EQ(close.firstWeek, 6);
    EXPECT_TRUE(close.usable());
    // A race this week, set up on a Thursday: the plan cannot start before it.
    EXPECT_EQ(PlanCalendar::make({20261119u, 20261122u, 10, 6}).status, Status::RaceGone);
    // A race in the past.
    EXPECT_EQ(PlanCalendar::make({20261119u, 20261101u, 10, 6}).status, Status::RaceGone);
    // A race this week, set up on a Monday: one week, the race week.
    const Calendar now = PlanCalendar::make({20261116u, 20261122u, 10, 6});
    EXPECT_EQ(now.status, Status::TooClose);
    EXPECT_EQ(now.weeks, 1);
    EXPECT_EQ(now.firstWeek, 9);
}

TEST(PlanCalendar, BadInput)
{
    EXPECT_EQ(PlanCalendar::make({0, 0, 10, 6}).status, Status::BadDate);
    EXPECT_EQ(PlanCalendar::make({20261115u, 20270230u, 10, 6}).status, Status::BadDate);
    EXPECT_EQ(PlanCalendar::make({20261115u, 0, 0, 0}).status, Status::BadDate);
    EXPECT_FALSE(Calendar{}.usable());
    EXPECT_EQ(PlanCalendar::locate(Calendar{}, 20261115u).where, DayPlace::Where::None);
}

TEST(PlanCalendar, DatesOfDays)
{
    const Calendar c = PlanCalendar::make({20261228u, 0, 10, 6});
    EXPECT_EQ(c.start, 20261228u);
    EXPECT_EQ(PlanCalendar::dateOf(c, 0, 6), 20270103u);
    EXPECT_EQ(PlanCalendar::dateOf(c, 9, 6), 20270307u);
}
