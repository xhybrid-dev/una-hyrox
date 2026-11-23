/**
 * Host tests for the training load graphs' history (NOTES "S8"): LoadTrend, recorded as days
 * leave the load's open window, LoadHistory::series, which joins it to the open days, and the
 * state file's optional "H".
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "LoadHistory.hpp"
#include "StateCodec.hpp"
#include "StreakModel.hpp"
#include "WeekMath.hpp"

using namespace Streak;

namespace
{

constexpr int32_t kDay0 = 20400;

/// A pattern with some shape: three weeks building, a hard week, an easy one, repeated.
uint16_t loadOf(int32_t d)
{
    const int32_t week = (d / 7) % 5;
    const bool    rest = d % 7 == 6;
    if (rest) {
        return 0;
    }
    return static_cast<uint16_t>(week == 3 ? 110 : (week == 4 ? 25 : 50 + 10 * week));
}

/// A history kept day by day for @p days, as the model keeps it (advance, then the day's load).
void live(LoadState& s, LoadTrend& t, int32_t days)
{
    LoadHistory h(s, &t);
    for (int32_t d = 0; d < days; ++d) {
        h.advanceTo(kDay0 + d);
        h.add(kDay0 + d, loadOf(d));
    }
}

int32_t day(int y, int m, int d)
{
    return WeekMath::daysFromCivil(y, static_cast<uint32_t>(m), static_cast<uint32_t>(d));
}

Found session(int32_t d, uint16_t load)
{
    Found f;
    std::strncpy(f.app, "HybridXRun", sizeof(f.app) - 1);
    f.appKey     = ActivityScanner::appKey("HybridXRun");
    f.localDay   = d;
    f.localStart = static_cast<uint32_t>(d) * 86400u + 7u * 3600u;
    f.kind       = Kind::Run;
    f.minutes    = 40;
    f.load       = load;
    return f;
}

} // namespace

TEST(LoadTrend, NothingRecordedUntilTheDaysLeaveTheWindow)
{
    LoadState s;
    LoadTrend t;
    live(s, t, kOpenDays);   // the first two weeks: all still open
    EXPECT_EQ(t.count, 0);
    LoadHistory h(s, &t);
    h.advanceTo(kDay0 + kOpenDays);   // a day later: the first day leaves the window
    ASSERT_EQ(t.count, 1);
    EXPECT_EQ(t.lastDay, kDay0);   // the first day of the athlete's own, not a filler day before it
    EXPECT_EQ(t.load[0], loadOf(0));
}

TEST(LoadTrend, EachSettledDayIsItsFiguresAtTheEndOfThatDay)
{
    LoadState s;
    LoadTrend t;
    live(s, t, 60);
    ASSERT_EQ(t.count, 60 - kOpenDays);
    EXPECT_EQ(t.lastDay, s.openStart - 1);
    // Replay every settled day from scratch and compare: the history is the model, a day at a time.
    for (int32_t n = kOpenDays + 1; n <= 60; ++n) {
        LoadState s2;
        LoadHistory h2(s2);
        for (int32_t d = 0; d < n; ++d) {
            h2.advanceTo(kDay0 + d);
            h2.add(kDay0 + d, loadOf(d));
        }
        const int32_t settled = s2.openStart - 1;   // the newest settled day after n days
        const uint8_t i       = static_cast<uint8_t>(settled - kDay0);
        EXPECT_EQ(t.fitnessX10[i], LoadTrend::tenths(s2.fitnessX100)) << n;
        EXPECT_EQ(t.fatigueX10[i], LoadTrend::tenths(s2.fatigueX100)) << n;
    }
}

TEST(LoadTrend, KeepsTwelveWeeks)
{
    LoadState s;
    LoadTrend t;
    live(s, t, 200);
    EXPECT_EQ(t.count, kTrendDays);
    EXPECT_EQ(t.lastDay, s.openStart - 1);
    EXPECT_EQ(t.load[kTrendDays - 1], loadOf(t.lastDay - kDay0));
    EXPECT_EQ(t.load[0], loadOf(t.lastDay - (kTrendDays - 1) - kDay0));
}

TEST(LoadTrend, AGapStartsAgainAndAYearsGapClearsIt)
{
    LoadTrend t;
    t.record(100, 1000, 2000, 5);
    t.record(101, 1000, 2000, 5);
    t.record(101, 9999, 9999, 9);   // the same day again: kept as it was
    EXPECT_EQ(t.count, 2);
    EXPECT_EQ(t.load[1], 5);
    t.record(105, 1000, 2000, 7);   // a gap cannot be filled in
    EXPECT_EQ(t.count, 1);
    EXPECT_EQ(t.lastDay, 105);

    LoadState s;
    LoadTrend tr;
    live(s, tr, 40);
    ASSERT_GT(tr.count, 0);
    LoadHistory h(s, &tr);
    h.advanceTo(kDay0 + 40 + LoadHistory::kMaxFold + 10);
    EXPECT_EQ(tr.count, 0);
}

TEST(LoadTrend, TenthsRoundAndHold)
{
    EXPECT_EQ(LoadTrend::tenths(0), 0);
    EXPECT_EQ(LoadTrend::tenths(1234), 123);
    EXPECT_EQ(LoadTrend::tenths(1235), 124);
    EXPECT_EQ(LoadTrend::tenths(0xFFFFFFFFu), 0xFFFF);
}

TEST(LoadSeries, EndsTodayWithTodaysFigures)
{
    LoadState s;
    LoadTrend t;
    live(s, t, 150);
    const int32_t today = kDay0 + 149;
    LoadHistory   h(s);
    LoadSeries    series;
    h.series(t, today, series);
    ASSERT_EQ(series.count, kTrendDays);
    EXPECT_EQ(series.lastDay, today);
    const LoadFigures f = h.figures(today);
    EXPECT_EQ(series.fitnessX10[kTrendDays - 1], LoadTrend::tenths(f.fitnessX100));
    EXPECT_EQ(series.fatigueX10[kTrendDays - 1], LoadTrend::tenths(f.fatigueX100));
    // Every day of it is that day's figures and load.
    for (uint8_t i = 0; i < kTrendDays; ++i) {
        const int32_t d = today - (kTrendDays - 1) + i;
        EXPECT_EQ(series.load[i], loadOf(d - kDay0)) << int(i);
    }
    for (int32_t back = 0; back < kOpenDays; ++back) {
        const uint8_t     i = static_cast<uint8_t>(kTrendDays - 1 - back);
        const LoadFigures g = h.figures(today - back);
        EXPECT_EQ(series.fitnessX10[i], LoadTrend::tenths(g.fitnessX100)) << back;
    }
}

TEST(LoadSeries, ANewAthleteHasTheirDaysOnly)
{
    LoadState s;
    LoadTrend t;
    live(s, t, 5);
    LoadHistory h(s);
    LoadSeries  series;
    h.series(t, kDay0 + 4, series);
    EXPECT_EQ(series.count, 5);   // from the first day, not the window's filler days
    EXPECT_EQ(series.historyDays, 5);
    EXPECT_EQ(series.load[0], loadOf(0));
}

TEST(LoadSeries, AnUpdatedAthleteStartsWithTheWindow)
{
    // Months of load from before the history was kept (an older file, no "H"): the graph starts
    // with the two weeks still open.
    LoadState s;
    LoadTrend t;
    live(s, t, 120);
    t.clear();
    LoadHistory h(s);
    LoadSeries  series;
    h.series(t, kDay0 + 119, series);
    EXPECT_EQ(series.count, kOpenDays);
    EXPECT_EQ(series.historyDays, 120);
}

TEST(LoadSeries, NotStartedIsEmpty)
{
    LoadState   s;
    LoadTrend   t;
    LoadHistory h(s);
    LoadSeries  series;
    h.series(t, kDay0, series);
    EXPECT_EQ(series.count, 0);
}

TEST(LoadTrend, TheModelRecordsAndTheStateFileKeepsIt)
{
    const int32_t mon = day(2026, 9, 21);
    StreakModel   m;
    m.reset(Goal {});
    for (int32_t d = 0; d < 40; ++d) {
        Events ev;
        std::vector<Found> f = {session(mon + d, static_cast<uint16_t>(30 + d % 5 * 10))};
        m.update(mon + d, f.data(), f.size(), ev);
    }
    const State& s = m.state();
    ASSERT_EQ(s.trend.count, 40 - kOpenDays);
    EXPECT_EQ(s.trend.lastDay, s.load.openStart - 1);

    char buf[StateCodec::kMaxBytes];
    const size_t len = StateCodec::encode(s, buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    EXPECT_NE(std::string(buf, len).find("\"H\":[1,"), std::string::npos);
    State back;
    ASSERT_TRUE(StateCodec::decode(buf, len, back));
    ASSERT_EQ(back.trend.count, s.trend.count);
    EXPECT_EQ(back.trend.lastDay, s.trend.lastDay);
    for (uint8_t i = 0; i < s.trend.count; ++i) {
        EXPECT_EQ(back.trend.fitnessX10[i], s.trend.fitnessX10[i]);
        EXPECT_EQ(back.trend.fatigueX10[i], s.trend.fatigueX10[i]);
        EXPECT_EQ(back.trend.load[i], s.trend.load[i]);
    }

    // The model, opened again from the file, carries on recording where it left off.
    StreakModel m2;
    m2.state() = back;
    Events ev;
    m2.update(mon + 41, nullptr, 0, ev);
    EXPECT_EQ(m2.state().trend.count, s.trend.count + 2);
    EXPECT_EQ(m2.state().trend.lastDay, m2.state().load.openStart - 1);
}

TEST(LoadTrend, AFileWithoutItOrWithABrokenOneStillReads)
{
    const int32_t mon = day(2026, 9, 21);
    StreakModel   m;
    m.reset(Goal {});
    for (int32_t d = 0; d < 20; ++d) {
        Events ev;
        std::vector<Found> f = {session(mon + d, 40)};
        m.update(mon + d, f.data(), f.size(), ev);
    }
    char buf[StateCodec::kMaxBytes];
    const size_t len = StateCodec::encode(m.state(), buf, sizeof(buf));
    std::string text(buf, len);
    const size_t at = text.find(",\"H\":[");
    ASSERT_NE(at, std::string::npos);
    const size_t end = text.find("]]", at);
    ASSERT_NE(end, std::string::npos);

    // Without it (a file from before S8): the streak and the load read; the history is empty.
    std::string older = text;
    older.erase(at, end + 2 - at);
    State back;
    ASSERT_TRUE(StateCodec::decode(older.c_str(), older.size(), back));
    EXPECT_TRUE(back.load.started);
    EXPECT_EQ(back.trend.count, 0);
    EXPECT_EQ(back.sessionCount, m.state().sessionCount);

    // Broken, or ending on the wrong day: dropped, nothing else lost.
    std::string broken = text;
    broken.replace(at, end + 2 - at, ",\"H\":[1,2,[3]]");
    ASSERT_TRUE(StateCodec::decode(broken.c_str(), broken.size(), back));
    EXPECT_EQ(back.trend.count, 0);
    EXPECT_TRUE(back.load.started);

    std::string wrongDay = text;
    const size_t comma = wrongDay.find(',', at + 7);   // after "H":[1,<lastDay>
    wrongDay.replace(at + 7, comma - (at + 7), "12345");
    ASSERT_TRUE(StateCodec::decode(wrongDay.c_str(), wrongDay.size(), back));
    EXPECT_EQ(back.trend.count, 0);
}

TEST(LoadTrend, AFullHistoryKeepsTheFileInBounds)
{
    // The widest file: a year of everything, and a full history of the largest numbers.
    const int32_t mon = day(2026, 9, 21);
    StreakModel   m;
    m.reset(Goal {});
    for (int32_t d = 0; d < 400; d += 3) {
        Events ev;
        std::vector<Found> f = {session(mon + d, 65535)};
        m.update(mon + d, f.data(), f.size(), ev);
    }
    State s = m.state();
    ASSERT_EQ(s.trend.count, kTrendDays);
    for (uint8_t i = 0; i < kTrendDays; ++i) {
        s.trend.fitnessX10[i] = 65535;
        s.trend.fatigueX10[i] = 65535;
        s.trend.load[i]       = 65535;
    }
    char buf[StateCodec::kMaxBytes];
    const size_t len = StateCodec::encode(s, buf, sizeof(buf));
    ASSERT_GT(len, 0u) << "the history must not push the file past kMaxBytes";
    State back;
    ASSERT_TRUE(StateCodec::decode(buf, len, back));
    EXPECT_EQ(back.trend.count, kTrendDays);
    std::printf("[ worst ] state file with a full history: %zu bytes of %zu\n", len, StateCodec::kMaxBytes);
}

// -- The graphs' scales (GraphMath.hpp) --------------------------------------------------------

#include "GraphMath.hpp"

TEST(GraphMath, RoundNumbers)
{
    using GraphMath::niceCeil;
    EXPECT_EQ(niceCeil(0), 1);
    EXPECT_EQ(niceCeil(1), 1);
    EXPECT_EQ(niceCeil(7), 8);
    EXPECT_EQ(niceCeil(9), 10);
    EXPECT_EQ(niceCeil(11), 15);
    EXPECT_EQ(niceCeil(55), 60);
    EXPECT_EQ(niceCeil(61), 80);
    EXPECT_EQ(niceCeil(120), 150);
    EXPECT_EQ(niceCeil(151), 200);
    EXPECT_EQ(niceCeil(65535), 80000);
}

TEST(GraphMath, FitnessAndFatigueShareAScaleFromZero)
{
    // Tenths: fitness up to 42.0, fatigue up to 61.3 -> 0 to 80 (61.3 plus room is 65 points).
    const uint16_t fit[] = {300, 350, 420};
    const uint16_t fat[] = {280, 613, 400};
    const GraphMath::Range r = GraphMath::fromZero(fit, fat, 3, 10, 100);
    EXPECT_EQ(r.lo, 0);
    EXPECT_EQ(r.hi, 800);
    // Never shorter than the minimum: a first week of small numbers is not a cliff.
    const uint16_t tiny[] = {3, 4, 5};
    EXPECT_EQ(GraphMath::fromZero(tiny, tiny, 3, 10, 100).hi, 100);
    // Whole points (daily load), one series.
    const uint16_t loads[] = {0, 45, 90, 120};
    EXPECT_EQ(GraphMath::fromZero(loads, nullptr, 4, 1, 50).hi, 150);
}

TEST(GraphMath, FormIsCentredOnZero)
{
    const uint16_t fit[] = {400, 400, 400};
    const uint16_t fat[] = {300, 520, 400};   // form +10.0, -12.0, 0
    const GraphMath::Range r = GraphMath::aroundZero(fit, fat, 3, 10, 50);
    EXPECT_EQ(r.lo, -150);
    EXPECT_EQ(r.hi, 150);
    const GraphMath::Range flat = GraphMath::aroundZero(fit, fit, 3, 10, 50);
    EXPECT_EQ(flat.hi, 50);
    EXPECT_EQ(flat.lo, -50);
}

TEST(GraphMath, PixelsStayInsideThePlot)
{
    const GraphMath::Range r {0, 800};
    EXPECT_EQ(GraphMath::yFor(800, r, 60, 100), 60);
    EXPECT_EQ(GraphMath::yFor(0, r, 60, 100), 159);
    EXPECT_EQ(GraphMath::yFor(400, r, 60, 100), 109);
    EXPECT_EQ(GraphMath::yFor(9999, r, 60, 100), 60);
    EXPECT_EQ(GraphMath::yFor(-5, r, 60, 100), 159);
    EXPECT_EQ(GraphMath::xFor(0, kTrendDays, 80, 130), 80);
    EXPECT_EQ(GraphMath::xFor(kTrendDays - 1, kTrendDays, 80, 130), 209);
    EXPECT_EQ(GraphMath::xFor(200, kTrendDays, 80, 130), 209);
}

TEST(GraphMath, ASmallGraphUsesItsOwnRange)
{
    // The Form page's 4 weeks: 30.0 to 40.0 fill the height, with a tenth each way.
    const uint16_t fit[] = {300, 320, 340};
    const uint16_t fat[] = {350, 400, 310};
    const GraphMath::Range r = GraphMath::ownRange(fit, fat, 3, 20);
    EXPECT_EQ(r.lo, 290);
    EXPECT_EQ(r.hi, 410);
    EXPECT_EQ(GraphMath::yFor(400, r, 184, 28) > 184, true);
    EXPECT_EQ(GraphMath::yFor(300, r, 184, 28) < 211, true);
    // Flat: a line in the middle, not on an edge.
    const uint16_t flat[] = {500, 500, 500};
    const GraphMath::Range f = GraphMath::ownRange(flat, flat, 3, 20);
    EXPECT_EQ(f.lo, 490);
    EXPECT_EQ(f.hi, 510);
    EXPECT_EQ(GraphMath::yFor(500, f, 184, 28), 184 + 13);
}
