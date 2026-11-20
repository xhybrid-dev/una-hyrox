#include "ReviewLog.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

using namespace XTraining;
using namespace XTraining::Review;

namespace {

ReviewEntry entry(Ymd date, uint8_t in = 80, uint8_t fast = 10, int16_t gap = -4, uint8_t feel = 0)
{
    ReviewEntry e;
    e.date      = date;
    e.kind      = Kind::Intervals;
    e.inPct     = in;
    e.fastPct   = fast;
    e.gapSecKm  = gap;
    e.fadeSecKm = 2;
    e.judgedSec = 900;
    e.feel      = feel;
    return e;
}

std::string json(const ReviewLog& l)
{
    char buf[ReviewLog::kMaxJsonBytes];
    const size_t n = l.toJson(buf, sizeof(buf));
    return std::string(buf, n);
}

bool parse(ReviewLog& l, const std::string& s)
{
    return l.fromJson(s.c_str(), s.size());
}

}  // namespace

TEST(ReviewLog, EmptyRoundTrips)
{
    ReviewLog a;
    ReviewLog b;
    ASSERT_TRUE(parse(b, json(a)));
    EXPECT_EQ(b.count(), 0);
    EXPECT_EQ(b.handled, 0u);
}

TEST(ReviewLog, EntriesRoundTripWithNegativeGapsAndTheAnswerDate)
{
    ReviewLog a;
    a.record(entry(20261117u, 80, 10, -4, 2));
    a.record(entry(20261119u, 40, 0, 11, 5));
    a.handled = 20261117u;
    const std::string s = json(a);
    ASSERT_FALSE(s.empty());
    ReviewLog b;
    ASSERT_TRUE(parse(b, s)) << s;
    ASSERT_EQ(b.count(), 2);
    EXPECT_EQ(b.handled, 20261117u);
    EXPECT_EQ(b.at(0).date, 20261117u);
    EXPECT_EQ(b.at(0).gapSecKm, -4);
    EXPECT_EQ(b.at(0).feel, 2);
    EXPECT_EQ(b.at(0).kind, Kind::Intervals);
    EXPECT_EQ(b.at(1).inPct, 40);
    EXPECT_EQ(b.at(1).slowPct(), 60);
    EXPECT_EQ(b.at(1).feel, 5);
    EXPECT_EQ(json(b), s);
}

TEST(ReviewLog, ADayHasOneEntryAndARerunReplacesItKeepingTheFeel)
{
    ReviewLog l;
    l.record(entry(20261117u, 80, 10, -4, 3));
    l.record(entry(20261117u, 55, 40, -6, 0));
    ASSERT_EQ(l.count(), 1);
    EXPECT_EQ(l.at(0).inPct, 55);
    EXPECT_EQ(l.at(0).feel, 3);   // the answer already given stands
    l.record(entry(20261117u, 70, 20, -2, 5));
    EXPECT_EQ(l.at(0).feel, 5);   // unless a new one is given
}

TEST(ReviewLog, EntriesStayInDateOrder)
{
    ReviewLog l;
    l.record(entry(20261120u));
    l.record(entry(20261118u));
    l.record(entry(20261119u));
    ASSERT_EQ(l.count(), 3);
    EXPECT_EQ(l.at(0).date, 20261118u);
    EXPECT_EQ(l.at(1).date, 20261119u);
    EXPECT_EQ(l.at(2).date, 20261120u);
    EXPECT_NE(l.find(20261119u), nullptr);
    EXPECT_EQ(l.find(20261101u), nullptr);
}

TEST(ReviewLog, TheOldestIsDroppedWhenFull)
{
    ReviewLog l;
    for (int i = 0; i < ReviewLog::kMax + 5; ++i) {
        l.record(entry(Date::addDays(20261001u, i)));
    }
    ASSERT_EQ(l.count(), ReviewLog::kMax);
    EXPECT_EQ(l.at(0).date, Date::addDays(20261001u, 5));
    EXPECT_EQ(l.at(ReviewLog::kMax - 1).date, Date::addDays(20261001u, ReviewLog::kMax + 4));
    // Something older than all of them is not kept.
    l.record(entry(20260101u));
    EXPECT_EQ(l.find(20260101u), nullptr);
}

TEST(ReviewLog, AFullLogFitsItsBuffer)
{
    ReviewLog l;
    for (int i = 0; i < ReviewLog::kMax; ++i) {
        ReviewEntry e = entry(Date::addDays(20261101u, i), 100, 0, -600, 5);
        e.gapSecKm  = -600;
        e.fadeSecKm = -100;
        e.judgedSec = 65535;
        l.record(e);
    }
    l.handled = 20261201u;
    const std::string s = json(l);
    ASSERT_FALSE(s.empty());
    EXPECT_LE(s.size(), ReviewLog::kMaxJsonBytes);
    ReviewLog b;
    EXPECT_TRUE(parse(b, s));
    EXPECT_EQ(b.count(), ReviewLog::kMax);
}

TEST(ReviewLog, AFeelCanBeSetAfterwardsOnlyForAKnownDayAndAValidAnswer)
{
    ReviewLog l;
    l.record(entry(20261117u));
    EXPECT_TRUE(l.setFeel(20261117u, 4));
    EXPECT_EQ(l.at(0).feel, 4);
    EXPECT_FALSE(l.setFeel(20261118u, 4));
    EXPECT_FALSE(l.setFeel(20261117u, kFeelCount));
    EXPECT_EQ(l.at(0).feel, 4);
}

TEST(ReviewLog, AnUnjudgedSessionKeepsOnlyItsFeel)
{
    Result r;   // not judged
    r.inPct = 90;   // must not leak through
    const ReviewEntry e = ReviewEntry::from(20261117u, Kind::Easy, r, 3);
    EXPECT_FALSE(e.judged());
    EXPECT_EQ(e.inPct, 0);
    EXPECT_EQ(e.feel, 3);
    EXPECT_EQ(e.dir(), Dir::None);
    EXPECT_FALSE(e.offPace());
    ReviewLog l;
    l.record(e);
    ReviewLog b;
    ASSERT_TRUE(parse(b, json(l)));
    EXPECT_FALSE(b.at(0).judged());
    EXPECT_EQ(b.at(0).feel, 3);
}

TEST(ReviewLog, AJudgedResultIsCopiedIn)
{
    Result r;
    r.judged = true;
    r.judgedSec = 700;
    r.inPct = 50;
    r.fastPct = 30;
    r.slowPct = 20;
    r.gapSecKm = -7;
    r.fadeSecKm = 5;
    const ReviewEntry e = ReviewEntry::from(20261117u, Kind::Tempo, r);
    EXPECT_TRUE(e.judged());
    EXPECT_EQ(e.inPct, 50);
    EXPECT_EQ(e.slowPct(), 20);
    EXPECT_EQ(e.dir(), Dir::Fast);
    EXPECT_TRUE(e.offPace());
}

TEST(ReviewLog, BadFilesAreRefusedAndLeaveTheLogEmpty)
{
    ReviewLog l;
    l.record(entry(20261117u));
    const char* bad[] = {
        "",
        "not json",
        "{}",
        "{\"v\":2,\"e\":[]}",
        "{\"v\":1}",
        "{\"v\":1,\"e\":[[20261117,3,80,10,596,102,900]]}",           // 7 fields
        "{\"v\":1,\"e\":[[20261317,3,80,10,596,102,900,0]]}",           // a month 13
        "{\"v\":1,\"e\":[[20261117,99,80,10,596,102,900,0]]}",          // a kind that is not
        "{\"v\":1,\"e\":[[20261117,3,101,0,596,102,900,0]]}",           // over 100
        "{\"v\":1,\"e\":[[20261117,3,80,30,596,102,900,0]]}",           // in + fast over 100
        "{\"v\":1,\"e\":[[20261117,3,80,10,1300,102,900,0]]}",        // a gap past 600
        "{\"v\":1,\"e\":[[20261117,3,80,10,596,300,900,0]]}",         // a fade past 100
        "{\"v\":1,\"e\":[[20261117,3,80,10,-4,102,900,0]]}",          // a negative number
        "{\"v\":1,\"e\":[[20261117,3,80,10,596,102,900,9]]}",           // a feel that is not
        "{\"v\":1,\"e\":[[20261118,3,80,10,596,102,900,0],[20261117,3,80,10,596,102,900,0]]}",   // not in order
        "{\"v\":1,\"e\":[[20261117,3,80,10,596,102,900,0],[20261117,3,80,10,596,102,900,0]]}",   // a day twice
        "{\"v\":1,\"eh\":20261399,\"e\":[]}",
    };
    for (const char* s : bad) {
        ReviewLog r = l;
        EXPECT_FALSE(r.fromJson(s, std::strlen(s))) << s;
        EXPECT_EQ(r.count(), 0) << s;
    }
    // Too many entries.
    std::string many = "{\"v\":1,\"e\":[";
    for (int i = 0; i < ReviewLog::kMax + 1; ++i) {
        char row[64];
        std::snprintf(row, sizeof(row), "%s[%u,3,80,10,596,102,900,0]", i ? "," : "", static_cast<unsigned>(Date::addDays(20261001u, i)));
        many += row;
    }
    many += "]}";
    ReviewLog r;
    EXPECT_FALSE(parse(r, many));
    // Too long altogether.
    EXPECT_FALSE(r.fromJson(std::string(ReviewLog::kMaxJsonBytes + 1, ' ').c_str(), ReviewLog::kMaxJsonBytes + 1));
}

TEST(ReviewLog, ToJsonRefusesABufferThatIsTooSmall)
{
    ReviewLog l;
    l.record(entry(20261117u));
    char tiny[16];
    EXPECT_EQ(l.toJson(tiny, sizeof(tiny)), 0u);
    EXPECT_EQ(l.toJson(nullptr, 10), 0u);
}

TEST(ReviewLog, ClearEmptiesTheLogAndTheAnswerDate)
{
    ReviewLog l;
    l.record(entry(20261117u));
    l.handled = 20261117u;
    l.clear();
    EXPECT_EQ(l.count(), 0);
    EXPECT_EQ(l.handled, 0u);
    EXPECT_EQ(l.at(0).date, 0u);   // out of range: empty, never a stray read
}
