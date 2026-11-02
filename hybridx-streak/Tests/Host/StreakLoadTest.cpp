/**
 * Host tests for training load in HybridX Streak: the sessions' loads into the model's
 * LoadState (once each, whatever the streak's rules), HomeView's form, and the state
 * file's optional "L".
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "StateCodec.hpp"
#include "StreakModel.hpp"
#include "WeekMath.hpp"

using namespace Streak;

namespace
{

int32_t day(int y, int m, int d)
{
    return WeekMath::daysFromCivil(y, static_cast<uint32_t>(m), static_cast<uint32_t>(d));
}

const int32_t kMon = day(2026, 9, 21);   // a Monday

Found session(int32_t d, uint16_t load, uint16_t minutes = 30, const char* app = "HybridXRide", int hour = 8)
{
    Found f;
    std::strncpy(f.app, app, sizeof(f.app) - 1);
    f.appKey     = ActivityScanner::appKey(app);
    f.localDay   = d;
    f.localStart = static_cast<uint32_t>(d) * 86400u + static_cast<uint32_t>(hour) * 3600u;
    f.kind       = Kind::Ride;
    f.minutes    = minutes;
    f.load       = load;
    return f;
}

void open(StreakModel& m, int32_t d, std::vector<Found> found = {})
{
    Events ev;
    m.update(d, found.data(), found.size(), ev);
}

StreakModel fresh(uint8_t minMinutes = 0)
{
    StreakModel m;
    Goal g;
    g.minMinutes = minMinutes;
    m.reset(g);
    return m;
}

} // namespace

TEST(StreakLoad, ASessionsLoadIsKeptOnItsDayOnce)
{
    StreakModel m = fresh();
    const Found s = session(kMon, 40);
    open(m, kMon, {s});
    open(m, kMon, {s});            // found again on the next open: not counted twice
    LoadHistory h(m.state().load);
    EXPECT_EQ(h.dayLoad(kMon), 40);
}

TEST(StreakLoad, TheStreaksRulesDoNotStopTheLoad)
{
    // A 5-minute session under a 10-minute minimum does not count for the streak,
    // but the body did the work: its load counts.
    StreakModel m = fresh(10);
    open(m, kMon, {session(kMon, 12, 5)});
    LoadHistory h(m.state().load);
    EXPECT_EQ(h.dayLoad(kMon), 12);
    EXPECT_EQ(m.view(kMon).sessions, 0);
}

TEST(StreakLoad, ASessionFromAJudgedWeekStillAddsItsLoad)
{
    StreakModel m = fresh();
    open(m, kMon);
    open(m, kMon + 8);                           // the first week is judged
    open(m, kMon + 9, {session(kMon + 3, 30)});  // found late, from that week
    LoadHistory h(m.state().load);
    EXPECT_EQ(h.dayLoad(kMon + 3), 30);
}

TEST(StreakLoad, TheLoadOutlivesTheWeek)
{
    StreakModel m = fresh();
    for (int d = 0; d < 21; ++d) {
        open(m, kMon + d, {session(kMon + d, 50, 30, "HybridXRide", 7)});
    }
    const HomeView v = m.view(kMon + 20);
    EXPECT_NE(v.formWord, static_cast<uint8_t>(FormWord::Learning));
    EXPECT_EQ(v.formWord, static_cast<uint8_t>(FormWord::Steady));   // a steady 50 a day
    EXPECT_GE(v.form, -5);
    EXPECT_LE(v.form, 5);
}

TEST(StreakLoad, TheFirstTwoWeeksAreLearning)
{
    StreakModel m = fresh();
    open(m, kMon, {session(kMon, 80)});
    EXPECT_EQ(m.view(kMon).formWord, static_cast<uint8_t>(FormWord::Learning));
}

TEST(StreakLoad, TheStateFileKeepsTheLoad)
{
    StreakModel m = fresh();
    for (int d = 0; d < 16; ++d) {
        open(m, kMon + d, {session(kMon + d, static_cast<uint16_t>(20 + d), 30, "HybridXRun", 7)});
    }
    const State& s = m.state();
    ASSERT_TRUE(s.load.started);
    char buf[StateCodec::kMaxBytes];
    const size_t len = StateCodec::encode(s, buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    EXPECT_NE(std::string(buf, len).find("\"L\":[1,"), std::string::npos);
    State back;
    ASSERT_TRUE(StateCodec::decode(buf, len, back));
    EXPECT_TRUE(back.load.started);
    EXPECT_EQ(back.load.firstDay, s.load.firstDay);
    EXPECT_EQ(back.load.openStart, s.load.openStart);
    EXPECT_EQ(back.load.fitnessX100, s.load.fitnessX100);
    EXPECT_EQ(back.load.fatigueX100, s.load.fatigueX100);
    EXPECT_EQ(back.load.seeded, s.load.seeded);
    for (uint8_t i = 0; i < kOpenDays; ++i) {
        EXPECT_EQ(back.load.daily[i], s.load.daily[i]) << int(i);
    }
}

TEST(StreakLoad, AnOlderFileWithoutTheLoadStillReads)
{
    StreakModel m = fresh();
    open(m, kMon, {session(kMon, 0)});
    State s = m.state();
    s.load = LoadState();                // as a file from before 2 November 2026
    char buf[StateCodec::kMaxBytes];
    const size_t len = StateCodec::encode(s, buf, sizeof(buf));
    ASSERT_EQ(std::string(buf, len).find("\"L\""), std::string::npos);
    State back;
    ASSERT_TRUE(StateCodec::decode(buf, len, back));
    EXPECT_FALSE(back.load.started);
    EXPECT_EQ(back.sessionCount, s.sessionCount);
}

TEST(StreakLoad, ABrokenLoadIsDroppedAndTheStreakKept)
{
    StreakModel m = fresh();
    open(m, kMon, {session(kMon, 30)});
    char buf[StateCodec::kMaxBytes];
    size_t len = StateCodec::encode(m.state(), buf, sizeof(buf));
    std::string text(buf, len);
    const size_t at = text.find("\"L\":[");
    ASSERT_NE(at, std::string::npos);
    const size_t end = text.find("]]", at);
    ASSERT_NE(end, std::string::npos);
    text.replace(at, end + 2 - at, "\"L\":[1,2,3]");   // too short
    State back;
    ASSERT_TRUE(StateCodec::decode(text.c_str(), text.size(), back));
    EXPECT_FALSE(back.load.started);
    EXPECT_EQ(back.sessionCount, 1);
}
