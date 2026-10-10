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

TEST(Vo2History, AutoMaxOnlyRises)
{
    Vo2History h;
    h.raiseAutoMaxHr(180);
    h.raiseAutoMaxHr(175);
    EXPECT_EQ(h.autoMaxHr(), 180);
    h.raiseAutoMaxHr(250);   // not believable
    EXPECT_EQ(h.autoMaxHr(), 180);
}

TEST(Vo2History, JsonRoundTrip)
{
    Vo2History h;
    h.raiseAutoMaxHr(188);
    h.add(RunRecord{1791000000u, 523, 24});
    h.add(RunRecord{1791100000u, 531, 18});
    char buf[Vo2History::kMaxJsonBytes];
    const size_t n = h.toJson(buf, sizeof(buf));
    ASSERT_GT(n, 0u);
    EXPECT_STREQ(buf, "{\"v\":1,\"autoMaxHr\":188,\"runs\":[[1791000000,523,24],[1791100000,531,18]]}");

    Vo2History back;
    ASSERT_TRUE(back.fromJson(buf, n));
    EXPECT_EQ(back.size(), 2);
    EXPECT_EQ(back.autoMaxHr(), 188);
    EXPECT_EQ(back.at(1).vo2x10, 531);
    EXPECT_EQ(back.rollingX10(), h.rollingX10());
}

TEST(Vo2History, FullHistoryFitsTheBuffer)
{
    Vo2History h;
    h.raiseAutoMaxHr(230);
    for (int i = 0; i < Config::kHistoryRuns; ++i) {
        h.add(RunRecord{0xFFFFFFFFu, 950, Config::kMaxWindows});
    }
    char buf[Vo2History::kMaxJsonBytes];
    EXPECT_GT(h.toJson(buf, sizeof(buf)), 0u);
    char tiny[20];
    EXPECT_EQ(h.toJson(tiny, sizeof(tiny)), 0u);
    EXPECT_STREQ(tiny, "");
}

TEST(Vo2History, EmptyAndWhitespace)
{
    Vo2History h;
    const char* t = " { \"v\" : 1 , \"runs\" : [ ] }\n";
    EXPECT_TRUE(h.fromJson(t, std::strlen(t)));
    EXPECT_EQ(h.size(), 0);
}

TEST(Vo2History, MalformedReadsAsEmpty)
{
    const char* bad[] = {
        "",
        "{",
        "{}",                                            // no version
        "{\"v\":2,\"runs\":[]}",                         // unknown version
        "{\"v\":1,\"runs\":[[1,2]]}",                    // short record
        "{\"v\":1,\"runs\":[[1,2,3]",                    // truncated
        "{\"v\":1,\"runs\":[[1,9999,3]]}",               // implausible VO2
        "{\"v\":1,\"runs\":[[99999999999,500,3]]}",      // utc overflow
        "{\"v\":1,\"autoMaxHr\":300,\"runs\":[]}",       // implausible HR
        "{\"v\":1,\"other\":1}",                         // unknown key
        "{\"v\":1,\"runs\":[[-1,500,3]]}",               // negative
        "{\"v\\\"\":1}",                                 // escaped key
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
    const std::string t = "{\"v\":1,\"runs\":[[1,500,3]]}";
    Vo2History h;
    EXPECT_FALSE(h.fromJson(t.c_str(), t.size() - 1));
    EXPECT_TRUE(h.fromJson(t.c_str(), t.size()));
}

TEST(Vo2History, SharedJson)
{
    Vo2History h;
    char buf[96];
    ASSERT_GT(h.toSharedJson(buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "{\"v\":1,\"vo2maxX10\":0,\"runs\":0,\"utc\":0}");
    h.add(RunRecord{1791000000u, 523, 24});
    ASSERT_GT(h.toSharedJson(buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "{\"v\":1,\"vo2maxX10\":523,\"runs\":1,\"utc\":1791000000}");
    EXPECT_EQ(h.toSharedJson(buf, 8), 0u);
}
