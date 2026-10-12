#include "Window.hpp"

#include "Calendar.hpp"
#include "Trend.hpp"

#include <gtest/gtest.h>

using namespace XWeighIn;

namespace
{
const int32_t kDay0 = daysFromCivil(2026, 9, 1);

Entry entry(int32_t day, int32_t hundredths)
{
    Entry e;
    e.day        = day;
    e.minute     = 7 * 60;
    e.hundredths = hundredths;
    return e;
}
}  // namespace

TEST(Window, EmptyWithNoData)
{
    Trend  t;
    Window w;
    w.count = 7;
    fillWindow(t, w);
    EXPECT_EQ(w.count, 0u);
}

TEST(Window, AYoungLogFromItsFirstDay)
{
    Trend t;
    t.onEntry(entry(kDay0, 8000));
    t.onEntry(entry(kDay0 + 3, 8030));   // two missed days
    Window w;
    fillWindow(t, w);
    ASSERT_EQ(w.count, 4u);
    EXPECT_EQ(w.firstDay, kDay0);
    EXPECT_EQ(w.weight[0], 8000);
    EXPECT_EQ(w.weight[1], 0);           // missed: no weigh-in
    EXPECT_EQ(w.weight[2], 0);
    EXPECT_EQ(w.weight[3], 8030);
    DayPoint p;
    ASSERT_TRUE(t.at(kDay0 + 1, p));
    EXPECT_EQ(w.trend[1], p.trend);      // the trend is there on every day
    EXPECT_NE(w.trend[1], 0);
}

TEST(Window, TheLastThirtyDays)
{
    Trend t;
    for (int i = 0; i < 45; ++i) {
        t.onEntry(entry(kDay0 + i, 8000 + i));
    }
    Window w;
    fillWindow(t, w);
    ASSERT_EQ(w.count, kWindowDays);
    EXPECT_EQ(w.firstDay, kDay0 + 44 - (kWindowDays - 1));
    EXPECT_EQ(w.weight[kWindowDays - 1], 8044);
    EXPECT_EQ(w.weight[0], 8000 + 44 - (kWindowDays - 1));
}

TEST(Window, RecentDaysNewestFirstSkippingMissedDays)
{
    Trend t;
    t.onEntry(entry(kDay0, 8000));
    t.onEntry(entry(kDay0 + 2, 8020));
    t.onEntry(entry(kDay0 + 3, 8030));
    Window w;
    fillWindow(t, w);

    RecentDay rows[5];
    ASSERT_EQ(recentDays(w, rows, 5), 3u);
    EXPECT_EQ(rows[0].day, kDay0 + 3);
    EXPECT_EQ(rows[0].hundredths, 8030);
    EXPECT_EQ(rows[1].day, kDay0 + 2);
    EXPECT_EQ(rows[2].day, kDay0);

    ASSERT_EQ(recentDays(w, rows, 2), 2u);   // at most max
    EXPECT_EQ(rows[1].day, kDay0 + 2);
}

TEST(Window, FitsOneMessage)
{
    // With its message header it must stay well inside the kernel's 256-byte
    // pool block (una-sdk Docs/writing-a-clockface.md).
    EXPECT_LE(sizeof(Window), 200u);
}
