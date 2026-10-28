// The Streak line: Streak's summary and the projection the activity apps make (StreakNow).

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "StreakNow.hpp"

using namespace HybridXLink;

namespace
{

constexpr int32_t kMon = 20752;   // Monday 26 October 2026, days since 1970
constexpr uint32_t kDay = 86400u;

/// An entry that started at @p hour on day @p day (UTC, zone 0) and lasted @p minutes.
Entry entry(int32_t day, unsigned hour, unsigned minutes, Sport sport = Sport::Run)
{
    Entry e;
    e.startUtc    = static_cast<uint32_t>(day) * kDay + hour * 3600u;
    e.tzOffsetMin = 0;
    e.durationSec = minutes * 60u;
    e.sport       = sport;
    return e;
}

ActivityFile file(const char* app, std::initializer_list<Entry> es)
{
    ActivityFile f;
    std::strncpy(f.app, app, sizeof(f.app) - 1);
    for (const Entry& e : es) {
        f.entries[f.count++] = e;
    }
    return f;
}

/// Streak saved on Tuesday: target 4, two counted (a run Monday, a race Tuesday), a 7-week streak.
StreakNow tuesday()
{
    StreakNow s;
    s.day        = kMon + 1;
    s.weekStart  = kMon;
    s.target     = 4;
    s.minMinutes = 10;
    s.counted    = 2;
    s.daysMask   = 0b011;
    s.streak     = 7;
    s.keyCount   = 2;
    s.keys[0]    = {appKey("HybridXRun"), entry(kMon, 7, 40).localStart()};
    s.keys[1]    = {appKey("HybridXRace"), entry(kMon + 1, 18, 70).localStart()};
    return s;
}

}  // namespace

TEST(StreakNow, TheSummaryRoundTrips)
{
    const StreakNow s = tuesday();
    char buf[kStreakNowMaxBytes];
    const size_t len = encodeStreakNow(s, buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    StreakNow r;
    ASSERT_TRUE(decodeStreakNow(buf, len, r));
    EXPECT_EQ(r.day, s.day);
    EXPECT_EQ(r.weekStart, s.weekStart);
    EXPECT_EQ(r.target, 4);
    EXPECT_EQ(r.minMinutes, 10);
    EXPECT_EQ(r.scope, kScopeAny);
    EXPECT_EQ(r.counted, 2);
    EXPECT_EQ(r.daysMask, 0b011);
    EXPECT_EQ(r.streak, 7);
    ASSERT_EQ(r.keyCount, 2);
    EXPECT_EQ(r.keys[1].appKey, s.keys[1].appKey);
    EXPECT_EQ(r.keys[1].localStart, s.keys[1].localStart);
}

TEST(StreakNow, TheFullestSummaryFits)
{
    StreakNow s = tuesday();
    s.keyCount = kMaxKeys;
    for (uint8_t i = 0; i < kMaxKeys; ++i) {
        s.keys[i] = {0xFFFFFFFFu, 0xFFFFFFFFu};
    }
    s.streak = 65535;
    char buf[kStreakNowMaxBytes];
    EXPECT_GT(encodeStreakNow(s, buf, sizeof(buf)), 0u);
}

TEST(StreakNow, ABadSummaryReadsAsNone)
{
    const char* bad[] = {
        "",
        "nonsense",
        R"({"v":2,"day":20755,"ws":20754,"t":4,"min":10,"sc":255,"opd":0,"pend":0,"n":2,"days":3,"st":7,"k":[]})",
        R"({"v":1,"day":20765,"ws":20754,"t":4,"min":10,"sc":255,"opd":0,"pend":0,"n":2,"days":3,"st":7,"k":[]})",
        R"({"v":1,"day":20755,"ws":20754,"t":0,"min":10,"sc":255,"opd":0,"pend":0,"n":2,"days":3,"st":7,"k":[]})",
        R"({"v":1,"day":20755,"ws":20754,"t":4,"min":10,"sc":9,"opd":0,"pend":0,"n":2,"days":3,"st":7,"k":[]})",
        R"({"v":1,"day":20755,"ws":20754,"t":4,"min":10,"sc":255,"opd":0,"pend":0,"n":2,"days":3,"st":7,"k":[[1]]})",
        R"({"v":1,"day":20755,"ws":20754,"t":4,"min":10,"sc":255,"opd":0,"pend":0,"n":2,"days":3,"st":-1,"k":[]})",
    };
    for (const char* t : bad) {
        StreakNow s = tuesday();
        EXPECT_FALSE(decodeStreakNow(t, std::strlen(t), s)) << t;
        EXPECT_EQ(s.target, 0) << t;
    }
}

TEST(StreakProjector, TheRunJustSavedCountsOnceAndWhatStreakHasIsNotCountedAgain)
{
    // Wednesday: Run's file holds Monday's run (Streak has it) and today's (it does not).
    StreakProjector p(tuesday(), kMon + 2);
    p.add(file("HybridXRun", {entry(kMon, 7, 40), entry(kMon + 2, 7, 35)}));
    p.add(file("HybridXRace", {entry(kMon + 1, 18, 70)}));
    const StreakLine l = p.result();
    ASSERT_TRUE(l.show);
    EXPECT_EQ(l.done, 3);
    EXPECT_EQ(l.target, 4);
    EXPECT_TRUE(l.hasStreak);
    EXPECT_EQ(l.streak, 7);   // not met yet
}

TEST(StreakProjector, MeetingTheTargetAddsThisWeekToTheStreak)
{
    StreakProjector p(tuesday(), kMon + 3);
    p.add(file("HybridXRun", {entry(kMon + 2, 7, 35)}));
    p.add(file("HybridXTrail", {entry(kMon + 3, 8, 50)}));
    const StreakLine l = p.result();
    EXPECT_EQ(l.done, 4);
    EXPECT_EQ(l.streak, 8);
    char a[32], b[24];
    ASSERT_TRUE(formatStreakLine(l, a, sizeof(a), b, sizeof(b)));
    EXPECT_STREQ(a, "4 of 4 this week");
    EXPECT_STREQ(b, "8-week streak");
}

TEST(StreakProjector, StreaksRulesApply)
{
    StreakNow s = tuesday();
    s.onePerDay = true;
    s.scope     = 0;   // runs only
    StreakProjector p(s, kMon + 3);
    p.add(file("HybridXRun", {entry(kMon + 1, 6, 30),     // Tuesday: a day that already counts
                              entry(kMon + 2, 6, 9),      // under the 10-minute minimum
                              entry(kMon + 2, 18, 30),    // counts
                              entry(kMon + 2, 20, 30)})); // the same day again
    p.add(file("HybridXRace", {entry(kMon + 3, 9, 60, Sport::Hybrid)}));   // out of scope
    p.add(file("HybridXRun", {entry(kMon + 2, 18, 30)}));                  // offered twice
    EXPECT_EQ(p.result().done, 3);
}

TEST(StreakProjector, OtherWeeksDoNotCount)
{
    StreakProjector p(tuesday(), kMon + 2);
    p.add(file("HybridXRun", {entry(kMon - 1, 7, 40), entry(kMon + 7, 7, 40)}));
    EXPECT_EQ(p.result().done, 2);
}

TEST(StreakProjector, TheWeekAfterAMetWeekCarriesTheStreak)
{
    StreakNow s = tuesday();
    s.counted = 4;   // met when Streak last saved
    StreakProjector p(s, kMon + 8);   // next Tuesday
    p.add(file("HybridXRun", {entry(kMon + 2, 7, 40), entry(kMon + 7, 7, 40), entry(kMon + 8, 7, 40)}));
    const StreakLine l = p.result();
    EXPECT_EQ(l.done, 2);
    EXPECT_EQ(l.target, 4);
    EXPECT_TRUE(l.hasStreak);
    EXPECT_EQ(l.streak, 8);
}

TEST(StreakProjector, AnUnknownWeekShowsOnlyTheCount)
{
    // Last week not met (Streak decides misses and shields), or two weeks on: no streak.
    StreakProjector next(tuesday(), kMon + 8);
    next.add(file("HybridXRun", {entry(kMon + 8, 7, 40)}));
    StreakLine l = next.result();
    EXPECT_EQ(l.done, 1);
    EXPECT_FALSE(l.hasStreak);

    StreakNow s = tuesday();
    s.counted = 4;
    StreakProjector later(s, kMon + 15);
    l = later.result();
    EXPECT_TRUE(l.show);
    EXPECT_EQ(l.done, 0);
    EXPECT_FALSE(l.hasStreak);

    // A goal change waiting: no target in a new week.
    s.pending = true;
    StreakProjector pend(s, kMon + 8);
    pend.add(file("HybridXRun", {entry(kMon + 8, 7, 40)}));
    l = pend.result();
    EXPECT_EQ(l.target, 0);
    EXPECT_FALSE(l.hasStreak);
    char a[32], b[24];
    ASSERT_TRUE(formatStreakLine(l, a, sizeof(a), b, sizeof(b)));
    EXPECT_STREQ(a, "1 this week");
    EXPECT_STREQ(b, "");
}

TEST(StreakProjector, NoSummaryNoLine)
{
    StreakNow none;   // decode failed or no file
    StreakProjector p(none, kMon);
    p.add(file("HybridXRun", {entry(kMon, 7, 40)}));
    EXPECT_FALSE(p.result().show);
    char a[32] = "x", b[24] = "x";
    EXPECT_FALSE(formatStreakLine(p.result(), a, sizeof(a), b, sizeof(b)));
    EXPECT_STREQ(a, "");
    // A clock gone back before the summary's week: no line either.
    StreakProjector back(tuesday(), kMon - 3);
    EXPECT_FALSE(back.result().show);
}

TEST(StreakNow, TheKeyAndKindsAreStreaks)
{
    // FNV-1a, as Streak's ActivityScanner::appKey (its value for "HybridXRace" in Streak's tests).
    EXPECT_EQ(appKey(""), 2166136261u);
    EXPECT_NE(appKey("HybridXRun"), appKey("HybridXRace"));
    EXPECT_EQ(kindCode(Sport::Run), 0);
    EXPECT_EQ(kindCode(Sport::Hybrid), 5);
    EXPECT_EQ(kindCode(Sport::Other), 7);
}
