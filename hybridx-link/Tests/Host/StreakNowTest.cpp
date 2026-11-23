// The Streak line: Streak's summary and the projection the activity apps make (StreakNow).

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "LoadFormText.hpp"
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

// -- Form after a save (Streak NOTES "S10") -----------------------------------------------------

namespace
{

/// Streak's load window after three weeks of a session a day at 50, saved on Tuesday at 20:00.
StreakNow tuesdayWithLoad()
{
    StreakNow s = tuesday();
    Streak::LoadState l;
    Streak::LoadHistory h(l);
    for (int32_t d = kMon - 20; d <= kMon + 1; ++d) {
        h.advanceTo(d);
        h.add(d, 50);
    }
    s.load    = l;
    s.hasLoad = true;
    s.at      = static_cast<uint32_t>(kMon + 1) * kDay + 20u * 3600u;
    return s;
}

Entry loaded(int32_t day, unsigned hour, unsigned minutes, uint16_t load, Sport sport = Sport::Run)
{
    Entry e = entry(day, hour, minutes, sport);
    e.load  = load;
    return e;
}

}  // namespace

TEST(StreakNowForm, TheLoadWindowRoundTripsAndIsOptional)
{
    const StreakNow s = tuesdayWithLoad();
    char buf[kStreakNowMaxBytes];
    const size_t len = encodeStreakNow(s, buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    EXPECT_NE(std::string(buf, len).find("\"lw\":["), std::string::npos);
    StreakNow back;
    ASSERT_TRUE(decodeStreakNow(buf, len, back));
    ASSERT_TRUE(back.hasLoad);
    EXPECT_EQ(back.at, s.at);
    EXPECT_EQ(back.load.firstDay, s.load.firstDay);
    EXPECT_EQ(back.load.openStart, s.load.openStart);
    EXPECT_EQ(back.load.fitnessX100, s.load.fitnessX100);
    EXPECT_EQ(back.load.fatigueX100, s.load.fatigueX100);
    EXPECT_EQ(back.load.seeded, s.load.seeded);
    for (uint8_t i = 0; i < Streak::kOpenDays; ++i) {
        EXPECT_EQ(back.load.daily[i], s.load.daily[i]);
    }

    // Without it (a Streak from before S10): the line as before, no form.
    const size_t old = encodeStreakNow(tuesday(), buf, sizeof(buf));
    ASSERT_TRUE(decodeStreakNow(buf, old, back));
    EXPECT_FALSE(back.hasLoad);

    // A broken window: dropped, the rest kept.
    std::string text(buf, encodeStreakNow(s, buf, sizeof(buf)));
    const size_t at = text.find("\"lw\":[");
    text.replace(at, text.find("]]", at) + 2 - at, "\"lw\":[1,2]");
    ASSERT_TRUE(decodeStreakNow(text.c_str(), text.size(), back));
    EXPECT_FALSE(back.hasLoad);
    EXPECT_EQ(back.counted, 2);
}

TEST(StreakNowForm, TheWidestSummaryFits)
{
    StreakNow s = tuesdayWithLoad();
    s.keyCount  = kMaxKeys;
    for (uint8_t i = 0; i < kMaxKeys; ++i) {
        s.keys[i] = {0xFFFFFFFFu, 0xFFFFFFFFu};
    }
    s.at                = 0xFFFFFFFFu;
    s.load.fitnessX100  = 0xFFFFu * 100u;
    s.load.fatigueX100  = 0xFFFFu * 100u;
    for (uint8_t i = 0; i < Streak::kOpenDays; ++i) {
        s.load.daily[i] = 0xFFFF;
    }
    s.streak = 65535;
    char buf[kStreakNowMaxBytes];
    const size_t len = encodeStreakNow(s, buf, sizeof(buf));
    EXPECT_GT(len, 0u) << "the widest summary must fit kStreakNowMaxBytes";
    std::printf("[ widest ] streak_now.json: %zu bytes of %zu\n", len, kStreakNowMaxBytes);
}

TEST(StreakNowForm, FormIsStreaksModelWithTheNewSessions)
{
    // Wednesday: a ride this morning (the session just saved) and a run last night after Streak's
    // save; the run and race Streak listed are not counted again.
    const StreakNow now = tuesdayWithLoad();
    StreakProjector p(now, kMon + 2);
    p.add(file("HybridXRun", {loaded(kMon, 7, 40, 50), loaded(kMon + 1, 21, 45, 60)}));
    p.add(file("HybridXRace", {loaded(kMon + 1, 18, 70, 120)}));
    p.add(file("HybridXRide", {loaded(kMon + 2, 7, 90, 85)}));
    const StreakLine l = p.result();
    ASSERT_TRUE(l.hasForm);

    // The same by hand: Streak's window, plus the two new loads, to today.
    Streak::LoadState   copy = now.load;
    Streak::LoadHistory h(copy);
    h.advanceTo(kMon + 2);
    h.add(kMon + 1, 60);
    h.add(kMon + 2, 85);
    const Streak::LoadFigures f = h.figures(kMon + 2);
    const int32_t form = f.formX100 >= 0 ? (f.formX100 + 50) / 100 : -((-f.formX100 + 50) / 100);
    EXPECT_EQ(l.form, form);
    EXPECT_EQ(l.formWord, static_cast<uint8_t>(f.word));
    EXPECT_LT(l.form, 0);   // two hard days on top of a steady block: fatigue leads

    char text[32];
    ASSERT_GT(formatStreakForm(l, text, sizeof(text)), 0u);
    EXPECT_EQ(std::string(text).rfind("Form ", 0), 0u) << text;   // "Form -18, building": the long form passes 21
}

TEST(StreakNowForm, WhatStreakCountedIsNotCountedAgain)
{
    const StreakNow now = tuesdayWithLoad();
    // Only sessions Streak had: form is Streak's own window moved to today.
    StreakProjector p(now, kMon + 1);
    p.add(file("HybridXRun", {loaded(kMon, 7, 40, 50)}));
    p.add(file("HybridXRace", {loaded(kMon + 1, 18, 70, 120)}));
    // Last week, before the save: Streak had it (it is not in k, which lists this week only).
    p.add(file("HybridXTrail", {loaded(kMon - 3, 9, 60, 70)}));
    // The same new session offered twice (a file and its backup, say): once.
    p.add(file("HybridXRide", {loaded(kMon + 1, 21, 60, 40)}));
    p.add(file("HybridXRide", {loaded(kMon + 1, 21, 60, 40)}));
    const StreakLine l = p.result();
    ASSERT_TRUE(l.hasForm);
    Streak::LoadState   copy = now.load;
    Streak::LoadHistory h(copy);
    h.advanceTo(kMon + 1);
    h.add(kMon + 1, 40);
    const Streak::LoadFigures f = h.figures(kMon + 1);
    const int32_t form = f.formX100 >= 0 ? (f.formX100 + 50) / 100 : -((-f.formX100 + 50) / 100);
    EXPECT_EQ(l.form, form);
}

TEST(StreakNowForm, NoFormWithoutTheWindowOrWhileLearning)
{
    StreakProjector none(tuesday(), kMon + 2);
    none.add(file("HybridXRide", {loaded(kMon + 2, 7, 90, 85)}));
    EXPECT_FALSE(none.result().hasForm);
    char text[32] = "x";
    EXPECT_EQ(formatStreakForm(none.result(), text, sizeof(text)), 0u);
    EXPECT_STREQ(text, "");

    // A first week: Streak's window holds three days.
    StreakNow young = tuesday();
    Streak::LoadHistory h(young.load);
    for (int32_t d = kMon - 1; d <= kMon + 1; ++d) {
        h.advanceTo(d);
        h.add(d, 40);
    }
    young.hasLoad = true;
    young.at      = static_cast<uint32_t>(kMon + 1) * kDay + 20u * 3600u;
    StreakProjector p(young, kMon + 2);
    p.add(file("HybridXRide", {loaded(kMon + 2, 7, 90, 85)}));
    EXPECT_FALSE(p.result().hasForm);
}

TEST(StreakNowForm, TheWordsFitTheirLine)
{
    char b[48];
    for (uint8_t w = static_cast<uint8_t>(Streak::FormWord::Fresh); w <= static_cast<uint8_t>(Streak::FormWord::Overreaching);
         ++w) {
        for (int16_t form : {int16_t(-9999), int16_t(-130), int16_t(0), int16_t(12), int16_t(9999)}) {
            Streak::formatFormNow(b, sizeof(b), form, static_cast<Streak::FormWord>(w));
            EXPECT_LE(std::strlen(b), 21u) << b;
            // Workout and Ride: the load's small label, low on the disc (Streak NOTES "S10").
            Streak::formatFormNow(b, sizeof(b), form, static_cast<Streak::FormWord>(w), 18);
            EXPECT_LE(std::strlen(b), 18u) << b;
        }
    }
    Streak::formatFormNow(b, sizeof(b), 12, Streak::FormWord::Fresh);
    EXPECT_STREQ(b, "Form now +12, fresh");
    Streak::formatFormNow(b, sizeof(b), -130, Streak::FormWord::Overreaching);
    EXPECT_STREQ(b, "Form -130, ease off");
    Streak::formatFormNow(b, sizeof(b), 12, Streak::FormWord::Fresh, 18);
    EXPECT_STREQ(b, "Form +12, fresh");
    Streak::formatFormNow(b, sizeof(b), -130, Streak::FormWord::Building, 18);
    EXPECT_STREQ(b, "Form now -130");   // the word gives way before the number

    // Through the line, as the apps call it.
    StreakLine l{};
    l.show     = true;
    l.hasForm  = true;
    l.form     = 4;
    l.formWord = static_cast<uint8_t>(Streak::FormWord::Fresh);
    EXPECT_GT(formatStreakForm(l, b, sizeof(b)), 0u);
    EXPECT_STREQ(b, "Form now +4, fresh");
    EXPECT_GT(formatStreakForm(l, b, sizeof(b), 18), 0u);
    EXPECT_STREQ(b, "Form now +4, fresh");   // 18 characters: it fits as it is
    l.form = 14;
    EXPECT_GT(formatStreakForm(l, b, sizeof(b), 18), 0u);
    EXPECT_STREQ(b, "Form +14, fresh");
}
