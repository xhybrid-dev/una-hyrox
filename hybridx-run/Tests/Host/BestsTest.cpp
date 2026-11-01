// HybridX Run: personal bests across runs (Bests).

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "Bests.hpp"

using RunVo2::Bests;

namespace
{
constexpr uint32_t kRun1 = 1793000000u;
constexpr uint32_t kRun2 = 1793100000u;
}  // namespace

TEST(Bests, AFirstRunSetsEveryBestItHas)
{
    Bests b;
    const uint32_t splits[3] = {240000u, 390000u, 0u};   // 4:00 km, 6:30 mile, no 5 km
    EXPECT_EQ(b.update(splits, kRun1), 0b011);
    EXPECT_EQ(b.at(0).ms, 240000u);
    EXPECT_EQ(b.at(0).utc, kRun1);
    EXPECT_EQ(b.at(2).ms, 0u);
    EXPECT_EQ(b.at(2).utc, 0u);
}

TEST(Bests, OnlyAFasterTimeReplacesABest)
{
    Bests b;
    const uint32_t first[3] = {240000u, 390000u, 1260000u};
    b.update(first, kRun1);
    const uint32_t second[3] = {235000u, 395000u, 1260000u};   // faster km, slower mile, equal 5 km
    EXPECT_EQ(b.update(second, kRun2), 0b001);
    EXPECT_EQ(b.at(0).ms, 235000u);
    EXPECT_EQ(b.at(0).utc, kRun2);
    EXPECT_EQ(b.at(1).ms, 390000u);
    EXPECT_EQ(b.at(1).utc, kRun1);
    EXPECT_EQ(b.at(2).utc, kRun1);   // an equal time is not a new best
}

TEST(Bests, AnImpossibleTimeIsAFaultNotABest)
{
    Bests b;
    // 1 km in 60 s (16.7 m/s: a GPS jump), and 5 km in 4 hours (a walk with stops).
    const uint32_t splits[3] = {60000u, 390000u, 14400000u};
    EXPECT_EQ(b.update(splits, kRun1), 0b010);
    EXPECT_EQ(b.at(0).ms, 0u);
    EXPECT_EQ(b.at(2).ms, 0u);
    EXPECT_FALSE(Bests::plausible(0, 60000u));
    EXPECT_TRUE(Bests::plausible(0, 150000u));   // 2:30 km: fast, but a person can
    EXPECT_FALSE(Bests::plausible(3, 240000u));  // no such distance
    EXPECT_FALSE(Bests::plausible(0, 0u));
}

TEST(Bests, NothingOfferedChangesNothing)
{
    Bests b;
    const uint32_t none[3] = {0u, 0u, 0u};
    EXPECT_EQ(b.update(none, kRun1), 0);
    EXPECT_EQ(b.update(nullptr, kRun1), 0);
    EXPECT_EQ(b.at(0).ms, 0u);
    EXPECT_EQ(b.at(9).ms, 0u);   // out of range reads as none
}

TEST(Bests, TheFileRoundTrips)
{
    Bests b;
    const uint32_t splits[3] = {240000u, 390000u, 1260000u};
    b.update(splits, kRun1);
    char buf[Bests::kMaxJsonBytes];
    const size_t len = b.toJson(buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    EXPECT_STREQ(buf, R"({"v":1,"b":[[240000,1793000000],[390000,1793000000],[1260000,1793000000]]})");
    Bests c;
    ASSERT_TRUE(c.fromJson(buf, len));
    for (uint8_t i = 0; i < Bests::kCount; ++i) {
        EXPECT_EQ(c.at(i).ms, b.at(i).ms);
        EXPECT_EQ(c.at(i).utc, b.at(i).utc);
    }

    Bests empty;
    const size_t elen = empty.toJson(buf, sizeof(buf));
    EXPECT_STREQ(buf, R"({"v":1,"b":[[0,0],[0,0],[0,0]]})");
    ASSERT_TRUE(c.fromJson(buf, elen));
    EXPECT_EQ(c.at(1).ms, 0u);
}

TEST(Bests, ABadFileReadsAsNoneAndLeavesNothing)
{
    const char* bad[] = {
        "",
        "not json",
        R"({"v":2,"b":[[0,0],[0,0],[0,0]]})",                       // a newer version
        R"({"v":1,"b":[[0,0],[0,0]]})",                             // too few
        R"({"v":1,"b":[[240000],[0,0],[0,0]]})",                    // a short entry
        R"({"v":1,"b":[[-5,0],[0,0],[0,0]]})",                      // negative
        R"({"v":1,"b":[[60000,1793000000],[0,0],[0,0]]})",          // an impossible best
        R"({"v":1})",
    };
    for (const char* text : bad) {
        Bests b;
        const uint32_t splits[3] = {240000u, 0u, 0u};
        b.update(splits, kRun1);
        EXPECT_FALSE(b.fromJson(text, std::strlen(text))) << text;
        EXPECT_EQ(b.at(0).ms, 0u) << text;   // cleared, not half-read
    }
}

TEST(Bests, TheBufferIsBigEnoughForTheLongestFile)
{
    Bests b;
    const uint32_t slowest[3] = {2000000u, 3218000u, 9999000u};   // all within kMinSpeedMs
    b.update(slowest, 4294967295u);
    char buf[Bests::kMaxJsonBytes];
    EXPECT_GT(b.toJson(buf, sizeof(buf)), 0u);
    char tiny[16];
    EXPECT_EQ(b.toJson(tiny, sizeof(tiny)), 0u);
    EXPECT_STREQ(tiny, "");
}
