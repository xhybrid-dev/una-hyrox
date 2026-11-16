#include "PlanSchedule.hpp"
#include "PlanText.hpp"
#include "TodayBuild.hpp"
#include "TodayFile.hpp"
#include "data/PlanData.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <string>
#include <tuple>

using namespace XTraining;
using TodayFile::State;

namespace {

// A plan from Monday 4 January 2027 to Sunday 14 March 2027 (10 weeks), and a file written on
// Wednesday 20 January: its window is Monday the 18th through Wednesday 3 February.
constexpr Ymd kStart = 20270104u;
constexpr Ymd kEnd   = 20270314u;
constexpr Ymd kWrit  = 20270120u;

TodayData makeData(Ymd today = kWrit)
{
    TodayData d;
    d.writtenUtc = 1800000000u;
    d.plan   = 1;
    d.weeks  = 10;
    d.start  = kStart;
    d.end    = kEnd;
    d.first  = Date::fromDayNum(Date::mondayOf(Date::toDayNum(today)));
    d.last   = Date::addDays(today, 14);
    // Tuesday, Wednesday, Friday and Sunday run; the rest rest.
    for (Ymd date = d.first; date <= d.last && d.count < TodayData::kMaxDays; date = Date::addDays(date, 1)) {
        if (date < kStart || date > kEnd) {
            continue;
        }
        TodayDay& row = d.day[d.count++];
        row.date = date;
        row.runWeek = static_cast<uint8_t>((Date::toDayNum(date) - Date::toDayNum(kStart)) / 7 + 1);
        switch (Date::weekday(date)) {
        case Weekday::Tue:
            row.kind = Kind::Intervals;
            TodayFile::setText(row.what, sizeof(row.what), "4 x 4 min");
            TodayFile::setText(row.pace, sizeof(row.pace), "4:18/km");
            break;
        case Weekday::Wed:
            row.kind = Kind::Easy;
            TodayFile::setText(row.what, sizeof(row.what), "45 min");
            break;
        case Weekday::Fri:
            row.kind = Kind::Cruise;
            TodayFile::setText(row.what, sizeof(row.what), "3 x 10 min");
            TodayFile::setText(row.pace, sizeof(row.pace), "4:35/km");
            break;
        case Weekday::Sun:
            row.kind = Kind::Long;
            TodayFile::setText(row.what, sizeof(row.what), "70 min");
            break;
        default:
            row.kind = Kind::Rest;
            break;
        }
    }
    return d;
}

std::string written(const TodayData& d)
{
    char buf[TodayFile::kMaxJsonBytes];
    const size_t n = TodayFile::write(d, buf, sizeof(buf));
    EXPECT_GT(n, 0u);
    return std::string(buf, n);
}

bool readString(const std::string& s, TodayData& out) { return TodayFile::read(s.data(), s.size(), out); }

void expectSame(const TodayData& a, const TodayData& b)
{
    EXPECT_EQ(a.writtenUtc, b.writtenUtc);
    EXPECT_EQ(a.plan, b.plan);
    EXPECT_EQ(a.weeks, b.weeks);
    EXPECT_EQ(a.pausedFrom, b.pausedFrom);
    EXPECT_EQ(a.start, b.start);
    EXPECT_EQ(a.end, b.end);
    EXPECT_EQ(a.first, b.first);
    EXPECT_EQ(a.last, b.last);
    ASSERT_EQ(a.count, b.count);
    for (uint8_t i = 0; i < a.count; ++i) {
        EXPECT_EQ(a.day[i].date, b.day[i].date) << int(i);
        EXPECT_EQ(a.day[i].kind, b.day[i].kind) << int(i);
        EXPECT_EQ(a.day[i].status, b.day[i].status) << int(i);
        EXPECT_EQ(a.day[i].runWeek, b.day[i].runWeek) << int(i);
        EXPECT_STREQ(a.day[i].what, b.day[i].what) << int(i);
        EXPECT_STREQ(a.day[i].pace, b.day[i].pace) << int(i);
    }
}

}  // namespace

TEST(TodayFile, RoundTrip)
{
    const TodayData d = makeData();
    ASSERT_EQ(d.count, 17);   // the 18th to the 3rd: 17 days
    TodayData back;
    ASSERT_TRUE(readString(written(d), back));
    expectSame(d, back);
}

TEST(TodayFile, RoundTripKeepsEveryKindStatusAndAnEmptyString)
{
    TodayData d = makeData();
    d.pausedFrom = 20270125u;
    uint8_t i = 0;
    for (uint8_t k = 0; k < kKindCount && i < d.count; ++k, ++i) {
        d.day[i].kind   = static_cast<Kind>(k);
        d.day[i].status = static_cast<TodayStatus>(k % 5);
    }
    TodayFile::setText(d.day[0].what, sizeof(d.day[0].what), "");
    TodayData back;
    ASSERT_TRUE(readString(written(d), back));
    expectSame(d, back);
}

TEST(TodayFile, NoPlanIsAFileOfItsOwn)
{
    TodayData d;
    d.writtenUtc = 1800000000u;
    const std::string s = written(d);
    EXPECT_EQ(s, "{\"v\":1,\"u\":1800000000,\"plan\":0}");
    TodayData back;
    ASSERT_TRUE(readString(s, back));
    EXPECT_EQ(back.plan, 0);
    EXPECT_EQ(back.count, 0);
}

TEST(TodayFile, TheLargestFileFits)
{
    TodayData d = makeData(20270124u);   // a Sunday: a Monday through 14 days after, 21 rows
    ASSERT_EQ(d.count, 21);
    for (uint8_t i = 0; i < d.count; ++i) {
        d.day[i].kind = Kind::LongFinish;
        TodayFile::setText(d.day[i].what, sizeof(d.day[i].what), "1234567890123456789012345");
        TodayFile::setText(d.day[i].pace, sizeof(d.day[i].pace), "12:34/mile 12:34");
        d.day[i].runWeek = 40;
    }
    char buf[TodayFile::kMaxJsonBytes];
    const size_t n = TodayFile::write(d, buf, sizeof(buf));
    EXPECT_GT(n, 0u);
    EXPECT_LE(n, TodayFile::kMaxJsonBytes - 200) << "room to grow";
    TodayData back;
    EXPECT_TRUE(TodayFile::read(buf, n, back));
    // Too small a buffer: nothing written, never an overrun.
    char small[200];
    EXPECT_EQ(TodayFile::write(d, small, sizeof(small)), 0u);
    EXPECT_EQ(small[0], '\0');
}

TEST(TodayFile, SetTextKeepsPrintableAsciiAndFits)
{
    char b[8];
    TodayFile::setText(b, sizeof(b), "a\"b\\c\xC3\xA9 d\n123456789");
    EXPECT_STREQ(b, "abc d12");
    TodayFile::setText(b, sizeof(b), nullptr);
    EXPECT_STREQ(b, "");
}

TEST(TodayFile, EveryTruncationIsRefused)
{
    const std::string s = written(makeData());
    for (size_t n = 0; n < s.size(); ++n) {
        TodayData back;
        EXPECT_FALSE(TodayFile::read(s.data(), n, back)) << n;
        EXPECT_EQ(back.plan, 0) << n;
        EXPECT_EQ(back.count, 0) << n;
    }
}

TEST(TodayFile, AChangedByteNeverGivesAFileThatBreaksTheRules)
{
    const std::string good = written(makeData());
    for (size_t i = 0; i < good.size(); ++i) {
        for (const char c : {'#', '\x01', '9', '"', '\\', '}'}) {
            std::string s = good;
            s[i] = c;
            TodayData back;
            if (!readString(s, back)) {
                EXPECT_EQ(back.count, 0);
                continue;
            }
            // It read: then it must still be one the glance can trust.
            for (uint8_t r = 0; r < back.count; ++r) {
                EXPECT_TRUE(Date::valid(back.day[r].date));
                EXPECT_GE(back.day[r].date, back.first);
                EXPECT_LE(back.day[r].date, back.last);
                EXPECT_LT(static_cast<uint8_t>(back.day[r].kind), kKindCount);
                if (r > 0) {
                    EXPECT_GT(back.day[r].date, back.day[r - 1].date);
                }
                EXPECT_LT(std::strlen(back.day[r].what), sizeof(back.day[r].what));
            }
        }
    }
}

TEST(TodayFile, TheRulesOfTheFile)
{
    const TodayData good = makeData();
    TodayData back;
    auto bad = [&](const std::string& from, const std::string& to) {
        std::string s = written(good);
        const size_t at = s.find(from);
        EXPECT_NE(at, std::string::npos) << from;
        if (at == std::string::npos) {
            return false;
        }
        s.replace(at, from.size(), to);
        return readString(s, back);
    };
    EXPECT_FALSE(bad("\"v\":1", "\"v\":2")) << "a newer version is not read";
    EXPECT_FALSE(bad("\"v\":1", "\"v\":0"));
    EXPECT_FALSE(bad("\"plan\":1", "\"plan\":4"));
    EXPECT_FALSE(bad("\"wks\":10", "\"wks\":0"));
    EXPECT_FALSE(bad("\"wks\":10", "\"wks\":41"));
    EXPECT_FALSE(bad("\"s\":20270104", "\"s\":20270230")) << "not a date";
    EXPECT_FALSE(bad("\"e\":20270314", "\"e\":20270103")) << "ends before it starts";
    EXPECT_FALSE(bad("\"t\":20270203", "\"t\":20270301")) << "a window longer than 21 days";
    EXPECT_FALSE(bad("\"t\":20270203", "\"t\":20270117")) << "last before first";
    EXPECT_FALSE(bad("[20270119,", "[20270118,")) << "a repeated day";
    EXPECT_FALSE(bad("[20270119,", "[20270117,")) << "a day before the window";
    EXPECT_FALSE(bad("[20270119,", "[20270102,")) << "a day before the plan";
    EXPECT_FALSE(bad("[20270119,", "[20270119,99,")) << "a row of seven";
    EXPECT_FALSE(bad("\"4 x 4 min\"", "\"4 x \\\"4 min\"")) << "an escape in a string";
    EXPECT_FALSE(bad("\"4 x 4 min\"", "\"123456789012345678901\"")) << "a string too long";
    EXPECT_FALSE(bad("\"4:18/km\"", "\"4:18/km 4:18/km\"")) << "a pace too long";
    EXPECT_FALSE(bad("\"4:18/km\"", "7")) << "a number where a string goes";
    EXPECT_TRUE(bad("\"wks\":10", "\"wks\":40"));
    EXPECT_TRUE(bad("\"ps\":0", "\"ps\":20270125"));
}

TEST(TodayFile, RowFieldsAreRangeChecked)
{
    // Rewrite one row's numbers: kind, status, run week.
    const TodayData good = makeData();
    TodayData back;
    for (const auto& [from, to, ok] : std::initializer_list<std::tuple<const char*, const char*, bool>>{
             {"[20270119,7,0,3,", "[20270119,16,0,3,", true},    // the last kind
             {"[20270119,7,0,3,", "[20270119,17,0,3,", false},   // one more
             {"[20270119,7,0,3,", "[20270119,7,4,3,", true},     // skipped
             {"[20270119,7,0,3,", "[20270119,7,5,3,", false},
             {"[20270119,7,0,3,", "[20270119,7,0,0,", false},    // run week 1-based
             {"[20270119,7,0,3,", "[20270119,7,0,255,", true},
             {"[20270119,7,0,3,", "[20270119,7,0,256,", false}}) {
        std::string s = written(good);
        const size_t at = s.find(from);
        ASSERT_NE(at, std::string::npos) << from;
        s.replace(at, std::strlen(from), to);
        EXPECT_EQ(readString(s, back), ok) << to;
    }
}

TEST(TodayFile, MoreThan21RowsAreRefused)
{
    std::string s = "{\"v\":1,\"u\":1,\"plan\":1,\"wks\":10,\"ps\":0,\"s\":20270104,\"e\":20270314,"
                    "\"f\":20270118,\"t\":20270208,\"d\":[";
    for (int i = 0; i < 22; ++i) {
        char row[64];
        std::snprintf(row, sizeof(row), "%s[%u,0,0,3,\"\",\"\"]", i == 0 ? "" : ",",
                      static_cast<unsigned>(Date::addDays(20270118u, i)));
        s += row;
    }
    s += "]}";
    TodayData back;
    EXPECT_FALSE(readString(s, back));
}

// ---- pick ------------------------------------------------------------------------------

TEST(TodayFile, NoFileNoPlanNoClock)
{
    EXPECT_EQ(TodayFile::pick(nullptr, kWrit).state, State::NoFile);
    TodayData none;
    EXPECT_EQ(TodayFile::pick(&none, kWrit).state, State::NoPlan);
    const TodayData d = makeData();
    EXPECT_EQ(TodayFile::pick(&d, 0).state, State::ClockUnset);
    EXPECT_EQ(TodayFile::pick(&d, 20270230u).state, State::ClockUnset) << "not a date";
}

TEST(TodayFile, ASessionTodayAndTheNextOne)
{
    const TodayData d = makeData();   // Wednesday the 20th: an easy run; Friday the 22nd is next
    const TodayFile::Pick p = TodayFile::pick(&d, 20270120u);
    EXPECT_EQ(p.state, State::Session);
    ASSERT_NE(p.today, nullptr);
    EXPECT_EQ(p.today->kind, Kind::Easy);
    ASSERT_NE(p.next, nullptr);
    EXPECT_EQ(p.next->date, 20270122u);
    EXPECT_EQ(p.next->kind, Kind::Cruise);
    // Tuesday the 19th: the next is the very next day.
    const TodayFile::Pick tue = TodayFile::pick(&d, 20270119u);
    EXPECT_EQ(tue.state, State::Session);
    EXPECT_EQ(tue.today->kind, Kind::Intervals);
    EXPECT_EQ(tue.next->date, 20270120u);
}

TEST(TodayFile, DoneSkippedAndPartDone)
{
    TodayData d = makeData();
    TodayDay* wed = const_cast<TodayDay*>(d.find(20270120u));
    ASSERT_NE(wed, nullptr);
    wed->status = TodayStatus::Done;
    EXPECT_EQ(TodayFile::pick(&d, 20270120u).state, State::Done);
    wed->status = TodayStatus::PartDone;
    EXPECT_EQ(TodayFile::pick(&d, 20270120u).state, State::Done);
    wed->status = TodayStatus::Skipped;
    EXPECT_EQ(TodayFile::pick(&d, 20270120u).state, State::Skipped);
    wed->status = TodayStatus::ToCome;
    EXPECT_EQ(TodayFile::pick(&d, 20270120u).state, State::Session);
}

TEST(TodayFile, ARestDayNamesTheNextSession)
{
    const TodayData d = makeData();
    const TodayFile::Pick p = TodayFile::pick(&d, 20270121u);   // Thursday
    EXPECT_EQ(p.state, State::Rest);
    ASSERT_NE(p.next, nullptr);
    EXPECT_EQ(p.next->date, 20270122u);
}

TEST(TodayFile, NoNextSessionAtTheEdgeOfTheWindow)
{
    const TodayData d = makeData();
    const TodayFile::Pick p = TodayFile::pick(&d, 20270203u);   // the last day: Wednesday
    EXPECT_EQ(p.state, State::Session);
    EXPECT_EQ(p.next, nullptr);
}

TEST(TodayFile, FourteenDaysAfterTheFileWasWrittenItStillReads)
{
    const TodayData d = makeData();   // written Wednesday 20 January; the window ends 3 February
    EXPECT_EQ(TodayFile::pick(&d, 20270118u).state, State::Rest) << "Monday of its own week";
    EXPECT_EQ(TodayFile::pick(&d, 20270203u).state, State::Session) << "the last day";
    const TodayFile::Pick late = TodayFile::pick(&d, 20270204u);
    EXPECT_EQ(late.state, State::Stale) << "the day after";
    EXPECT_EQ(late.today, nullptr);
    EXPECT_EQ(late.next, nullptr);
}

TEST(TodayFile, AFileFromTheFutureIsStale)
{
    const TodayData d = makeData();
    EXPECT_EQ(TodayFile::pick(&d, 20270117u).state, State::Stale) << "the Sunday before its window";
    EXPECT_EQ(TodayFile::pick(&d, 20260101u).state, State::Stale);
}

TEST(TodayFile, AWeekChangeKeepsTheWeekOnItsSide)
{
    // Written on a Sunday, read the next Monday and the Sunday after: still inside.
    const TodayData d = makeData(20270124u);
    EXPECT_EQ(TodayFile::pick(&d, 20270124u).state, State::Session);
    EXPECT_EQ(TodayFile::pick(&d, 20270125u).state, State::Rest);      // Monday
    EXPECT_EQ(TodayFile::pick(&d, 20270126u).state, State::Session);   // Tuesday: intervals
    EXPECT_EQ(TodayFile::pick(&d, 20270207u).state, State::Session);   // the second Sunday after
    EXPECT_EQ(TodayFile::pick(&d, 20270208u).state, State::Stale);
}

TEST(TodayFile, PausedFromThatDayOn)
{
    TodayData d = makeData();
    d.pausedFrom = 20270122u;
    EXPECT_EQ(TodayFile::pick(&d, 20270121u).state, State::Rest);
    EXPECT_EQ(TodayFile::pick(&d, 20270122u).state, State::Paused);
    EXPECT_EQ(TodayFile::pick(&d, 20270130u).state, State::Paused);
}

TEST(TodayFile, BeforeTheStartAndAfterTheEnd)
{
    // Written Thursday 31 December 2026, a plan starting Monday 4 January.
    TodayData d;
    d.plan = 1;
    d.weeks = 10;
    d.start = kStart;
    d.end = kEnd;
    d.first = 20261228u;
    d.last = 20270114u;
    for (Ymd date = kStart; date <= d.last; date = Date::addDays(date, 1)) {
        TodayDay& r = d.day[d.count++];
        r.date = date;
        r.runWeek = static_cast<uint8_t>((Date::toDayNum(date) - Date::toDayNum(kStart)) / 7 + 1);
    }
    EXPECT_EQ(TodayFile::pick(&d, 20261231u).state, State::BeforeStart);
    EXPECT_EQ(TodayFile::pick(&d, 20270104u).state, State::Rest);
    // The last day and the days after it: the plan is complete.
    TodayData e = makeData(20270314u);
    EXPECT_EQ(TodayFile::pick(&e, 20270314u).state, State::Session) << "the last day, a Sunday";
    EXPECT_EQ(TodayFile::pick(&e, 20270315u).state, State::Complete);
    EXPECT_EQ(TodayFile::pick(&e, 20270320u).state, State::Complete);
}

TEST(TodayFile, ADayOfThePlanWithNoRowIsNotATrustedFile)
{
    TodayData d = makeData();
    // Remove Thursday the 21st's row.
    uint8_t out = 0;
    for (uint8_t i = 0; i < d.count; ++i) {
        if (d.day[i].date != 20270121u) {
            d.day[out++] = d.day[i];
        }
    }
    d.count = out;
    EXPECT_EQ(TodayFile::pick(&d, 20270121u).state, State::Stale);
}

// ---- TodayBuild: the file from a real plan ----------------------------------------------

namespace {

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
};

}  // namespace

TEST(TodayBuild, ARealPlanRoundTripsAndNamesThePlansSessions)
{
    Plan p;
    TodayData d;
    TodayBuild::fill(p.st, &p.paces, false, kWrit, 1800000000u, p.scratch.get(), d);
    ASSERT_EQ(d.plan, static_cast<uint8_t>(PlanId::TenK));
    EXPECT_EQ(d.weeks, p.st.calendar.weeks);
    EXPECT_EQ(d.start, kStart);
    EXPECT_EQ(d.end, Date::addDays(kStart, p.st.calendar.weeks * 7 - 1));
    EXPECT_EQ(d.first, 20270118u);
    EXPECT_EQ(d.last, 20270203u);
    ASSERT_EQ(d.count, 17);
    for (uint8_t i = 0; i < d.count; ++i) {
        DaySession s;
        ASSERT_TRUE(PlanSchedule::day(p.st.setup, p.def, p.st.calendar, p.paces, d.day[i].date, s));
        EXPECT_EQ(d.day[i].kind, s.asRun.kind) << int(i);
        EXPECT_EQ(d.day[i].runWeek, s.runWeek + 1) << int(i);
        EXPECT_EQ(d.day[i].status, TodayStatus::ToCome);
        if (s.asRun.kind != Kind::Rest) {
            EXPECT_GT(std::strlen(d.day[i].what), 0u) << int(i);
        }
    }
    char buf[TodayFile::kMaxJsonBytes];
    const size_t n = TodayFile::write(d, buf, sizeof(buf));
    ASSERT_GT(n, 0u);
    TodayData back;
    ASSERT_TRUE(TodayFile::read(buf, n, back));
    expectSame(d, back);
}

TEST(TodayBuild, EveryPlanLevelAndAWeekOfEveryDayFitsTheFile)
{
    for (PlanId id : {PlanId::TenK, PlanId::Half, PlanId::Vo2max}) {
        for (uint8_t level = 1; level <= 3; ++level) {
            Plan p;
            const PlanDef& def = *PlanData::find(id);
            p.st.setup.plan = id;
            p.st.setup.level = level;
            p.st.calendar = PlanCalendar::make({kStart, 0, def.weekCount, def.minWeeks});
            // Every day of the plan as a "today": the file always fits and reads back.
            for (int n = 0; n < def.weekCount * 7; n += 3) {
                const Ymd today = Date::addDays(kStart, n);
                TodayData d;
                TodayBuild::fill(p.st, &p.paces, n % 2 == 0, today, 1800000000u, p.scratch.get(), d);
                ASSERT_EQ(d.plan, static_cast<uint8_t>(id));
                char buf[TodayFile::kMaxJsonBytes];
                const size_t len = TodayFile::write(d, buf, sizeof(buf));
                ASSERT_GT(len, 0u) << int(id) << " " << int(level) << " " << today;
                TodayData back;
                ASSERT_TRUE(TodayFile::read(buf, len, back)) << today;
                expectSame(d, back);
                EXPECT_NE(TodayFile::pick(&back, today).state, State::Stale) << today;
            }
        }
    }
}

TEST(TodayBuild, TheLogAndMovesAreInTheRows)
{
    Plan p;
    DaySession tue;
    ASSERT_TRUE(PlanSchedule::day(p.st.setup, p.def, p.st.calendar, p.paces, 20270119u, tue));
    p.st.record({20270119u, tue.asRun.kind, DayStatus::Done, 1});
    p.st.record({20270120u, Kind::Easy, DayStatus::Skipped, 0});
    p.st.record({20270118u, Kind::Easy, DayStatus::PartDone, 0});
    p.st.record({20270122u, Kind::Easy, DayStatus::Missed, 0});
    TodayData d;
    TodayBuild::fill(p.st, &p.paces, false, 20270120u, 1u, p.scratch.get(), d);
    EXPECT_EQ(d.find(20270119u)->status, TodayStatus::Done);
    EXPECT_EQ(d.find(20270120u)->status, TodayStatus::Skipped);
    EXPECT_EQ(d.find(20270118u)->status, TodayStatus::PartDone);
    EXPECT_EQ(d.find(20270122u)->status, TodayStatus::Missed);
    EXPECT_EQ(d.find(20270121u)->status, TodayStatus::ToCome);
    // An override (today made easy, tomorrow rested) shows in the rows.
    p.st.setOverride({20270121u, 0, OverrideMode::Rest});
    TodayBuild::fill(p.st, &p.paces, false, 20270120u, 1u, p.scratch.get(), d);
    EXPECT_EQ(d.find(20270121u)->kind, Kind::Rest);
}

TEST(TodayBuild, PausedAndNoPlanAndNoPaces)
{
    Plan p;
    p.st.pausedFrom = 20270125u;
    TodayData d;
    TodayBuild::fill(p.st, &p.paces, false, kWrit, 1u, p.scratch.get(), d);
    EXPECT_EQ(d.pausedFrom, 20270125u);
    EXPECT_EQ(TodayFile::pick(&d, 20270126u).state, State::Paused);
    // No paces (a 5K the model refuses), no plan, no date: no plan to show.
    TodayBuild::fill(p.st, nullptr, false, kWrit, 1u, p.scratch.get(), d);
    EXPECT_EQ(d.plan, 0);
    TodayBuild::fill(PlanState{}, &p.paces, false, kWrit, 1u, p.scratch.get(), d);
    EXPECT_EQ(d.plan, 0);
    TodayBuild::fill(p.st, &p.paces, false, 0, 1u, p.scratch.get(), d);
    EXPECT_EQ(d.plan, 0);
}

TEST(TodayBuild, NoScratchLeavesThePaceOut)
{
    Plan p;
    TodayData withPace, without;
    TodayBuild::fill(p.st, &p.paces, false, 20270119u, 1u, p.scratch.get(), withPace);
    TodayBuild::fill(p.st, &p.paces, false, 20270119u, 1u, nullptr, without);
    bool any = false;
    for (uint8_t i = 0; i < withPace.count; ++i) {
        any = any || withPace.day[i].pace[0] != '\0';
        EXPECT_STREQ(without.day[i].pace, "");
        EXPECT_STREQ(without.day[i].what, withPace.day[i].what);
    }
    EXPECT_TRUE(any) << "a plan has paced sessions";
}

TEST(TodayBuild, PaceTextInKilometresAndMiles)
{
    char b[16];
    TodayBuild::paceText(258, false, b, sizeof(b));
    EXPECT_STREQ(b, "4:18/km");
    TodayBuild::paceText(258, true, b, sizeof(b));   // 258 s/km = 415.2 s/mi
    EXPECT_STREQ(b, "6:55/mi");
    TodayBuild::paceText(600, true, b, sizeof(b));   // 965 s/mi
    EXPECT_STREQ(b, "16:05/mi");
    char tiny[4];
    TodayBuild::paceText(258, false, tiny, sizeof(tiny));
    EXPECT_EQ(std::strlen(tiny), 3u) << "cut to fit, terminated";
}

TEST(PlanText, GlanceWhat)
{
    char b[32];
    PlanText::glanceWhat(Session{Kind::LongFinish, 0, 100, 20, 0}, 0, b, sizeof(b));
    EXPECT_STREQ(b, "100 min, 20 fast");
    PlanText::glanceWhat(Session{Kind::Intervals, 4, 240, 0, 0}, 0, b, sizeof(b));
    EXPECT_STREQ(b, "4 x 4 min");
    PlanText::glanceWhat(Session{Kind::Race, 0, 0, 0, 0}, 1000000, b, sizeof(b));
    EXPECT_STREQ(b, "10 km");
    for (uint8_t k = 1; k < kKindCount; ++k) {
        Session s{static_cast<Kind>(k), 9, 999, 99, 0};
        PlanText::glanceWhat(s, 2110000, b, sizeof(b));
        EXPECT_LT(std::strlen(b), TodayDay::kWhatLen) << int(k) << " " << b;
    }
}
