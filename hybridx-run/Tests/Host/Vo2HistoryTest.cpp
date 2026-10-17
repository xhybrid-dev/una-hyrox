#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "Vo2History.hpp"

using namespace RunVo2;

TEST(Vo2History, RingDropsTheOldest)
{
    Vo2History h;
    for (uint32_t i = 1; i <= Config::kHistoryRuns + 3; ++i) {
        h.add(RunRecord{i, static_cast<uint16_t>(400 + i), 10});
    }
    EXPECT_EQ(h.size(), Config::kHistoryRuns);
    EXPECT_EQ(h.at(0).utc, 4u);
    EXPECT_EQ(h.at(Config::kHistoryRuns - 1).utc, Config::kHistoryRuns + 3u);
    EXPECT_EQ(h.at(200).utc, 0u);
}

TEST(Vo2History, RollingIsWeightedOverTheLatest)
{
    Vo2History h;
    EXPECT_EQ(h.rollingX10(), 0);
    h.add(RunRecord{1, 300, 30});   // falls out of the latest five
    h.add(RunRecord{2, 500, 10});
    h.add(RunRecord{3, 500, 10});
    h.add(RunRecord{4, 500, 10});
    h.add(RunRecord{5, 500, 10});
    h.add(RunRecord{6, 560, 100});  // weight capped at 30
    // (4 x 500 x 10 + 560 x 30) / 70 = 525.7
    EXPECT_EQ(h.rollingX10(), 526);
}

TEST(Vo2History, AutoMaxNeedsTwoRuns)
{
    Vo2History h;
    h.addRunMax(205);                // one artefact alone
    EXPECT_EQ(h.autoMaxHr(), 0);
    h.addRunMax(184);
    EXPECT_EQ(h.autoMaxHr(), 184);   // the second-highest
    h.addRunMax(188);
    EXPECT_EQ(h.autoMaxHr(), 188);
    h.addRunMax(188);
    EXPECT_EQ(h.autoMaxHr(), 188);   // 205 once, 188 twice
}

TEST(Vo2History, AutoMaxLeavesWithItsRuns)
{
    Vo2History h;
    h.addRunMax(200);
    h.addRunMax(199);
    EXPECT_EQ(h.autoMaxHr(), 199);
    for (int i = 0; i < Config::kHistoryRuns; ++i) {
        h.addRunMax(180);
    }
    EXPECT_EQ(h.runMaxCount(), Config::kHistoryRuns);
    EXPECT_EQ(h.autoMaxHr(), 180);   // the old highs rolled out
}

TEST(Vo2History, ImplausibleValuesAreNotKept)
{
    Vo2History h;
    h.addRunMax(0);
    h.addRunMax(250);
    EXPECT_EQ(h.runMaxCount(), 0);
    h.setWatchRestHr(120);   // the simulator's running floor
    EXPECT_EQ(h.watchRestHr(), 0);
    h.setWatchRestHr(52);
    h.setWatchRestHr(0);     // a new day before its figure: keep the last
    EXPECT_EQ(h.watchRestHr(), 52);
}

TEST(Vo2History, JsonRoundTrip)
{
    Vo2History h;
    h.setWatchRestHr(52);
    h.addRunMax(186);
    h.addRunMax(184);
    h.add(RunRecord{1791000000u, 523, 24});
    h.add(RunRecord{1791100000u, 531, 18});
    char buf[Vo2History::kMaxJsonBytes];
    const size_t n = h.toJson(buf, sizeof(buf));
    ASSERT_GT(n, 0u);
    EXPECT_STREQ(buf, "{\"v\":2,\"restHr\":52,\"maxes\":[186,184],"
                      "\"runs\":[[1791000000,523,24,523],[1791100000,531,18,526]]}");

    Vo2History back;
    ASSERT_TRUE(back.fromJson(buf, n));
    EXPECT_EQ(back.size(), 2);
    EXPECT_EQ(back.watchRestHr(), 52);
    EXPECT_EQ(back.autoMaxHr(), 184);
    EXPECT_EQ(back.at(1).vo2x10, 531);
    EXPECT_EQ(back.at(1).shownX10, 526);
    EXPECT_EQ(back.rollingX10(), h.rollingX10());
}

TEST(Vo2History, ReadsVersionOne)
{
    // R1-R3's file: its single auto max becomes one run's maximum, which alone
    // does not confirm.
    const char* t = "{\"v\":1,\"autoMaxHr\":188,\"runs\":[[1790500000,512,22],[1790900000,524,30]]}";
    Vo2History h;
    ASSERT_TRUE(h.fromJson(t, std::strlen(t)));
    EXPECT_EQ(h.size(), 2);
    EXPECT_EQ(h.runMaxCount(), 1);
    EXPECT_EQ(h.autoMaxHr(), 0);
    EXPECT_EQ(h.watchRestHr(), 0);
}

TEST(Vo2History, FullHistoryFitsTheBuffer)
{
    Vo2History h;
    h.setWatchRestHr(100);
    for (int i = 0; i < Config::kHistoryRuns; ++i) {
        h.add(RunRecord{0xFFFFFFFFu, 950, Config::kMaxWindows});
        h.addRunMax(230);
    }
    char buf[Vo2History::kMaxJsonBytes];
    const size_t n = h.toJson(buf, sizeof(buf));
    ASSERT_GT(n, 0u);
    Vo2History back;
    EXPECT_TRUE(back.fromJson(buf, n));
    EXPECT_EQ(back.size(), Config::kHistoryRuns);
    char tiny[20];
    EXPECT_EQ(h.toJson(tiny, sizeof(tiny)), 0u);
    EXPECT_STREQ(tiny, "");
}

TEST(Vo2History, EmptyAndWhitespace)
{
    Vo2History h;
    const char* t = " { \"v\" : 2 , \"restHr\" : 0 , \"maxes\" : [ ] , \"runs\" : [ ] }\n";
    EXPECT_TRUE(h.fromJson(t, std::strlen(t)));
    EXPECT_EQ(h.size(), 0);
}

TEST(Vo2History, UnknownKeysAreIgnored)
{
    // A newer version's extra key must not cost the history.
    const char* t = "{\"v\":2,\"restHr\":50,\"maxes\":[],\"runs\":[[1,500,6]],\"note\":\"x\",\"extra\":[1,2]}";
    Vo2History h;
    ASSERT_TRUE(h.fromJson(t, std::strlen(t)));
    EXPECT_EQ(h.size(), 1);
}

TEST(Vo2History, MalformedReadsAsEmpty)
{
    const char* bad[] = {
        "",
        "{",
        "{}",                                                        // no version
        "{\"v\":3,\"restHr\":0,\"maxes\":[],\"runs\":[]}",           // unknown version
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,500]]}",    // short record
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,500,6]]",   // truncated
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,9999,6]]}", // implausible VO2
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,100,6]]}",  // implausible VO2 (low)
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[99999999999,500,6]]}", // utc overflow
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[0,500,6]]}",  // no time
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,500,2]]}",  // too few windows
        "{\"v\":2,\"restHr\":0,\"maxes\":[300],\"runs\":[]}",        // implausible max
        "{\"v\":2,\"restHr\":-5,\"maxes\":[],\"runs\":[]}",          // negative
        "{\"v\":2,\"maxes\":[],\"runs\":[]}",                        // no restHr
        "{\"v\":2,\"restHr\":0,\"runs\":[]}",                        // no maxes
        "{\"v\":1,\"runs\":[]}",                                     // v1 without autoMaxHr
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[-1,500,6]]}", // negative time
    };
    for (const char* t : bad) {
        Vo2History h;
        h.add(RunRecord{1, 500, 10});
        EXPECT_FALSE(h.fromJson(t, std::strlen(t))) << t;
        EXPECT_EQ(h.size(), 0) << t;
    }
    Vo2History h;
    EXPECT_FALSE(h.fromJson(nullptr, 10));
}

TEST(Vo2History, ReadStopsAtTheGivenLength)
{
    const std::string t = "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,500,6]]}";
    Vo2History h;
    EXPECT_FALSE(h.fromJson(t.c_str(), t.size() - 1));
    EXPECT_TRUE(h.fromJson(t.c_str(), t.size()));
}

TEST(Vo2History, SharedJson)
{
    Vo2History h;
    char buf[Vo2History::kMaxSharedJsonBytes];
    ASSERT_GT(h.toSharedJson(buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "{\"v\":2,\"vo2maxX10\":0,\"runs\":0,\"utc\":0,\"beforeX10\":0,\"trend\":[]}");
    h.add(RunRecord{1791000000u, 523, 24});
    ASSERT_GT(h.toSharedJson(buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "{\"v\":2,\"vo2maxX10\":523,\"runs\":1,\"utc\":1791000000,"
                      "\"beforeX10\":0,\"trend\":[523]}");
    h.add(RunRecord{1791100000u, 533, 24});
    ASSERT_GT(h.toSharedJson(buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "{\"v\":2,\"vo2maxX10\":528,\"runs\":2,\"utc\":1791100000,"
                      "\"beforeX10\":523,\"trend\":[523,528]}");
    EXPECT_EQ(h.toSharedJson(buf, 8), 0u);
}

TEST(Vo2History, RunsWithoutTheShownValueStillRead)
{
    // Before R6 each run had three fields: the shown value is worked out from
    // the runs read so far.
    const std::string t = "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,500,10],[2,520,10,505]]}";
    Vo2History h;
    ASSERT_TRUE(h.fromJson(t.c_str(), t.size()));
    EXPECT_EQ(h.at(0).shownX10, 500);
    EXPECT_EQ(h.at(1).shownX10, 505);   // as written, not recomputed (510)
}

TEST(Vo2History, ABadShownValueFailsTheFile)
{
    const char* bad[] = {
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,500,10,0]]}",
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,500,10,9999]]}",
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,500,10,500,1]]}",
        "{\"v\":2,\"restHr\":0,\"maxes\":[],\"runs\":[[1,500]]}",
    };
    for (const char* s : bad) {
        Vo2History h;
        EXPECT_FALSE(h.fromJson(s, std::strlen(s))) << s;
        EXPECT_EQ(h.size(), 0) << s;
    }
}

TEST(Vo2History, JsonFitsWhenFull)
{
    Vo2History h;
    for (uint8_t i = 0; i < Config::kHistoryRuns; ++i) {
        h.addRunMax(220);
    }
    h.setWatchRestHr(100);
    for (uint32_t i = 0; i < Config::kHistoryRuns; ++i) {
        h.add(RunRecord{4000000000u + i, 950, Config::kMaxWindows});
    }
    char buf[Vo2History::kMaxJsonBytes];
    EXPECT_GT(h.toJson(buf, sizeof(buf)), 0u);
}

TEST(Vo2History, SharedJsonFitsWhenFull)
{
    Vo2History h;
    for (uint32_t i = 0; i < Config::kHistoryRuns + 3; ++i) {
        h.add(RunRecord{4000000000u + i, 899, 65535});
    }
    char buf[Vo2History::kMaxSharedJsonBytes];
    EXPECT_GT(h.toSharedJson(buf, sizeof(buf)), 0u);
}

TEST(Vo2History, TrendIsTheShownFigureAfterEachRun)
{
    Vo2History h;
    uint16_t t[Config::kHistoryRuns] = {};
    EXPECT_EQ(h.trendX10(t, Config::kHistoryRuns), 0u);
    EXPECT_EQ(h.trendX10(nullptr, Config::kHistoryRuns), 0u);
    for (uint16_t i = 0; i < 12; ++i) {
        h.add(RunRecord{1000u + i, static_cast<uint16_t>(500 + 10 * i), 10});
    }
    ASSERT_EQ(h.trendX10(t, Config::kHistoryRuns), Config::kHistoryRuns);
    // The last entry is today's figure, the one before it the figure before.
    EXPECT_EQ(t[Config::kHistoryRuns - 1], h.rollingX10());
    EXPECT_EQ(t[Config::kHistoryRuns - 2], h.rollingX10Before());
    // Kept runs 2..11 (vo2 520..610), each with the value shown when it was
    // added, so dropped runs still count: after run 2, runs 0..2 = 510; after
    // run 6, runs 2..6 = 540.
    EXPECT_EQ(t[0], 510);
    EXPECT_EQ(t[4], 540);
    // Fewer asked for: the newest ones.
    uint16_t two[2] = {};
    ASSERT_EQ(h.trendX10(two, 2), 2u);
    EXPECT_EQ(two[1], h.rollingX10());
    EXPECT_EQ(two[0], h.rollingX10Before());
}

TEST(Vo2History, RollingBeforeTheLatestRun)
{
    Vo2History h;
    EXPECT_EQ(h.rollingX10Before(), 0);
    h.add(RunRecord{1, 500, 10});
    EXPECT_EQ(h.rollingX10Before(), 0);   // first run: no earlier figure
    h.add(RunRecord{2, 520, 10});
    EXPECT_EQ(h.rollingX10Before(), 500);
    EXPECT_EQ(h.rollingX10(), 510);
    for (uint32_t i = 3; i <= 7; ++i) {
        h.add(RunRecord{i, 540, 10});
    }
    // Before the 7th run: runs 2-6 (520, 540 x4) -> 536; now: runs 3-7 -> 540.
    EXPECT_EQ(h.rollingX10Before(), 536);
    EXPECT_EQ(h.rollingX10(), 540);
}
