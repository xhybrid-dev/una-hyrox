#include "Trend.hpp"

#include "Calendar.hpp"

#include <gtest/gtest.h>

using namespace XWeighIn;

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

int32_t trendAt(const Trend& t, int32_t day)
{
    DayPoint p;
    EXPECT_TRUE(t.at(day, p)) << day;
    return p.trend;
}
}  // namespace

TEST(Trend, EmptyHasNoData)
{
    Trend t;
    const Summary s = t.summary();
    EXPECT_EQ(s.days, 0u);
    EXPECT_EQ(s.latest, 0);
    EXPECT_FALSE(s.hasWeekChange);
    DayPoint p;
    EXPECT_FALSE(t.at(kDay0, p));
}

TEST(Trend, FirstWeighInStartsTheTrend)
{
    Trend t;
    t.onEntry(entry(kDay0, 8200));
    const Summary s = t.summary();
    EXPECT_EQ(s.days, 1u);
    EXPECT_EQ(s.firstDay, kDay0);
    EXPECT_EQ(s.lastDay, kDay0);
    EXPECT_EQ(s.latest, 8200);
    EXPECT_EQ(s.trend, 8200);
    EXPECT_FALSE(s.hasWeekChange);
}

TEST(Trend, SteadyWeightStaysPut)
{
    Trend t;
    for (int i = 0; i < 30; ++i) {
        t.onEntry(entry(kDay0 + i, 8200));
    }
    const Summary s = t.summary();
    EXPECT_EQ(s.trend, 8200);
    EXPECT_TRUE(s.hasWeekChange);
    EXPECT_EQ(s.weekChange, 0);
}

TEST(Trend, MovesTenPercentADay)
{
    // A 1 kg step: the trend closes 10% of the gap each day, so 0.10 kg after
    // one day and 1 - 0.9^7 = 52.2% after seven.
    Trend t;
    t.onEntry(entry(kDay0, 8200));
    for (int i = 1; i <= 7; ++i) {
        t.onEntry(entry(kDay0 + i, 8300));
    }
    EXPECT_EQ(trendAt(t, kDay0 + 1), 8210);
    EXPECT_EQ(trendAt(t, kDay0 + 7), 8252);
    EXPECT_EQ(t.summary().trend, 8252);
    EXPECT_EQ(t.summary().latest, 8300);
}

TEST(Trend, TheLatestWeighInOfADayCounts)
{
    Trend t;
    t.onEntry(entry(kDay0, 8200));
    t.onEntry(entry(kDay0 + 1, 8300, 7 * 60));
    EXPECT_EQ(t.summary().trend, 8210);
    t.onEntry(entry(kDay0 + 1, 8250, 7 * 60 + 2));   // weighed again, a typo fixed
    const Summary s = t.summary();
    EXPECT_EQ(s.days, 2u);
    EXPECT_EQ(s.latest, 8250);
    EXPECT_EQ(s.trend, 8205);
    EXPECT_EQ(trendAt(t, kDay0 + 1), 8205);
}

TEST(Trend, TheLatestCountsOnTheFirstDayToo)
{
    Trend t;
    t.onEntry(entry(kDay0, 8200));
    t.onEntry(entry(kDay0, 8000, 8 * 60));
    const Summary s = t.summary();
    EXPECT_EQ(s.days, 1u);
    EXPECT_EQ(s.latest, 8000);
    EXPECT_EQ(s.trend, 8000);
    // And the next day steps from 80.00, not 82.00.
    t.onEntry(entry(kDay0 + 1, 8100));
    EXPECT_EQ(t.summary().trend, 8010);
}

TEST(Trend, ReplacingAfterAGapRedoesTheFilledDays)
{
    Trend t;
    t.onEntry(entry(kDay0, 8000));
    t.onEntry(entry(kDay0 + 4, 9000));
    t.onEntry(entry(kDay0 + 4, 8000));   // the 90.00 was a slip
    EXPECT_EQ(t.summary().trend, 8000);
    for (int i = 1; i <= 4; ++i) {
        EXPECT_EQ(trendAt(t, kDay0 + i), 8000) << i;
    }
}

TEST(Trend, AnEarlierDayIsSkipped)
{
    Trend t;
    t.onEntry(entry(kDay0, 8200));
    t.onEntry(entry(kDay0 + 2, 8100));
    t.onEntry(entry(kDay0 + 1, 9999));   // the clock went back
    const Summary s = t.summary();
    EXPECT_EQ(s.skipped, 1u);
    EXPECT_EQ(s.days, 2u);
    EXPECT_EQ(s.lastDay, kDay0 + 2);
    EXPECT_EQ(s.latest, 8100);
}

TEST(Trend, InvalidEntriesAreSkipped)
{
    Trend t;
    t.onEntry(entry(kDay0, 0));
    t.onEntry(entry(kDay0, 50001));
    EXPECT_EQ(t.summary().skipped, 2u);
    EXPECT_EQ(t.summary().days, 0u);
}

TEST(Trend, MissedDaysFollowAStraightLine)
{
    // Weighing every other day on a steady slope gives the same trend as
    // weighing every day: the missed days take the line between the two.
    Trend daily;
    Trend alternate;
    for (int i = 0; i <= 20; ++i) {
        const int32_t w = 8500 - 10 * i;   // -0.1 kg a day
        daily.onEntry(entry(kDay0 + i, w));
        if (i % 2 == 0) {
            alternate.onEntry(entry(kDay0 + i, w));
        }
    }
    for (int i = 0; i <= 20; ++i) {
        EXPECT_EQ(trendAt(alternate, kDay0 + i), trendAt(daily, kDay0 + i)) << i;
    }
    EXPECT_EQ(alternate.summary().days, 11u);
    EXPECT_EQ(daily.summary().days, 21u);
}

TEST(Trend, FilledDaysHaveNoWeighIn)
{
    Trend t;
    t.onEntry(entry(kDay0, 8000));
    t.onEntry(entry(kDay0 + 3, 8300));
    DayPoint p;
    ASSERT_TRUE(t.at(kDay0 + 1, p));
    EXPECT_EQ(p.hundredths, 0);
    ASSERT_TRUE(t.at(kDay0 + 3, p));
    EXPECT_EQ(p.hundredths, 8300);
}

TEST(Trend, AWeekOffNeitherFreezesNorJumps)
{
    // A 0.7 kg drop over a missed week moves the trend about as a daily
    // record of the same drop would, not 10% in one go.
    Trend gap;
    Trend daily;
    gap.onEntry(entry(kDay0, 8000));
    daily.onEntry(entry(kDay0, 8000));
    for (int i = 1; i <= 7; ++i) {
        daily.onEntry(entry(kDay0 + i, 8000 - 10 * i));
    }
    gap.onEntry(entry(kDay0 + 7, 7930));
    EXPECT_EQ(gap.summary().trend, daily.summary().trend);
}

TEST(Trend, WeeklyChange)
{
    Trend t;
    for (int i = 0; i < 7; ++i) {
        t.onEntry(entry(kDay0 + i, 8500 - 10 * i));
    }
    EXPECT_FALSE(t.summary().hasWeekChange);   // spans 6 days
    t.onEntry(entry(kDay0 + 7, 8430));
    Summary s = t.summary();
    ASSERT_TRUE(s.hasWeekChange);
    EXPECT_EQ(s.weekChange, trendAt(t, kDay0 + 7) - trendAt(t, kDay0));
    EXPECT_LT(s.weekChange, 0);

    for (int i = 8; i < 60; ++i) {
        t.onEntry(entry(kDay0 + i, 8500 - 10 * i));
    }
    s = t.summary();
    // Long into a steady -0.1 kg a day, the trend lags a fixed amount behind
    // and falls at the same rate: -0.70 kg a week.
    EXPECT_EQ(s.weekChange, -70);
}

TEST(Trend, HistoryKeepsTheLastNinetyDays)
{
    Trend t;
    for (int i = 0; i < 120; ++i) {
        t.onEntry(entry(kDay0 + i, 8000 + i));
    }
    const int32_t last = kDay0 + 119;
    DayPoint p;
    EXPECT_TRUE(t.at(last, p));
    EXPECT_EQ(p.hundredths, 8119);
    EXPECT_TRUE(t.at(last - (kHistoryDays - 1), p));
    EXPECT_EQ(p.day, last - (kHistoryDays - 1));
    EXPECT_FALSE(t.at(last - kHistoryDays, p));
    EXPECT_FALSE(t.at(last + 1, p));
    EXPECT_FALSE(t.at(kDay0, p));
}

TEST(Trend, ResetForgetsEverything)
{
    Trend t;
    t.onEntry(entry(kDay0, 8200));
    t.reset();
    EXPECT_EQ(t.summary().days, 0u);
    t.onEntry(entry(kDay0 - 5, 7000));   // earlier than before: fine after a reset
    EXPECT_EQ(t.summary().latest, 7000);
    EXPECT_EQ(t.summary().skipped, 0u);
}
