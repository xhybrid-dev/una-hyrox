#include <gtest/gtest.h>

#include "ProbeStats.hpp"

using Sauna::kX100Invalid;
using Sauna::Sense;
using Sauna::StreamStats;

TEST(StreamStats, StartsEmpty)
{
    StreamStats s;
    EXPECT_FALSE(s.has());
    EXPECT_EQ(s.count(), 0u);
    EXPECT_EQ(s.last(), kX100Invalid);
    EXPECT_EQ(s.min(), kX100Invalid);
    EXPECT_EQ(s.max(), kX100Invalid);
    EXPECT_EQ(s.lastGapMs(), 0u);
    EXPECT_EQ(s.minGapMs(), 0u);
    EXPECT_EQ(s.maxGapMs(), 0u);
}

TEST(StreamStats, KeepsLastAndRange)
{
    StreamStats s;
    s.add(1000, 2200);
    s.add(2000, 2500);
    s.add(3000, 2100);
    EXPECT_EQ(s.count(), 3u);
    EXPECT_EQ(s.last(), 2100);
    EXPECT_EQ(s.min(), 2100);
    EXPECT_EQ(s.max(), 2500);
}

TEST(StreamStats, RangeHandlesNegativeReadings)
{
    StreamStats s;
    s.add(0, -500);
    s.add(1000, -1200);
    s.add(2000, 300);
    EXPECT_EQ(s.min(), -1200);
    EXPECT_EQ(s.max(), 300);
}

TEST(StreamStats, GapsAreTheTimeBetweenSamples)
{
    StreamStats s;
    s.add(1000, 1);
    EXPECT_EQ(s.lastGapMs(), 0u);       // no gap from one sample
    EXPECT_EQ(s.minGapMs(), 0u);
    s.add(2000, 1);                     // 1000
    s.add(2400, 1);                     // 400
    s.add(5400, 1);                     // 3000
    EXPECT_EQ(s.lastGapMs(), 3000u);
    EXPECT_EQ(s.minGapMs(), 400u);
    EXPECT_EQ(s.maxGapMs(), 3000u);
}

TEST(StreamStats, GapsAreRightAcrossTheClockWrap)
{
    StreamStats s;
    s.add(0xFFFFFC18u, 1);              // 1000 ms before the wrap
    s.add(500, 1);                      // 1500 ms later
    EXPECT_EQ(s.lastGapMs(), 1500u);
    EXPECT_EQ(s.minGapMs(), 1500u);
    EXPECT_EQ(s.maxGapMs(), 1500u);
}

TEST(StreamStats, SamplesWithTheSameStampHaveAZeroGap)
{
    StreamStats s;
    s.add(1000, 1);
    s.add(1000, 2);
    s.add(2000, 3);
    EXPECT_EQ(s.minGapMs(), 0u);
    EXPECT_EQ(s.maxGapMs(), 1000u);
}

TEST(StreamStats, ClearRangeKeepsTheLatestReading)
{
    StreamStats s;
    s.add(0, 1000);
    s.add(1000, 9000);
    s.add(2000, 4000);
    s.clearRange();
    EXPECT_EQ(s.last(), 4000);
    EXPECT_EQ(s.min(), 4000);
    EXPECT_EQ(s.max(), 4000);
    EXPECT_EQ(s.count(), 3u);           // the count is not a range
    s.add(3000, 5000);
    EXPECT_EQ(s.min(), 4000);
    EXPECT_EQ(s.max(), 5000);
}

TEST(StreamStats, ClearRangeBeforeAnySampleLeavesItEmpty)
{
    StreamStats s;
    s.clearRange();
    EXPECT_EQ(s.min(), kX100Invalid);
    EXPECT_EQ(s.max(), kX100Invalid);
    s.add(0, 700);
    EXPECT_EQ(s.min(), 700);
    EXPECT_EQ(s.max(), 700);
}

TEST(StreamStats, SenseNamesAreDistinct)
{
    EXPECT_STRNE(Sauna::senseName(Sense::NotStarted), Sauna::senseName(Sense::ConnectFailed));
    EXPECT_STRNE(Sauna::senseName(Sense::NoData), Sauna::senseName(Sense::Ok));
}
