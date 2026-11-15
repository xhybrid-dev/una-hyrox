#include "WeekLayout.hpp"
#include "data/PlanData.hpp"

#include <gtest/gtest.h>

using namespace XTraining;

namespace {

bool hardRole(Role r)
{
    return r == Role::Quality1 || r == Role::Quality2 || r == Role::KeyOnly || r == Role::Long;
}

DayMask mask(std::initializer_list<int> days)
{
    DayMask m = 0;
    for (int d : days) {
        m = static_cast<DayMask>(m | (1u << d));
    }
    return m;
}

constexpr int Mon = 0, Tue = 1, Wed = 2, Thu = 3, Fri = 4, Sat = 5, Sun = 6;

}  // namespace

TEST(WeekLayout, TheDefaultGivesThePlansTables)
{
    const Layout l = WeekLayout::place(WeekLayout::kDefaultDays, WeekLayout::kDefaultLongDay);
    ASSERT_TRUE(l.ok);
    EXPECT_EQ(l.role[Mon], Role::Rest);
    EXPECT_EQ(l.role[Tue], Role::Quality1);
    EXPECT_EQ(l.role[Wed], Role::Easy);
    EXPECT_EQ(l.role[Thu], Role::Rest);
    EXPECT_EQ(l.role[Fri], Role::Quality2);
    EXPECT_EQ(l.role[Sat], Role::Rest);
    EXPECT_EQ(l.role[Sun], Role::Long);
}

TEST(WeekLayout, EveryChoiceOfDays)
{
    // PLANS.md 4.2, checked on every choice of 3 to 6 days and long-run day.
    int cases = 0, pairs = 0, singles = 0, beforeLong = 0;
    for (unsigned m = 0; m < 128; ++m) {
        const DayMask days = static_cast<DayMask>(m);
        const uint8_t n = WeekLayout::dayCount(days);
        for (uint8_t L = 0; L < 7; ++L) {
            const bool expectOk = n >= 3 && n <= 6 && ((days >> L) & 1u);
            const Layout l = WeekLayout::place(days, L);
            ASSERT_EQ(l.ok, expectOk) << m << " " << int(L);
            ASSERT_EQ(WeekLayout::valid(days, L), expectOk);
            if (!expectOk) {
                continue;
            }
            ++cases;
            SCOPED_TRACE(testing::Message() << "days " << m << " long " << int(L));
            int q = 0, single = 0, easy = 0, longs = 0, runs = 0;
            for (uint8_t d = 0; d < 7; ++d) {
                const Role r = l.role[d];
                const bool run = (days >> d) & 1u;
                ASSERT_EQ(r != Role::Rest, run) << "a run day for every chosen day, and only those";
                runs += run;
                q += r == Role::Quality1 || r == Role::Quality2;
                single += r == Role::KeyOnly;
                easy += r == Role::Easy;
                longs += r == Role::Long;
            }
            ASSERT_EQ(runs, n);
            ASSERT_EQ(longs, 1);
            ASSERT_EQ(l.role[L], Role::Long);
            ASSERT_TRUE((q == 2 && single == 0) || (q == 0 && single == 1)) << "one or two quality days";
            ASSERT_EQ(easy, 1);
            if (n == 3) {
                ASSERT_EQ(single, 1);
            }
            q == 2 ? ++pairs : ++singles;
            // Never quality the day after the long run.
            ASSERT_FALSE(hardRole(l.role[(L + 1) % 7]));
            // Two quality days are never next to each other, nor next to L.
            for (uint8_t d = 0; d < 7; ++d) {
                const Role r = l.role[d];
                const Role next = l.role[(d + 1) % 7];
                if (q == 2 && hardRole(r) && hardRole(next)) {
                    ADD_FAILURE() << "hard days " << int(d) << " and " << (d + 1) % 7;
                }
            }
            if (single == 1 && l.role[(L + 6) % 7] == Role::KeyOnly) {
                ++beforeLong;  // only as a last resort
            }
        }
    }
    EXPECT_EQ(cases, 392);
    RecordProperty("pairs", pairs);
    RecordProperty("singles", singles);
    std::printf("[ layout ] %d cases: %d with two quality days, %d with one (%d of those the day before the long run)\n",
                cases, pairs, singles, beforeLong);
}

TEST(WeekLayout, ThreeDays)
{
    // Tue, Thu, Sun: the key on Thursday (furthest from Sunday), easy on Tuesday.
    Layout l = WeekLayout::place(mask({Tue, Thu, Sun}), Sun);
    ASSERT_TRUE(l.ok);
    EXPECT_EQ(l.role[Thu], Role::KeyOnly);
    EXPECT_EQ(l.role[Tue], Role::Easy);
    // Sat, Sun, Mon: only Saturday can hold quality, the day before L.
    l = WeekLayout::place(mask({Sat, Sun, Mon}), Sun);
    ASSERT_TRUE(l.ok);
    EXPECT_EQ(l.role[Sat], Role::KeyOnly);
    EXPECT_EQ(l.role[Mon], Role::Easy);
}

TEST(WeekLayout, FourDaysWithNoPair)
{
    // Thu, Fri, Sat, Sun with Sunday long: Friday and Saturday sit next to the
    // long run or each other, so one quality day.
    const Layout l = WeekLayout::place(mask({Thu, Fri, Sat, Sun}), Sun);
    ASSERT_TRUE(l.ok);
    EXPECT_EQ(l.role[Thu], Role::KeyOnly);
    int demoted = 0;
    for (Role r : l.role) {
        demoted += r == Role::Demoted;
    }
    EXPECT_EQ(demoted, 1);
}

TEST(WeekLayout, SessionsFromAWeek)
{
    const PlanDef* half = PlanData::find(PlanId::Half);
    const Week& wk7 = half->weeks[6];  // INT 6x3 | E+S 45 | *RPHM 3x10 | L 105
    Session s[7];

    WeekLayout::sessions(wk7, WeekLayout::place(WeekLayout::kDefaultDays, Sun), s);
    EXPECT_EQ(s[Tue].kind, Kind::Intervals);
    EXPECT_EQ(s[Wed].kind, Kind::EasyStrides);
    EXPECT_EQ(s[Fri].kind, Kind::PaceHalf);
    EXPECT_EQ(s[Sun].kind, Kind::Long);
    EXPECT_EQ(s[Mon].kind, Kind::Rest);

    // Three days: the key (half pace) on the quality day, the easy run with strides.
    WeekLayout::sessions(wk7, WeekLayout::place(mask({Tue, Thu, Sun}), Sun), s);
    EXPECT_EQ(s[Thu].kind, Kind::PaceHalf);
    EXPECT_EQ(s[Tue].kind, Kind::EasyStrides);
    EXPECT_EQ(s[Tue].a, 45);

    // Five days: an extra easy run.
    WeekLayout::sessions(wk7, WeekLayout::place(mask({Tue, Wed, Fri, Sat, Sun}), Sun), s);
    EXPECT_EQ(s[Sat].kind, Kind::Easy);
    EXPECT_EQ(s[Sat].a, WeekLayout::kExtraEasyMinutes);

    // One quality day of four: the other quality becomes easy with strides.
    WeekLayout::sessions(wk7, WeekLayout::place(mask({Thu, Fri, Sat, Sun}), Sun), s);
    EXPECT_EQ(s[Thu].kind, Kind::PaceHalf);
    int easyStrides = 0;
    for (const Session& x : s) {
        easyStrides += x.kind == Kind::EasyStrides;
    }
    EXPECT_EQ(easyStrides, 2);  // the easy slot and the demoted intervals
}

TEST(WeekLayout, RaceWeekOnAnotherDay)
{
    // A Saturday race with Sunday long runs: the race week is placed round Saturday.
    const PlanDef* tenK = PlanData::find(PlanId::TenK);
    const Week& race = tenK->weeks[tenK->weekCount - 1];
    const DayMask days = static_cast<DayMask>(WeekLayout::kDefaultDays | (1u << Sat));
    const Layout l = WeekLayout::place(days, Sat, true);
    ASSERT_TRUE(l.ok);
    Session s[7];
    WeekLayout::sessions(race, l, s);
    EXPECT_EQ(s[Sat].kind, Kind::Race);
    EXPECT_NE(s[Sun].kind, Kind::Race);
}
