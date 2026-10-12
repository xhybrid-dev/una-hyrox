/**
 * Host tests for what the glance shows (GlanceView): the headline from the
 * trend, and the last 14 days as bars ending today (or at the log's last day
 * when the clock is not set). And that the glance, in its own folder, reads
 * the app's SharedData copy of the log.
 */

#include "GlanceView.hpp"

#include "Calendar.hpp"
#include "LogStore.hpp"
#include "Trend.hpp"
#include "support/TreeFileSystem.hpp"

#include <gtest/gtest.h>

using namespace XWeighIn;
using XWeighIn::Glance::kGlanceDays;
using XWeighIn::Glance::makeView;
using XWeighIn::Glance::View;

namespace
{
const int32_t kDay0 = daysFromCivil(2026, 10, 1);

Entry entry(int32_t day, int32_t hundredths, uint16_t minute = 7 * 60)
{
    Entry e;
    e.day        = day;
    e.minute     = minute;
    e.hundredths = hundredths;
    return e;
}

int barsIn(const View& v)
{
    int n = 0;
    for (uint8_t i = 0; i < kGlanceDays; ++i) {
        n += v.weight[i] != 0 ? 1 : 0;
    }
    return n;
}
}  // namespace

TEST(GlanceView, NoLogHasNoData)
{
    Trend t;
    View  v;
    makeView(t, kDay0, true, v);
    EXPECT_FALSE(v.hasData);
    EXPECT_EQ(barsIn(v), 0);
}

TEST(GlanceView, OneWeighInTodayIsTheLastBar)
{
    Trend t;
    t.onEntry(entry(kDay0, 8240));
    View v;
    makeView(t, kDay0, true, v);
    ASSERT_TRUE(v.hasData);
    EXPECT_EQ(v.trend, 8240);
    EXPECT_FALSE(v.hasWeekChange);
    EXPECT_EQ(v.lastDay, kDay0);
    EXPECT_EQ(v.endDay, kDay0);
    EXPECT_EQ(v.weight[kGlanceDays - 1], 8240);
    EXPECT_EQ(barsIn(v), 1);
}

TEST(GlanceView, FourteenDaysEndingToday)
{
    Trend t;
    for (int i = 0; i < 20; ++i) {
        t.onEntry(entry(kDay0 + i, 8400 - 10 * i));
    }
    const int32_t today = kDay0 + 19;
    View v;
    makeView(t, today, true, v);
    EXPECT_EQ(v.endDay, today);
    EXPECT_EQ(barsIn(v), kGlanceDays);
    // Oldest first: day today - 13 is index 0.
    EXPECT_EQ(v.weight[0], 8400 - 10 * 6);
    EXPECT_EQ(v.weight[kGlanceDays - 1], 8400 - 10 * 19);
    // The headline is Home's: the trend and the weekly change at the last day.
    const Summary s = t.summary();
    EXPECT_EQ(v.trend, s.trend);
    EXPECT_TRUE(v.hasWeekChange);
    EXPECT_EQ(v.weekChange, s.weekChange);
}

TEST(GlanceView, DaysSinceTheLastWeighInAreEmpty)
{
    Trend t;
    for (int i = 0; i < 10; ++i) {
        t.onEntry(entry(kDay0 + i, 8200));
    }
    // Last weigh-in kDay0 + 9; today is 3 days later.
    View v;
    makeView(t, kDay0 + 12, true, v);
    EXPECT_EQ(v.endDay, kDay0 + 12);
    EXPECT_EQ(v.lastDay, kDay0 + 9);
    EXPECT_EQ(v.weight[kGlanceDays - 1], 0);
    EXPECT_EQ(v.weight[kGlanceDays - 2], 0);
    EXPECT_EQ(v.weight[kGlanceDays - 3], 0);
    EXPECT_EQ(v.weight[kGlanceDays - 4], 8200);
    // 10 weigh-ins, all inside the 14 days.
    EXPECT_EQ(barsIn(v), 10);
}

TEST(GlanceView, AnOldLogShowsTheTrendAndNoBars)
{
    Trend t;
    for (int i = 0; i < 10; ++i) {
        t.onEntry(entry(kDay0 + i, 8200));
    }
    View v;
    makeView(t, kDay0 + 9 + 20, true, v);
    ASSERT_TRUE(v.hasData);
    EXPECT_EQ(v.trend, 8200);
    EXPECT_EQ(barsIn(v), 0);
}

TEST(GlanceView, WithoutTheClockTheBarsEndAtTheLastDay)
{
    Trend t;
    for (int i = 0; i < 5; ++i) {
        t.onEntry(entry(kDay0 + i, 8200 + i));
    }
    View v;
    makeView(t, kDay0 + 30, false, v);
    EXPECT_EQ(v.endDay, kDay0 + 4);
    EXPECT_EQ(v.weight[kGlanceDays - 1], 8204);
    EXPECT_EQ(barsIn(v), 5);
}

TEST(GlanceView, AClockBehindTheLogEndsAtTheLastDay)
{
    Trend t;
    t.onEntry(entry(kDay0 + 5, 8200));
    View v;
    makeView(t, kDay0, true, v);
    EXPECT_EQ(v.endDay, kDay0 + 5);
    EXPECT_EQ(v.weight[kGlanceDays - 1], 8200);
}

TEST(GlanceView, AMissedDayHasNoBar)
{
    Trend t;
    t.onEntry(entry(kDay0, 8200));
    t.onEntry(entry(kDay0 + 2, 8180));
    View v;
    makeView(t, kDay0 + 2, true, v);
    EXPECT_EQ(v.weight[kGlanceDays - 3], 8200);
    EXPECT_EQ(v.weight[kGlanceDays - 2], 0);   // filled in by the trend, not weighed
    EXPECT_EQ(v.weight[kGlanceDays - 1], 8180);
}

TEST(GlanceView, TheLatestWeighInOfTheDayCounts)
{
    Trend t;
    t.onEntry(entry(kDay0, 8200, 7 * 60));
    t.onEntry(entry(kDay0 + 1, 9230, 7 * 60));       // a typo...
    t.onEntry(entry(kDay0 + 1, 8230, 7 * 60 + 1));   // ...weighed again a minute later
    View v;
    makeView(t, kDay0 + 1, true, v);
    EXPECT_EQ(v.weight[kGlanceDays - 1], 8230);
    EXPECT_EQ(v.trend, t.summary().trend);
}

TEST(GlanceView, TheGlanceReadsTheAppsSharedDataCopy)
{
    // The glance runs in its own folder, confined to it and SharedData: the
    // app's own log is out of reach, its copy is not.
    TreeFileSystem fs("/Apps/XWeighInGlance");
    fs.blockParentAccess(true);
    fs.addFile("/Apps/XWeighIn/weighins.csv", "date,time,kg\n2026-10-12,07:31,99.90\n");
    fs.addFile("/Apps/SharedData/HybridX/weighins.csv",
               "date,time,kg\n2026-10-11,07:30,82.50\n2026-10-12,15:30,82.40\n");

    Trend t;
    const LogStore::ScanResult scan = LogStore::scan(fs, LogStore::kSharedFile, t);
    EXPECT_TRUE(scan.opened);
    EXPECT_EQ(scan.entries, 2u);
    EXPECT_FALSE(LogStore::scan(fs, "../XWeighIn/weighins.csv", t).opened);

    View v;
    makeView(t, daysFromCivil(2026, 10, 12), true, v);
    ASSERT_TRUE(v.hasData);
    EXPECT_EQ(v.weight[kGlanceDays - 2], 8250);
    EXPECT_EQ(v.weight[kGlanceDays - 1], 8240);
}

TEST(GlanceView, NoSharedDataCopyMeansNoData)
{
    TreeFileSystem fs("/Apps/XWeighInGlance");
    Trend t;
    EXPECT_FALSE(LogStore::scan(fs, LogStore::kSharedFile, t).opened);
    View v;
    makeView(t, daysFromCivil(2026, 10, 12), true, v);
    EXPECT_FALSE(v.hasData);
}
