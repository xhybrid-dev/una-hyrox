#include <gtest/gtest.h>

#include <cstring>

#include "RunVo2Max.hpp"

using namespace XTraining;

namespace
{
bool readText(const char* s, RunVo2Max::Figure& f) { return RunVo2Max::read(s, std::strlen(s), f); }
}  // namespace

TEST(RunVo2Max, XRunsFileReads)
{
    // As X Run's Vo2History::toSharedJson writes it (hybridx-run's sim_trail_ingest.sh, start 4).
    RunVo2Max::Figure f;
    ASSERT_TRUE(readText(R"({"v":2,"vo2maxX10":514,"runs":7,"utc":1791600000,"beforeX10":519,)"
                         R"("trend":[512,519,517,517,514,519,514]})", f));
    EXPECT_EQ(f.x10, 514);
    EXPECT_EQ(f.runs, 7);
    EXPECT_EQ(f.utc, 1791600000u);
    // Version 1 (before the trend) has the same keys.
    ASSERT_TRUE(readText(R"({"v":1,"vo2maxX10":452,"runs":3,"utc":1791000000})", f));
    EXPECT_EQ(f.x10, 452);
}

TEST(RunVo2Max, AnythingWrongIsNoFigure)
{
    const char* bad[] = {
        "",
        "not json",
        R"({"v":3,"vo2maxX10":514,"runs":7,"utc":1791600000})",      // a newer version
        R"({"v":2,"vo2maxX10":514,"runs":0,"utc":1791600000})",      // no runs yet
        R"({"v":2,"vo2maxX10":9999,"runs":7,"utc":1791600000})",     // out of range
        R"({"v":2,"vo2maxX10":-514,"runs":7,"utc":1791600000})",
        R"({"v":2,"runs":7,"utc":1791600000})",                      // no figure
        R"({"v":2,"vo2maxX10":514,"runs":7})",                       // no time
    };
    for (const char* s : bad) {
        RunVo2Max::Figure f;
        f.x10 = 1;
        EXPECT_FALSE(RunVo2Max::read(s, std::strlen(s), f)) << s;
        EXPECT_EQ(f.x10, 0) << s;
    }
    RunVo2Max::Figure f;
    EXPECT_FALSE(RunVo2Max::read(nullptr, 10, f));
}
