#include "GlanceView.hpp"
#include "TodayBuild.hpp"
#include "data/PlanData.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <string>

using namespace XTraining;
using namespace TrainingGlance;
using State = TodayFile::State;

namespace {

// The 10K plan at level 2 from Monday 4 January 2027, the file as the app writes it on Wednesday 20 January.
constexpr Ymd kStart = 20270104u;
constexpr Ymd kWed   = 20270120u;

struct Plan {
    const PlanDef& def = *PlanData::find(PlanId::TenK);
    PlanState st;
    Paces paces = PaceModel::compute({22 * 60, 0}).paces;
    std::unique_ptr<SDK::Workout::Program> scratch{new SDK::Workout::Program()};
    Plan()
    {
        st.setup.plan = PlanId::TenK;
        st.setup.level = 2;
        st.setup.fiveKSec = 22 * 60;
        st.setup.fiveKDate = kStart;
        st.calendar = PlanCalendar::make({kStart, 0, def.weekCount, def.minWeeks});
    }
    TodayData file(Ymd today)
    {
        TodayData d;
        TodayBuild::fill(st, &paces, false, today, scratch.get(), d);
        return d;
    }
};

}  // namespace

TEST(GlanceView, NoFileOrAFileThatDoesNotRead)
{
    View v;
    makeView(nullptr, kWed, v);
    EXPECT_EQ(v.state, State::NoFile);
    EXPECT_FALSE(v.hasWeek);
    makeView("", 0, kWed, v);
    EXPECT_EQ(v.state, State::NoFile);
    makeView("not json", 8, kWed, v);
    EXPECT_EQ(v.state, State::NoFile);
    const std::string cut = "{\"v\":1,\"plan\":1,\"wks\":10,\"ps\":0,\"s\":20270104,\"e\":2027";
    makeView(cut.data(), cut.size(), kWed, v);
    EXPECT_EQ(v.state, State::NoFile);
    std::string huge(kMaxFileBytes + 1, ' ');
    makeView(huge.data(), huge.size(), kWed, v);
    EXPECT_EQ(v.state, State::NoFile);
}

TEST(GlanceView, ASessionTodayAndItsWeek)
{
    Plan p;
    const TodayData d = p.file(kWed);   // Wednesday of week 3: an easy run
    View v;
    makeView(&d, kWed, v);
    ASSERT_EQ(v.state, State::Session);
    EXPECT_TRUE(v.hasWeek);
    EXPECT_EQ(v.weekNo, 3);
    EXPECT_EQ(v.weeks, p.st.calendar.weeks);
    EXPECT_STREQ(v.what, d.find(kWed)->what);
    // The week of Monday 18 January: Monday rest; Tuesday a session passed with nothing logged;
    // today; then to come; a rest day between.
    EXPECT_EQ(v.pip[0], Pip::Rest);
    EXPECT_EQ(v.pip[1], Pip::Missed) << "Tuesday: passed, nothing logged";
    EXPECT_EQ(v.pip[2], Pip::Today);
    EXPECT_EQ(v.pip[3], Pip::Rest);
    EXPECT_EQ(v.pip[4], Pip::ToCome);
    EXPECT_EQ(v.pip[5], Pip::Rest);
    EXPECT_EQ(v.pip[6], Pip::ToCome);
    // The next session is Friday.
    EXPECT_TRUE(v.hasNext);
    EXPECT_FALSE(v.nextTomorrow);
    EXPECT_EQ(v.nextWeekday, 4);
}

TEST(GlanceView, DoneTodayTurnsItsPipGreenAndTheNextOneIsTomorrow)
{
    Plan p;
    TodayData d = p.file(20270119u);   // Tuesday: intervals; Wednesday is next
    const_cast<TodayDay*>(d.find(20270119u))->status = TodayStatus::PartDone;
    View v;
    makeView(&d, 20270119u, v);
    ASSERT_EQ(v.state, State::Done);
    EXPECT_EQ(v.pip[1], Pip::Done);
    EXPECT_TRUE(v.hasNext);
    EXPECT_TRUE(v.nextTomorrow);
    EXPECT_EQ(v.nextWeekday, 2);
}

TEST(GlanceView, SkippedAndMissedAreAmberAndLoggedDaysBeforeTodayKeepTheirMarks)
{
    Plan p;
    TodayData d = p.file(kWed);
    const_cast<TodayDay*>(d.find(20270119u))->status = TodayStatus::Done;
    const_cast<TodayDay*>(d.find(kWed))->status = TodayStatus::Skipped;
    const_cast<TodayDay*>(d.find(20270122u))->status = TodayStatus::Missed;
    View v;
    makeView(&d, kWed, v);
    EXPECT_EQ(v.state, State::Skipped);
    EXPECT_EQ(v.pip[1], Pip::Done);
    EXPECT_EQ(v.pip[2], Pip::Missed);
    EXPECT_EQ(v.pip[4], Pip::Missed);
}

TEST(GlanceView, ARestDayHasNoSessionOfItsOwn)
{
    Plan p;
    const TodayData d = p.file(20270121u);   // Thursday
    View v;
    makeView(&d, 20270121u, v);
    ASSERT_EQ(v.state, State::Rest);
    EXPECT_EQ(v.kind, Kind::Rest);
    EXPECT_TRUE(v.hasNext);
    EXPECT_TRUE(v.nextTomorrow);
    EXPECT_EQ(v.pip[3], Pip::Rest);
}

TEST(GlanceView, ANewWeekStartsItsPipsAgain)
{
    Plan p;
    const TodayData d = p.file(20270124u);   // Sunday 24 January: the file reaches 7 February
    View sun, mon;
    makeView(&d, 20270124u, sun);
    makeView(&d, 20270125u, mon);   // Monday: the next week, 25 to 31 January
    ASSERT_EQ(sun.state, State::Session);
    ASSERT_EQ(mon.state, State::Rest);
    EXPECT_EQ(sun.weekNo, 3);
    EXPECT_EQ(mon.weekNo, 4);
    EXPECT_EQ(mon.pip[0], Pip::Rest);
    EXPECT_EQ(mon.pip[1], Pip::ToCome);
    EXPECT_EQ(mon.pip[6], Pip::ToCome);
}

TEST(GlanceView, StatesWithNoWeek)
{
    Plan p;
    TodayData d = p.file(kWed);
    View v;
    makeView(&d, 20270204u, v);   // a file from a fortnight ago
    EXPECT_EQ(v.state, State::Stale);
    EXPECT_FALSE(v.hasWeek);
    makeView(&d, 0, v);
    EXPECT_EQ(v.state, State::ClockUnset);
    d.pausedFrom = 20270121u;
    makeView(&d, kWed, v);
    EXPECT_EQ(v.state, State::Session);
    makeView(&d, 20270122u, v);
    EXPECT_EQ(v.state, State::Paused);
    EXPECT_FALSE(v.hasWeek);
    // Before the start: a file written on 31 December.
    Plan q;
    const TodayData early = q.file(20261231u);
    makeView(&early, 20261231u, v);
    EXPECT_EQ(v.state, State::BeforeStart);
    EXPECT_EQ(v.startDate, kStart);
    // After the end.
    const Ymd end = Date::addDays(kStart, q.st.calendar.weeks * 7 - 1);
    const TodayData late = q.file(end);
    makeView(&late, Date::addDays(end, 1), v);
    EXPECT_EQ(v.state, State::Complete);
    // No plan.
    const TodayData none;
    makeView(&none, kWed, v);
    EXPECT_EQ(v.state, State::NoPlan);
}

TEST(GlanceView, FromTheFilesText)
{
    Plan p;
    const TodayData d = p.file(kWed);
    char buf[TodayFile::kMaxJsonBytes];
    const size_t n = TodayFile::write(d, buf, sizeof(buf));
    ASSERT_GT(n, 0u);
    View a, b;
    makeView(&d, kWed, a);
    makeView(buf, n, kWed, b);
    EXPECT_EQ(a.state, b.state);
    EXPECT_STREQ(a.what, b.what);
    EXPECT_STREQ(a.pace, b.pace);
    for (int i = 0; i < 7; ++i) {
        EXPECT_EQ(a.pip[i], b.pip[i]) << i;
    }
}
