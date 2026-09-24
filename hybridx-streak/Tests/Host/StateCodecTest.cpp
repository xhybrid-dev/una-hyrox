/**
 * Host tests for the state file (PLAN 6.5): JSON round trip, a truncated or
 * corrupt file, the .bak fallback of the SDK's save sequence, and clamping of
 * hand-edited values.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "SafeFile.hpp"
#include "StateCodec.hpp"
#include "StreakModel.hpp"
#include "TreeFileSystem.hpp"
#include "WeekMath.hpp"

using namespace Streak;

namespace
{

const int32_t kMon = WeekMath::daysFromCivil(2026, 9, 21);

Found session(int32_t d, int hour = 8, const char* app = "Running", Kind kind = Kind::Run)
{
    Found f;
    std::strncpy(f.app, app, sizeof(f.app) - 1);
    f.appKey     = ActivityScanner::appKey(app);
    f.localDay   = d;
    f.localStart = static_cast<uint32_t>(d) * 86400u + static_cast<uint32_t>(hour) * 3600u;
    f.kind       = kind;
    f.minutes    = 42;
    return f;
}

/// A state with something in every field: weeks, a pending miss, sessions of
/// several apps, a manual log, a full ring and wrapped history.
StreakModel busyModel()
{
    StreakModel m;
    Goal        g;
    g.target = 4;
    m.reset(g);
    Events ev;
    m.update(kMon, nullptr, 0, ev);
    for (int w = 0; w < 60; ++w) {
        const int32_t      start = kMon + 7 * w;
        std::vector<Found> f;
        if (w % 9 != 8) {
            for (int i = 0; i < 4; ++i) {
                f.push_back(session(start + i, 7 + i, i % 2 ? "Cycling" : "Running", i % 2 ? Kind::Ride : Kind::Run));
            }
        }
        Events e;
        m.update(start + 6, f.data(), f.size(), e);
        m.decideShields(true, e);
    }
    Events e;
    m.update(kMon + 7 * 60, nullptr, 0, e);
    m.update(kMon + 7 * 61 + 2, nullptr, 0, e);   // a missed week, left undecided
    const std::vector<Found> now = {session(kMon + 7 * 61, 6, "HybridXRace", Kind::Hybrid),
                                    session(kMon + 7 * 61 + 1, 18, "Workout", Kind::Workout)};
    m.update(kMon + 7 * 61 + 2, now.data(), now.size(), e);
    m.logManual(Kind::Row, false, kMon + 7 * 61 + 2, 3600, e);
    m.toggleExclude(1, e);
    Goal next = m.state().goal;
    next.target = 5;
    m.setGoal(next, kMon + 7 * 61 + 2, e);
    return m;
}

void expectSame(const State& a, const State& b)
{
    char ea[StateCodec::kMaxBytes];
    char eb[StateCodec::kMaxBytes];
    ASSERT_GT(StateCodec::encode(a, ea, sizeof(ea)), 0u);
    ASSERT_GT(StateCodec::encode(b, eb, sizeof(eb)), 0u);
    EXPECT_STREQ(ea, eb);
}

} // namespace

TEST(StateCodec, RoundTripsABusyState)
{
    const StreakModel m = busyModel();
    const State&      s = m.state();
    ASSERT_EQ(s.ringCount, State::kRing);
    ASSERT_EQ(s.historyCount, State::kHistory);
    ASSERT_GT(s.sessionCount, 2);
    ASSERT_TRUE(s.hasPending);

    char         buf[StateCodec::kMaxBytes];
    const size_t len = StateCodec::encode(s, buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    EXPECT_LT(len, 4096u) << "the state file should stay under 4 KB (PLAN 6.5)";

    State back;
    ASSERT_TRUE(StateCodec::decode(buf, len, back));
    expectSame(s, back);
    EXPECT_EQ(back.streak, s.streak);
    EXPECT_EQ(back.period, s.period);
    EXPECT_EQ(back.sessions[1].flags, s.sessions[1].flags);
    EXPECT_STREQ(back.apps[0], s.apps[0]);
    EXPECT_EQ(back.history[5].outcome, s.history[5].outcome);
}

TEST(StateCodec, AFreshStateRoundTrips)
{
    State s;
    char  buf[StateCodec::kMaxBytes];
    const size_t len = StateCodec::encode(s, buf, sizeof(buf));
    State back;
    back.streak = 99;
    ASSERT_TRUE(StateCodec::decode(buf, len, back));
    EXPECT_EQ(back.period, State::kNoPeriod);
    EXPECT_EQ(back.streak, 0);
}

TEST(StateCodec, RefusesTruncatedCorruptAndForeignFiles)
{
    const StreakModel m = busyModel();
    char              buf[StateCodec::kMaxBytes];
    const size_t      len = StateCodec::encode(m.state(), buf, sizeof(buf));
    State             s;
    s.streak = 7;
    EXPECT_FALSE(StateCodec::decode(buf, len / 2, s));
    std::string bad(buf, len);
    bad[len / 3] = '}';
    EXPECT_FALSE(StateCodec::decode(bad.data(), bad.size(), s));
    const char* other = "{\"v\":9,\"g\":[3,1,255,10,0]}";
    EXPECT_FALSE(StateCodec::decode(other, std::strlen(other), s));
    EXPECT_FALSE(StateCodec::decode("", 0, s));
    EXPECT_EQ(s.streak, 7) << "a failed decode leaves the state alone";
}

TEST(StateCodec, ClampsHandEditedValues)
{
    State s;
    s.period       = 3000;
    s.sessionCount = 1;
    s.sessions[0].kind = Kind::Run;
    char buf[StateCodec::kMaxBytes];
    std::string json(buf, StateCodec::encode(s, buf, sizeof(buf)));
    // Edit: shields 9, kind 99, app 77, target 12.
    auto replace = [&json](const std::string& from, const std::string& to) {
        const size_t at = json.find(from);
        ASSERT_NE(at, std::string::npos) << from;
        json.replace(at, from.size(), to);
    };
    replace("\"g\":[3,", "\"g\":[12,");
    replace("\"c\":[0,0,0,0,0,", "\"c\":[0,0,0,0,9,");
    replace(",0,0,255,0]]", ",0,99,77,0]]");
    State back;
    ASSERT_TRUE(StateCodec::decode(json.data(), json.size(), back)) << json;
    EXPECT_EQ(back.goal.target, 7);
    EXPECT_EQ(back.shields, 2);
    EXPECT_EQ(back.sessions[0].kind, Kind::Other);
    EXPECT_EQ(back.sessions[0].app, kNoApp);
}

TEST(StateCodec, SavesCrashSafelyAndFallsBackToTheBackup)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    char           scratch[StateCodec::kMaxBytes];
    State          first;
    first.streak = 1;
    State second;
    second.streak = 2;

    ASSERT_TRUE(StateCodec::save(fs, "state.json", first, scratch, sizeof(scratch)));
    EXPECT_FALSE(fs.hasFile("state.json.bak"));
    ASSERT_TRUE(StateCodec::save(fs, "state.json", second, scratch, sizeof(scratch)));
    EXPECT_TRUE(fs.hasFile("state.json.bak"));
    EXPECT_FALSE(fs.hasFile("state.json.tmp"));

    State loaded;
    EXPECT_EQ(StateCodec::load(fs, "state.json", loaded, scratch, sizeof(scratch)), StateCodec::Source::Primary);
    EXPECT_EQ(loaded.streak, 2);

    // A crash that tore the primary: the backup is used.
    fs.addFile("state.json", "{\"v\":1,\"g\":[3");
    EXPECT_EQ(StateCodec::load(fs, "state.json", loaded, scratch, sizeof(scratch)), StateCodec::Source::Backup);
    EXPECT_EQ(loaded.streak, 1);

    // Nothing at all: a fresh start.
    TreeFileSystem empty("/Apps/HybridXStreak");
    State          untouched;
    untouched.streak = 5;
    EXPECT_EQ(StateCodec::load(empty, "state.json", untouched, scratch, sizeof(scratch)), StateCodec::Source::None);
    EXPECT_EQ(untouched.streak, 5);
}

TEST(StateCodec, APublicCopyInSharedData)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    char           scratch[StateCodec::kMaxBytes];
    ASSERT_TRUE(fs.mkdir("../SharedData/HybridX"));
    const StreakModel m = busyModel();
    ASSERT_TRUE(StateCodec::save(fs, "../SharedData/HybridX/streak.json", m.state(), scratch, sizeof(scratch)));
    EXPECT_TRUE(fs.hasFile("/Apps/SharedData/HybridX/streak.json"));
    // The glance, from its own folder, reads the same file.
    TreeFileSystem glance = fs;
    State          s;
    // (Same tree; the glance's sandbox differs only in its own name.)
    EXPECT_EQ(StateCodec::load(glance, "../SharedData/HybridX/streak.json", s, scratch, sizeof(scratch)),
              StateCodec::Source::Primary);
    EXPECT_EQ(s.streak, m.state().streak);
}

TEST(StateCodec, SafeFileWriteFailureKeepsTheOldCopy)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    ASSERT_TRUE(SafeFile::write(fs, "x.json", "old", 3));
    // No such folder: staging fails, nothing changes.
    EXPECT_FALSE(SafeFile::write(fs, "missing/x.json", "new", 3));
    char buf[16];
    EXPECT_EQ(SafeFile::read(fs, "x.json", false, buf, sizeof(buf)), 3u);
    EXPECT_STREQ(buf, "old");
}
