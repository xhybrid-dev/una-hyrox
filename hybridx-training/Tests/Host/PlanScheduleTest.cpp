#include "PlanSchedule.hpp"
#include "data/PlanData.hpp"

#include <gtest/gtest.h>

using namespace XTraining;

namespace {

PlanSetup setupFor(PlanId plan)
{
    PlanSetup s;
    s.plan = plan;
    s.level = 2;
    s.fiveKSec = 22 * 60;
    return s;
}

}  // namespace

TEST(PlanSchedule, TheDefaultWeekIsThePlanFilesWeek)
{
    const PlanDef& half = *PlanData::find(PlanId::Half);
    const PlanSetup st = setupFor(PlanId::Half);
    const Calendar cal = PlanCalendar::make({20270104u, 0, half.weekCount, half.minWeeks});
    const Paces paces = PaceModel::compute({st.fiveKSec, 0}).paces;
    DaySession d[7];
    ASSERT_TRUE(PlanSchedule::week(st, half, cal, paces, 6, d));  // week 7
    EXPECT_EQ(d[1].asRun.kind, Kind::Intervals);
    EXPECT_FALSE(d[1].key);
    EXPECT_EQ(d[4].asRun.kind, Kind::PaceHalf);
    EXPECT_TRUE(d[4].key);
    EXPECT_EQ(d[6].asRun.kind, Kind::Long);
    EXPECT_EQ(d[6].asRun.a, 105);
    EXPECT_EQ(d[0].asRun.kind, Kind::Rest);
    EXPECT_EQ(d[1].date, 20270216u);
    DaySession one;
    ASSERT_TRUE(PlanSchedule::day(st, half, cal, paces, 20270219u, one));
    EXPECT_EQ(one.asRun.kind, Kind::PaceHalf);
    EXPECT_FALSE(PlanSchedule::day(st, half, cal, paces, 20270103u, one));  // before the start
}

TEST(PlanSchedule, ANotRecentFiveKStartsWithATimeTrial)
{
    const PlanDef& tenK = *PlanData::find(PlanId::TenK);
    const Calendar cal = PlanCalendar::make({20270104u, 0, tenK.weekCount, tenK.minWeeks});
    EXPECT_EQ(&PlanSchedule::weekFor(tenK, cal, 0, false), tenK.trialWeek);
    EXPECT_EQ(&PlanSchedule::weekFor(tenK, cal, 0, true), &tenK.weeks[0]);
    EXPECT_EQ(&PlanSchedule::weekFor(tenK, cal, 1, false), &tenK.weeks[1]);
    // The VO2max block opens with its own time trial, or its "recent" week.
    const PlanDef& vo2 = *PlanData::find(PlanId::Vo2max);
    const Calendar vcal = PlanCalendar::make({20270104u, 0, vo2.weekCount, vo2.minWeeks});
    EXPECT_EQ(&PlanSchedule::weekFor(vo2, vcal, 0, false), &vo2.weeks[0]);
    EXPECT_EQ(&PlanSchedule::weekFor(vo2, vcal, 0, true), vo2.recentWeek);
}

TEST(PlanSchedule, TheRaceWeekIsPlacedRoundTheRace)
{
    // A Saturday 10K, Sunday long runs: the race lands on the Saturday.
    const PlanDef& tenK = *PlanData::find(PlanId::TenK);
    PlanSetup st = setupFor(PlanId::TenK);
    st.raceDate = 20270313u;  // a Saturday
    const Calendar cal = PlanCalendar::make({20270104u, st.raceDate, tenK.weekCount, tenK.minWeeks});
    ASSERT_EQ(cal.status, Calendar::Status::Ok);
    const Paces paces = PaceModel::compute({st.fiveKSec, 0}).paces;
    DaySession race;
    ASSERT_TRUE(PlanSchedule::day(st, tenK, cal, paces, st.raceDate, race));
    EXPECT_EQ(race.asRun.kind, Kind::Race);
    DaySession sunday;
    ASSERT_TRUE(PlanSchedule::day(st, tenK, cal, paces, 20270314u, sunday));
    EXPECT_NE(sunday.asRun.kind, Kind::Race);
    // The race week is never replaced by the time-trial week, even when it is run week 0.
    st.fiveKRecent = false;
    const Calendar last = PlanCalendar::make({20270308u, st.raceDate, tenK.weekCount, tenK.minWeeks});
    ASSERT_EQ(last.weeks, 1);
    EXPECT_EQ(&PlanSchedule::weekFor(tenK, last, 0, false), &tenK.weeks[tenK.weekCount - 1]);
}

TEST(PlanSchedule, ATruncatedPlanRunsItsLaterWeeks)
{
    const PlanDef& half = *PlanData::find(PlanId::Half);
    PlanSetup st = setupFor(PlanId::Half);
    st.raceDate = 20270307u;
    const Calendar cal = PlanCalendar::make({20270104u, st.raceDate, half.weekCount, half.minWeeks});
    ASSERT_EQ(cal.status, Calendar::Status::Truncated);
    EXPECT_EQ(cal.weeks, 9);
    EXPECT_EQ(cal.firstWeek, 3);
    const Paces paces = PaceModel::compute({st.fiveKSec, 0}).paces;
    DaySession d[7];
    ASSERT_TRUE(PlanSchedule::week(st, half, cal, paces, 0, d));
    EXPECT_EQ(d[0].planWeek, 3);
    EXPECT_EQ(d[6].asRun.kind, Kind::TimeTrial5k);  // the plan's week 4
    DaySession race;
    ASSERT_TRUE(PlanSchedule::day(st, half, cal, paces, st.raceDate, race));
    EXPECT_EQ(race.asRun.kind, Kind::Race);
}
