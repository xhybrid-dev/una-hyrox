#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <string>

#include "ProbeCsv.hpp"

using Sauna::CsvRow;
using Sauna::formatCsvHeader;
using Sauna::formatCsvRow;
using Sauna::formatNoteLine;
using Sauna::formatRunLine;
using Sauna::kX100Invalid;
using Sauna::RunCounter;

namespace {

std::string row(const CsvRow &r)
{
    char buf[200];
    EXPECT_GT(formatCsvRow(buf, sizeof(buf), r), 0u);
    return buf;
}

} // namespace

TEST(ProbeCsv, HeaderNamesFifteenColumns)
{
    char buf[160];
    const size_t n = formatCsvHeader(buf, sizeof(buf));
    ASSERT_GT(n, 0u);
    const std::string h = buf;
    EXPECT_EQ(h.back(), '\n');
    EXPECT_EQ(std::count(h.begin(), h.end(), ','), 14);
    EXPECT_EQ(h.rfind("ms,t_s,mark,", 0), 0u);
}

TEST(ProbeCsv, RowBeforeAnySampleHasBlankReadings)
{
    CsvRow r;
    r.tMs = 1000;
    r.tS = 1;
    EXPECT_EQ(row(r), "1000,1,0,,0,0,0,0,,,0,0,,0,0\n");
}

TEST(ProbeCsv, RowWithSamples)
{
    CsvRow r;
    r.tMs = 61250;
    r.tS = 61;
    r.mark = 2;
    r.tempX100 = 8512;
    r.tempN = 61;
    r.tempBad = 1;
    r.tempFields = 1;
    r.tempGapMs = 1000;
    r.hrN = 59;
    r.hr = 132;
    r.trustX100 = 100;
    r.phase = 2;
    r.hintDeltaX100 = -350;
    r.hintLevel = 1;
    r.hintTrend = 1;
    EXPECT_EQ(row(r), "61250,61,2,8512,61,1,1,1000,132,100,59,2,-350,1,1\n");
}

TEST(ProbeCsv, NegativeTemperatureIsWrittenWithItsSign)
{
    CsvRow r;
    r.tempX100 = -1250;
    r.tempN = 3;
    EXPECT_EQ(row(r), "0,0,0,-1250,3,0,0,0,,,0,0,,0,0\n");
}

TEST(ProbeCsv, HintDeltaIsBlankUntilTheHintHasALevel)
{
    CsvRow r;
    r.hintDeltaX100 = 120;
    r.hintLevel = 0;
    EXPECT_EQ(row(r), "0,0,0,,0,0,0,0,,,0,0,,0,0\n");
    r.hintLevel = 2;
    r.hintTrend = 3;
    EXPECT_EQ(row(r), "0,0,0,,0,0,0,0,,,0,0,120,2,3\n");
}

TEST(ProbeCsv, ZeroHeartRateIsBlankUntilASampleArrives)
{
    CsvRow r;
    r.hr = 0;
    r.hrN = 0;
    EXPECT_EQ(row(r), "0,0,0,,0,0,0,0,,,0,0,,0,0\n");
    r.hrN = 1;                         // a sample that really read 0 (watch off the wrist)
    r.trustX100 = 0;
    EXPECT_EQ(row(r), "0,0,0,,0,0,0,0,0,0,1,0,,0,0\n");
}

TEST(ProbeCsv, RowEndsAtItsNewlineWithNothingAfter)
{
    CsvRow r;
    r.tMs = 4294967295u;
    r.tS = 4294967u;
    char buf[160];
    std::memset(buf, 'X', sizeof(buf));
    const size_t n = formatCsvRow(buf, sizeof(buf), r);
    ASSERT_GT(n, 0u);
    EXPECT_EQ(buf[n - 1], '\n');
    EXPECT_EQ(buf[n], '\0');
}

TEST(ProbeCsv, ATooSmallBufferGivesNothing)
{
    CsvRow r;
    r.tMs = 123456;
    r.tempX100 = 8512;
    for (size_t cap = 1; cap < 20; ++cap) {   // the shortest row is longer than that
        char buf[32];
        std::memset(buf, 'G', sizeof(buf));
        EXPECT_EQ(formatCsvRow(buf, cap, r), 0u) << "cap " << cap;
        EXPECT_EQ(buf[0], '\0');
        for (size_t i = cap; i < sizeof(buf); ++i) {
            ASSERT_EQ(buf[i], 'G') << "cap " << cap << " wrote past the end at " << i;
        }
    }
    EXPECT_EQ(formatCsvRow(nullptr, 10, r), 0u);
    char c;
    EXPECT_EQ(formatCsvRow(&c, 0, r), 0u);
}

TEST(ProbeCsv, RunLine)
{
    char buf[80];
    ASSERT_GT(formatRunLine(buf, sizeof(buf), 3, "2026-10-20 14:32:05"), 0u);
    EXPECT_STREQ(buf, "# run 3 2026-10-20 14:32:05\n");
    ASSERT_GT(formatRunLine(buf, sizeof(buf), 1, nullptr), 0u);
    EXPECT_STREQ(buf, "# run 1 \n");
    EXPECT_EQ(formatRunLine(buf, 8, 3, "2026-10-20 14:32:05"), 0u);
}

TEST(ProbeCsv, NoteLineFlattensNewlines)
{
    char buf[80];
    ASSERT_GT(formatNoteLine(buf, sizeof(buf), "temperature: connect ok\nhr: connect ok"), 0u);
    EXPECT_STREQ(buf, "# temperature: connect ok hr: connect ok\n");
    EXPECT_EQ(formatNoteLine(buf, 6, "far too long for this"), 0u);
}

TEST(RunCounter, CountsRunLines)
{
    const char *log =
        "ms,t_s,mark\n"
        "# run 1 2026-10-20 14:00:00\n"
        "# sensors: ok\n"
        "1000,1,0\n"
        "# run 2 2026-10-20 15:00:00\n"
        "1000,1,0\n";
    RunCounter c;
    c.feed(log, std::strlen(log));
    EXPECT_EQ(c.runs(), 2u);
}

TEST(RunCounter, ChunkSizeDoesNotChangeTheCount)
{
    const std::string log =
        "ms,t_s\n# run 1 a\n1,1\n# run 2 b\n2,2\n# note run 3\n#run 4\n# run 5 c\n";
    for (size_t chunk = 1; chunk <= log.size(); ++chunk) {
        RunCounter c;
        for (size_t at = 0; at < log.size(); at += chunk) {
            c.feed(log.data() + at, std::min(chunk, log.size() - at));
        }
        ASSERT_EQ(c.runs(), 3u) << "chunk " << chunk;
    }
}

TEST(RunCounter, OnlyLineStartsCount)
{
    const char *log = "1,2,# run 9 x\n  # run 8 y\n";
    RunCounter c;
    c.feed(log, std::strlen(log));
    EXPECT_EQ(c.runs(), 0u);
}

TEST(RunCounter, EmptyInput)
{
    RunCounter c;
    c.feed("", 0);
    c.feed(nullptr, 0);
    EXPECT_EQ(c.runs(), 0u);
}
