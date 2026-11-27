#include <gtest/gtest.h>

#include "ActivityLevel.hpp"
#include "PretendWatch.hpp"
#include "WeekView.hpp"

using namespace ClockProbe;
using HybridXLink::Sport;

namespace
{
ScanData scanOf(TreeFileSystem& fs)
{
    static char scratch[kScratchBytes];
    ScanData d;
    runScan(fs, scratch, sizeof(scratch), d, ScanSink {});
    return d;
}
} // namespace

TEST(WeekView, MondayIsDayZero)
{
    EXPECT_EQ(weekdayOf(0), 3);                       // 1 January 1970 was a Thursday
    EXPECT_EQ(weekdayOf(pretend::thursday()), 3);     // 26 November 2026 is a Thursday
    EXPECT_EQ(weekdayOf(pretend::thursday() - 3), 0);
    EXPECT_EQ(weekdayOf(pretend::thursday() + 3), 6);
    EXPECT_EQ(weekdayOf(-1), 2);                      // a Wednesday: the modulus must not go negative
}

TEST(WeekView, AWeekOfSessionsLandsOnTheRightDays)
{
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    const WeekView w = makeWeek(scanOf(fs), pretend::thursday());

    EXPECT_EQ(w.today, 3);
    EXPECT_EQ(w.monday, pretend::thursday() - 3);
    EXPECT_EQ(w.day[0].sessions, 2);   // Monday: a run and a ride
    EXPECT_EQ(w.day[0].minutes, 52 + 16);
    EXPECT_EQ(w.day[0].load, 120 + 39);
    EXPECT_EQ(w.day[1].sessions, 2);   // Tuesday: a trail run and a workout
    EXPECT_EQ(w.day[2].sessions, 1);   // Wednesday: a race
    EXPECT_EQ(w.day[3].sessions, 2);   // Thursday: a training run and a run
    EXPECT_EQ(w.day[4].sessions, 0);
    EXPECT_EQ(w.day[5].sessions, 0);
    EXPECT_EQ(w.day[6].sessions, 0);
}

TEST(WeekView, ThePlaceholderGradingIsNoneLowHigh)
{
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    const WeekView w = makeWeek(scanOf(fs), pretend::thursday());
    EXPECT_EQ(w.day[0].level, Level::High);   // 120 + 39 = 159 points
    EXPECT_EQ(w.day[1].level, Level::Low);    // a trail run of load 90 and a workout with none: 90 points
    EXPECT_EQ(w.day[2].level, Level::High);   // a race of load 170
    EXPECT_EQ(w.day[3].level, Level::High);   // 80 + 60 = 140 points
    EXPECT_EQ(w.day[4].level, Level::None);
}

TEST(WeekView, ASessionWithNoLoadCountsItsMinutesAtTwoPointsEach)
{
    EXPECT_EQ(dayScore(0, 40), 80u);
    EXPECT_EQ(levelFor(1, 0, 40), Level::Low);    // 80 < 100
    EXPECT_EQ(levelFor(1, 0, 50), Level::High);   // 100
    EXPECT_EQ(levelFor(1, 39, 16), Level::Low);   // Jon's 16-minute ride
    EXPECT_EQ(levelFor(1, 99, 200), Level::Low);  // load wins over minutes when there is one
    EXPECT_EQ(levelFor(1, 100, 1), Level::High);
    EXPECT_EQ(levelFor(0, 500, 500), Level::None);
}

TEST(WeekView, SessionsOutsideThisWeekAreIgnored)
{
    TreeFileSystem fs(pretend::kSandbox);
    const int32_t thu = pretend::thursday();
    fs.addFile("/Apps/SharedData/HybridX/act_run.json",
               pretend::actText("HybridXRun", {pretend::entry(thu - 4, 7, 60, Sport::Run, 90),    // last Sunday
                                               pretend::entry(thu + 4, 7, 60, Sport::Run, 90),    // next Monday
                                               pretend::entry(thu - 3, 7, 60, Sport::Run, 90)}),  // this Monday
               1);
    const WeekView w = makeWeek(scanOf(fs), thu);
    int total = 0;
    for (const DayStat& d : w.day) {
        total += d.sessions;
    }
    EXPECT_EQ(total, 1);
    EXPECT_EQ(w.day[0].sessions, 1);
}

TEST(WeekView, TheLocalDayUsesTheTimeZoneOffset)
{
    // 23:30 UTC on Sunday is already Monday in UTC+1.
    TreeFileSystem fs(pretend::kSandbox);
    const int32_t thu = pretend::thursday();
    const int32_t sundayBefore = thu - 4;
    const uint32_t utc = static_cast<uint32_t>(sundayBefore) * 86400u + 23u * 3600u + 30u * 60u;
    fs.addFile("/Apps/SharedData/HybridX/act_run.json",
               pretend::actText("HybridXRun", {HybridXLink::makeEntry(utc, 60, 3600, 0.f, 140.f, 160.f, Sport::Run, 80)}),
               1);
    const WeekView w = makeWeek(scanOf(fs), thu);
    EXPECT_EQ(w.day[0].sessions, 1) << "Monday 00:30 local";
}

TEST(WeekView, TwoAppsOnTheSameDayShowInTheirBits)
{
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    const WeekView w = makeWeek(scanOf(fs), pretend::thursday());
    EXPECT_EQ(w.day[0].apps, (1u << 0) | (1u << 3));   // run and ride
}

TEST(WeekView, AnUnsetClockGivesAnEmptyWeek)
{
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    const WeekView w = makeWeek(scanOf(fs), -1);
    EXPECT_EQ(w.today, -1);
    for (const DayStat& d : w.day) {
        EXPECT_EQ(d.sessions, 0);
        EXPECT_EQ(d.level, Level::None);
    }
}
