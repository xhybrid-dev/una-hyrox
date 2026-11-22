/**
 * Host tests for Streak::FamilyReader: the X family's other figures (NOTES "S7"),
 * read from the shared folder the way a watch lets Streak read it, with each app's
 * own code (the copies in Libs/Family).
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#include "FamilyReader.hpp"
#include "TreeFileSystem.hpp"
#include "WeekMath.hpp"

using Streak::FamilyReader;
using Streak::FamilyView;
using XTraining::Kind;
using XTraining::TodayData;
using XTraining::TodayDay;
using XTraining::TodayStatus;
using State = XTraining::TodayFile::State;

namespace
{

int32_t day(int y, int m, int d)
{
    return Streak::WeekMath::daysFromCivil(y, static_cast<uint32_t>(m), static_cast<uint32_t>(d));
}

// Sunday 22 November 2026.
const int32_t kToday = day(2026, 11, 22);

constexpr const char* kTodayPath  = "/Apps/SharedData/HybridX/training_today.json";
constexpr const char* kVo2Path    = "/Apps/SharedData/HybridX/vo2max.json";
constexpr const char* kWeighPath  = "/Apps/SharedData/HybridX/weighins.csv";
constexpr const char* kUnitPath   = "/Apps/SharedData/HybridX/weighin_units.txt";

TodayDay row(XTraining::Ymd date, Kind kind, TodayStatus status, const char* what, const char* pace)
{
    TodayDay r;
    r.date    = date;
    r.kind    = kind;
    r.status  = status;
    r.runWeek = 3;
    XTraining::TodayFile::setText(r.what, sizeof(r.what), what);
    XTraining::TodayFile::setText(r.pace, sizeof(r.pace), pace);
    return r;
}

/// A plan from 2 November to 20 December; the file covers 16 November to 6 December.
TodayData plan()
{
    TodayData d;
    d.plan  = 1;
    d.weeks = 7;
    d.start = 20261102;
    d.end   = 20261220;
    d.first = 20261116;
    d.last  = 20261206;
    d.day[d.count++] = row(20261120, Kind::Easy, TodayStatus::Done, "40 min", "5:40/km");
    d.day[d.count++] = row(20261122, Kind::Tempo, TodayStatus::ToCome, "30 min", "4:18/km");
    d.day[d.count++] = row(20261123, Kind::Rest, TodayStatus::ToCome, "", "");
    d.day[d.count++] = row(20261124, Kind::Intervals, TodayStatus::ToCome, "5 x 3 min", "3:58/km");
    return d;
}

std::string json(const TodayData& d)
{
    char buf[XTraining::TodayFile::kMaxJsonBytes];
    const size_t len = XTraining::TodayFile::write(d, buf, sizeof(buf));
    EXPECT_GT(len, 0u);
    return std::string(buf, len);
}

struct Watch {
    TreeFileSystem fs {"/Apps/HybridXStreak"};
    Watch()
    {
        fs.addDir("/Apps/SharedData/HybridX");
        fs.blockParentAccess(true);
        fs.blockListingOutside(true);
    }
};

} // namespace

TEST(FamilyReader, LocalDaysAreXTrainingDates)
{
    EXPECT_EQ(FamilyReader::ymdOf(kToday), 20261122u);
    EXPECT_EQ(FamilyReader::ymdOf(day(2028, 2, 29)), 20280229u);
    EXPECT_EQ(FamilyReader::ymdOf(day(2026, 12, 31) + 1), 20270101u);
    EXPECT_EQ(FamilyReader::ymdOf(0), 0u);   // 1970: outside X Training's dates
}

TEST(FamilyReader, TodaysSessionAsTheGlancePicksIt)
{
    const TodayData d = plan();
    Streak::PlanToday p;
    FamilyReader::planFrom(&d, kToday, true, p);
    EXPECT_EQ(p.state, static_cast<uint8_t>(State::Session));
    EXPECT_EQ(p.kind, static_cast<uint8_t>(Kind::Tempo));
    EXPECT_STREQ(p.what, "30 min");
    EXPECT_STREQ(p.pace, "4:18/km");
    // The next session skips the rest day.
    EXPECT_EQ(p.hasNext, 1);
    EXPECT_EQ(p.nextKind, static_cast<uint8_t>(Kind::Intervals));
    EXPECT_EQ(p.nextTomorrow, 0);
    EXPECT_EQ(p.nextWeekday, 1);   // Tuesday, Monday 0
    EXPECT_STREQ(p.nextWhat, "5 x 3 min");
    EXPECT_EQ(p.startDate, 20261102u);
}

TEST(FamilyReader, DoneRestStaleAndNoClock)
{
    const TodayData d = plan();
    Streak::PlanToday p;
    FamilyReader::planFrom(&d, day(2026, 11, 20), true, p);
    EXPECT_EQ(p.state, static_cast<uint8_t>(State::Done));

    FamilyReader::planFrom(&d, day(2026, 11, 23), true, p);
    EXPECT_EQ(p.state, static_cast<uint8_t>(State::Rest));
    EXPECT_EQ(p.hasNext, 1);
    EXPECT_EQ(p.nextTomorrow, 1);

    FamilyReader::planFrom(&d, day(2026, 12, 10), true, p);   // after the file's window
    EXPECT_EQ(p.state, static_cast<uint8_t>(State::Stale));
    EXPECT_EQ(p.hasNext, 0);

    FamilyReader::planFrom(&d, kToday, false, p);
    EXPECT_EQ(p.state, static_cast<uint8_t>(State::ClockUnset));

    FamilyReader::planFrom(nullptr, kToday, true, p);
    EXPECT_EQ(p.state, static_cast<uint8_t>(State::NoFile));
}

TEST(FamilyReader, NothingThereIsNothingShown)
{
    Watch        w;
    FamilyReader r;
    FamilyView   v;
    r.read(w.fs, kToday, true, v);
    EXPECT_EQ(v.flags, 0);
    EXPECT_EQ(v.today, kToday);
    EXPECT_EQ(v.clockOk, 1);
}

TEST(FamilyReader, ReadsAllThree)
{
    Watch w;
    w.fs.addFile(kTodayPath, json(plan()));
    w.fs.addFile(kVo2Path, R"({"v":2,"vo2maxX10":523,"runs":5,"utc":1791000000,"beforeX10":519,"trend":[505,512,519,523]})");
    // 80.00 kg, then 81.00 the next day: the trend starts at the first and closes 20 % of the gap.
    w.fs.addFile(kWeighPath, "date,time,kg\n2026-11-20,07:31,80.00\n2026-11-21,07:30,81.00\n");
    w.fs.addFile(kUnitPath, "0\n");

    FamilyReader r;
    FamilyView   v;
    r.read(w.fs, kToday, true, v);
    EXPECT_EQ(v.flags, FamilyView::kPlan | FamilyView::kVo2 | FamilyView::kWeight);
    EXPECT_EQ(v.plan.state, static_cast<uint8_t>(State::Session));
    EXPECT_EQ(v.vo2.x10, 523);
    EXPECT_EQ(v.vo2.runs, 5);
    EXPECT_EQ(v.weight.trend, 8020);
    EXPECT_EQ(v.weight.days, 2);
    EXPECT_EQ(v.weight.lastDay, day(2026, 11, 21));
    EXPECT_EQ(v.weight.hasWeekChange, 0);   // under a week
    EXPECT_EQ(v.weight.unit, 0);
}

TEST(FamilyReader, AWeekOfWeighInsHasAChangeAndItsUnit)
{
    Watch       w;
    std::string csv = "date,time,kg\n";
    for (int d = 1; d <= 10; ++d) {
        char line[40];
        std::snprintf(line, sizeof(line), "2026-11-%02d,07:00,%d.00\n", 10 + d, 80 - (d >= 5 ? 1 : 0));
        csv += line;
    }
    w.fs.addFile(kWeighPath, csv);
    w.fs.addFile(kUnitPath, "1\n");   // stones and pounds

    FamilyReader r;
    FamilyView   v;
    r.read(w.fs, kToday, true, v);
    ASSERT_EQ(v.flags, FamilyView::kWeight);
    EXPECT_EQ(v.weight.hasWeekChange, 1);
    EXPECT_LT(v.weight.weekChange, 0);   // down a kilo from the 15th
    EXPECT_GT(v.weight.trend, 7900);
    EXPECT_LT(v.weight.trend, 8000);
    EXPECT_EQ(v.weight.unit, 1);

    // The same as X Weigh-in's own trend makes of it.
    XWeighIn::Trend t;
    XWeighIn::LogStore::scan(w.fs, XWeighIn::LogStore::kSharedFile, t);
    EXPECT_EQ(v.weight.trend, t.summary().trend);
    EXPECT_EQ(v.weight.weekChange, t.summary().weekChange);
}

TEST(FamilyReader, ABrokenFileFallsBackToItsBackup)
{
    Watch w;
    w.fs.addFile(kTodayPath, "{\"v\":1,\"plan\":");   // torn mid-save
    w.fs.addFile(std::string(kTodayPath) + ".bak", json(plan()));
    w.fs.addFile(kVo2Path, "{\"v\":2,\"vo2maxX10\":");
    w.fs.addFile(std::string(kVo2Path) + ".bak", R"({"v":1,"vo2maxX10":480,"runs":2,"utc":1791000000})");

    FamilyReader r;
    FamilyView   v;
    r.read(w.fs, kToday, true, v);
    EXPECT_EQ(v.flags, FamilyView::kPlan | FamilyView::kVo2);
    EXPECT_EQ(v.plan.kind, static_cast<uint8_t>(Kind::Tempo));
    EXPECT_EQ(v.vo2.x10, 480);
}

TEST(FamilyReader, FiguresOutOfRangeAreNotShown)
{
    Watch w;
    w.fs.addFile(kVo2Path, R"({"v":2,"vo2maxX10":9999,"runs":5,"utc":1791000000})");   // past X Run's range
    w.fs.addFile(kWeighPath, "date,time,kg\nnot,a,weight\n");                              // no valid weigh-in
    w.fs.addFile(kTodayPath, R"({"v":1,"plan":0})");                                       // no plan

    FamilyReader r;
    FamilyView   v;
    r.read(w.fs, kToday, true, v);
    EXPECT_EQ(v.flags & (FamilyView::kVo2 | FamilyView::kWeight), 0);
    // "No plan" is a file that read: the screen decides it is not worth a row (DashTextTest).
    if ((v.flags & FamilyView::kPlan) != 0) {
        EXPECT_EQ(v.plan.state, static_cast<uint8_t>(State::NoPlan));
    }
}

TEST(FamilyReader, WeightNeedsAWeighIn)
{
    XWeighIn::Summary s;
    Streak::WeightFigure f;
    EXPECT_FALSE(FamilyReader::weightFrom(s, XWeighIn::WeightUnit::Kg, f));
    s.days    = 3;
    s.trend   = 7512;
    s.lastDay = kToday;
    EXPECT_TRUE(FamilyReader::weightFrom(s, XWeighIn::WeightUnit::Lb, f));
    EXPECT_EQ(f.trend, 7512);
    EXPECT_EQ(f.unit, static_cast<uint8_t>(XWeighIn::WeightUnit::Lb));
    EXPECT_EQ(f.hasWeekChange, 0);
}
