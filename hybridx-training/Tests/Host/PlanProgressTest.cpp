#include "PlanProgress.hpp"
#include "data/PlanData.hpp"

#include <gtest/gtest.h>

using namespace XTraining;

namespace {

// The 10K plan at level 2, 5K 22:00, from Monday 4 January 2027, on the default days
// (Tue, Wed, Fri, Sun; the long run Sunday).
PlanState tenKPlan()
{
    PlanState st;
    st.setup.plan = PlanId::TenK;
    st.setup.level = 2;
    st.setup.fiveKSec = 22 * 60;
    st.setup.fiveKRecent = true;
    const PlanDef& def = *PlanData::find(PlanId::TenK);
    st.calendar = PlanCalendar::make({20270104u, 0, def.weekCount, def.minWeeks});
    return st;
}

Paces paces() { return PaceModel::compute({22 * 60, 0}).paces; }

}  // namespace

TEST(PlanProgress, DayMarks)
{
    PlanState st = tenKPlan();
    const PlanDef& def = *PlanData::find(PlanId::TenK);
    DaySession d[7];
    ASSERT_TRUE(PlanSchedule::week(st.setup, def, st.calendar, paces(), 1, d));   // 11-17 Jan
    const Ymd wed = 20270113u;
    st.record({d[1].date, d[1].asRun.kind, DayStatus::Done, 0});        // Tue: done
    st.record({d[4].date, d[4].asRun.kind, DayStatus::PartDone, 0});    // Fri: part done (logged ahead)
    EXPECT_EQ(PlanProgress::mark(d[0], wed, st), DayMark::Rest);
    EXPECT_EQ(PlanProgress::mark(d[1], wed, st), DayMark::Done);
    EXPECT_EQ(PlanProgress::mark(d[2], wed, st), DayMark::Today);
    EXPECT_EQ(PlanProgress::mark(d[4], wed, st), DayMark::PartDone);
    EXPECT_EQ(PlanProgress::mark(d[6], wed, st), DayMark::ToCome);
    // The next Monday: Wednesday's session was never logged.
    EXPECT_EQ(PlanProgress::mark(d[2], 20270118u, st), DayMark::NotLogged);
}

TEST(PlanProgress, WeekToShow)
{
    const PlanState st = tenKPlan();
    EXPECT_EQ(PlanProgress::weekToShow(st.calendar, 20270101u), 0);   // before the start: week 1
    EXPECT_EQ(PlanProgress::weekToShow(st.calendar, 20270104u), 0);
    EXPECT_EQ(PlanProgress::weekToShow(st.calendar, 20270117u), 1);   // a Sunday: still week 2
    EXPECT_EQ(PlanProgress::weekToShow(st.calendar, 20270118u), 2);
    EXPECT_EQ(PlanProgress::weekToShow(st.calendar, 20270601u), 9);   // over: the last week
}

TEST(PlanProgress, Counts)
{
    PlanState st = tenKPlan();
    const PlanDef& def = *PlanData::find(PlanId::TenK);
    // Four runs a week for ten weeks; the race week may differ, so count from the schedule.
    uint16_t total = 0;
    for (uint8_t w = 0; w < st.calendar.weeks; ++w) {
        DaySession d[7];
        ASSERT_TRUE(PlanSchedule::week(st.setup, def, st.calendar, paces(), w, d));
        for (const DaySession& s : d) {
            total = static_cast<uint16_t>(total + (s.asRun.kind != Kind::Rest ? 1 : 0));
        }
    }
    EXPECT_GE(total, 38);

    // Before the start: nothing so far.
    PlanProgress::Counts c = PlanProgress::counts(st, def, paces(), 20270101u);
    EXPECT_EQ(c.total, total);
    EXPECT_EQ(c.soFar, 0);
    EXPECT_EQ(c.done, 0);
    EXPECT_EQ(c.weekSessions, 4);

    // Wednesday of week 2: week 1's four and Tuesday's are past; two done, today done too.
    st.record({20270105u, Kind::Easy, DayStatus::Done, 0});
    st.record({20270108u, Kind::Cruise, DayStatus::PartDone, 0});
    st.record({20270112u, Kind::Hills, DayStatus::Done, 0});
    st.record({20270113u, Kind::Easy, DayStatus::Done, 0});
    st.record({20270110u, Kind::Long, DayStatus::Skipped, 0});   // skipped: not counted
    c = PlanProgress::counts(st, def, paces(), 20270113u);
    EXPECT_EQ(c.soFar, 6);       // four in week 1, Tuesday, and today's (done)
    EXPECT_EQ(c.done, 4);
    EXPECT_EQ(c.weekSessions, 4);
    EXPECT_EQ(c.weekDone, 2);

    // Not today's yet: today does not count as so far.
    PlanState st2 = tenKPlan();
    c = PlanProgress::counts(st2, def, paces(), 20270113u);
    EXPECT_EQ(c.soFar, 5);

    // No plan: nothing.
    PlanState none;
    c = PlanProgress::counts(none, def, paces(), 20270113u);
    EXPECT_EQ(c.total, 0);
}
