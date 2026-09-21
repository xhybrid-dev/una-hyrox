/**
 * @file LapAccumulatorTest.cpp
 * @brief Heart-rate accumulation, and the exact merge on undo (brief 10.1).
 *
 * The FIT lap carries avg_heart_rate and max_heart_rate. If undo merged two
 * segments by averaging their averages the result would drift, so the model
 * keeps a sum, a count and a max per segment. These tests pin that.
 */

#include <gtest/gtest.h>

#include "RaceModel.hpp"

using Race::Format;
using Race::RaceModel;

namespace
{

RaceModel::Config config()
{
    RaceModel::Config c;
    c.format = Format::HalfA;
    c.roxzone = false;
    c.lockoutMs = 3000u;
    return c;
}

void feed(RaceModel &m, const std::initializer_list<uint8_t> &samples)
{
    for (const uint8_t bpm : samples) {
        m.addHeartRate(bpm);
    }
}

}  // namespace

// -- Accumulation ----------------------------------------------------------------

TEST(LapAccumulatorTest, SamplesLandOnTheOpenSegment)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    feed(m, { 150u, 160u, 170u });
    ASSERT_TRUE(m.split(10000u));

    const auto *seg = m.recorded(0u);
    ASSERT_NE(seg, nullptr);
    EXPECT_EQ(seg->hrSum, 480u);
    EXPECT_EQ(seg->hrCount, 3u);
    EXPECT_EQ(seg->hrMax, 170u);
    EXPECT_EQ(seg->hrAvg(), 160u);
}

TEST(LapAccumulatorTest, EachSegmentAccumulatesIndependently)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    feed(m, { 140u, 150u });
    ASSERT_TRUE(m.split(10000u));
    feed(m, { 180u, 182u, 184u });
    ASSERT_TRUE(m.split(20000u));

    EXPECT_EQ(m.recorded(0u)->hrCount, 2u);
    EXPECT_EQ(m.recorded(0u)->hrMax, 150u);
    EXPECT_EQ(m.recorded(1u)->hrCount, 3u);
    EXPECT_EQ(m.recorded(1u)->hrMax, 184u);
    EXPECT_EQ(m.recorded(1u)->hrAvg(), 182u);
}

TEST(LapAccumulatorTest, AverageRoundsToNearest)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    // 150 + 151 = 301, over 2 samples = 150.5, which must round up to 151.
    feed(m, { 150u, 151u });
    ASSERT_TRUE(m.split(10000u));
    EXPECT_EQ(m.recorded(0u)->hrAvg(), 151u);
}

TEST(LapAccumulatorTest, ASegmentWithNoSamplesReportsZero)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    ASSERT_TRUE(m.split(10000u));

    const auto *seg = m.recorded(0u);
    ASSERT_NE(seg, nullptr);
    EXPECT_EQ(seg->hrCount, 0u);
    EXPECT_EQ(seg->hrMax, 0u);
    EXPECT_EQ(seg->hrAvg(), 0u) << "no samples must not divide by zero";
}

TEST(LapAccumulatorTest, SamplesTakenWhilePausedAreDropped)
{
    // A pause is rest. Counting it would drag the segment average down and
    // misrepresent the effort.
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    feed(m, { 180u, 180u });
    ASSERT_TRUE(m.pause(5000u));
    feed(m, { 90u, 85u, 80u });
    ASSERT_TRUE(m.resume(20000u));
    ASSERT_TRUE(m.split(25000u));

    const auto *seg = m.recorded(0u);
    ASSERT_NE(seg, nullptr);
    EXPECT_EQ(seg->hrCount, 2u);
    EXPECT_EQ(seg->hrSum, 360u);
    EXPECT_EQ(seg->hrAvg(), 180u);
}

TEST(LapAccumulatorTest, SamplesBeforeTheStartAreDropped)
{
    RaceModel m;
    m.addHeartRate(150u);
    ASSERT_TRUE(m.start(config(), 0u));
    ASSERT_TRUE(m.split(10000u));

    EXPECT_EQ(m.recorded(0u)->hrCount, 0u);
}

// -- The merge on undo (brief 10.1) ------------------------------------------------

TEST(LapAccumulatorTest, UndoMergesSumCountAndMaxExactly)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    feed(m, { 150u, 160u });            // segment 0
    ASSERT_TRUE(m.split(10000u));
    feed(m, { 190u, 170u, 180u });      // segment 1, opened by mistake
    ASSERT_TRUE(m.undoSplit());

    // Everything now belongs to segment 0 again.
    ASSERT_TRUE(m.split(20000u));

    const auto *seg = m.recorded(0u);
    ASSERT_NE(seg, nullptr);
    EXPECT_EQ(seg->hrCount, 5u);
    EXPECT_EQ(seg->hrSum, 150u + 160u + 190u + 170u + 180u);
    EXPECT_EQ(seg->hrMax, 190u) << "the max must survive the merge";
    EXPECT_EQ(seg->hrAvg(), 170u);
}

TEST(LapAccumulatorTest, TheMergedAverageIsNotAnAverageOfAverages)
{
    // The point of keeping a sum and a count. Segment 0 has one sample at 200,
    // segment 1 has four at 100. Averaging averages gives 150; the truthful
    // answer is 120.
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    feed(m, { 200u });
    ASSERT_TRUE(m.split(10000u));
    feed(m, { 100u, 100u, 100u, 100u });
    ASSERT_TRUE(m.undoSplit());
    ASSERT_TRUE(m.split(20000u));

    EXPECT_EQ(m.recorded(0u)->hrAvg(), 120u);
}

TEST(LapAccumulatorTest, UndoKeepsTheEarlierMaxWhenItIsHigher)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    feed(m, { 195u });                  // the peak happened before the split
    ASSERT_TRUE(m.split(10000u));
    feed(m, { 120u, 130u });
    ASSERT_TRUE(m.undoSplit());
    ASSERT_TRUE(m.split(20000u));

    EXPECT_EQ(m.recorded(0u)->hrMax, 195u);
}

TEST(LapAccumulatorTest, UndoFinishMergesTheFinalSegmentToo)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    uint32_t t = 0u;
    for (uint8_t i = 0u; i < 7u; ++i) {
        t += 10000u;
        ASSERT_TRUE(m.split(t));
    }

    feed(m, { 175u, 185u });            // during the final segment
    t += 10000u;
    ASSERT_TRUE(m.split(t));            // finishes the race
    ASSERT_EQ(m.state(), RaceModel::State::Finished);

    ASSERT_TRUE(m.undoFinish());
    feed(m, { 195u });                  // a bit more effort after the undo
    ASSERT_TRUE(m.split(t + 5000u));

    const auto *last = m.recorded(7u);
    ASSERT_NE(last, nullptr);
    EXPECT_EQ(last->hrCount, 3u);
    EXPECT_EQ(last->hrSum, 175u + 185u + 195u);
    EXPECT_EQ(last->hrMax, 195u);
}

TEST(LapAccumulatorTest, RepeatedUndoMergesAllTheWayBack)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    feed(m, { 100u });
    ASSERT_TRUE(m.split(10000u));
    feed(m, { 110u });
    ASSERT_TRUE(m.split(20000u));
    feed(m, { 120u });
    ASSERT_TRUE(m.split(30000u));
    feed(m, { 130u });

    ASSERT_TRUE(m.undoSplit());
    ASSERT_TRUE(m.undoSplit());
    ASSERT_TRUE(m.undoSplit());
    EXPECT_EQ(m.recordedCount(), 0u);
    EXPECT_EQ(m.currentIndex(), 0u);

    ASSERT_TRUE(m.split(40000u));
    const auto *seg = m.recorded(0u);
    ASSERT_NE(seg, nullptr);
    EXPECT_EQ(seg->hrCount, 4u);
    EXPECT_EQ(seg->hrSum, 100u + 110u + 120u + 130u);
    EXPECT_EQ(seg->hrMax, 130u);
    EXPECT_EQ(seg->activeMs, 40000u) << "all four segments collapsed into one";
    EXPECT_EQ(seg->startMs, 0u);
}

TEST(LapAccumulatorTest, UndoMergesPausedTimeAsWellAsHeartRate)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    ASSERT_TRUE(m.pause(2000u));
    ASSERT_TRUE(m.resume(5000u));       // 3 s paused in segment 0
    ASSERT_TRUE(m.split(10000u));

    ASSERT_TRUE(m.pause(12000u));
    ASSERT_TRUE(m.resume(16000u));      // 4 s paused in segment 1
    ASSERT_TRUE(m.undoSplit());
    ASSERT_TRUE(m.split(20000u));

    const auto *seg = m.recorded(0u);
    ASSERT_NE(seg, nullptr);
    EXPECT_EQ(seg->pausedMs, 7000u) << "both pauses belong to the merged segment";
    EXPECT_EQ(seg->activeMs, 13000u) << "20 s wall, 7 s paused";
}
