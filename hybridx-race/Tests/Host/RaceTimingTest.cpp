/**
 * @file RaceTimingTest.cpp
 * @brief The invariants of brief 7.5, plus clock wrap and the lockout boundary.
 *
 * Each invariant gets a test named after it, so a failure says which promise
 * to the athlete has been broken.
 */

#include <gtest/gtest.h>

#include "RaceModel.hpp"

#include <vector>

using Race::Format;
using Race::RaceModel;
using Race::SegmentResult;
using State = Race::RaceModel::State;

namespace
{

RaceModel::Config config(Format format = Format::HalfA, bool roxzone = false,
                         uint32_t lockoutMs = 3000u)
{
    RaceModel::Config c;
    c.format = format;
    c.roxzone = roxzone;
    c.lockoutMs = lockoutMs;
    return c;
}

/// Copy out the closed segments so two runs can be compared.
std::vector<SegmentResult> snapshot(const RaceModel &m)
{
    std::vector<SegmentResult> out;
    for (uint8_t i = 0u; i < m.recordedCount(); ++i) {
        out.push_back(*m.recorded(i));
    }
    return out;
}

bool sameResult(const SegmentResult &a, const SegmentResult &b)
{
    return a.desc.type == b.desc.type && a.desc.round == b.desc.round &&
           a.desc.stationId == b.desc.stationId && a.startMs == b.startMs &&
           a.activeMs == b.activeMs && a.pausedMs == b.pausedMs &&
           a.hrSum == b.hrSum && a.hrCount == b.hrCount && a.hrMax == b.hrMax;
}

void expectSameSegments(const std::vector<SegmentResult> &a,
                        const std::vector<SegmentResult> &b)
{
    ASSERT_EQ(a.size(), b.size());
    for (size_t i = 0u; i < a.size(); ++i) {
        EXPECT_TRUE(sameResult(a[i], b[i])) << "segment " << i << " differs";
    }
}

}  // namespace

// -- Invariant 1 ----------------------------------------------------------------

TEST(RaceTimingTest, Invariant1_SegmentActiveTimesSumToTotalActiveTime)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    uint32_t t = 0u;
    for (uint8_t i = 0u; i < 8u; ++i) {
        t += 10000u + (i * 1234u);
        ASSERT_TRUE(m.split(t));
    }
    ASSERT_EQ(m.state(), State::Finished);

    uint32_t sum = 0u;
    for (uint8_t i = 0u; i < m.recordedCount(); ++i) {
        sum += m.recorded(i)->activeMs;
    }
    EXPECT_EQ(sum, m.totalActiveMs(t));
    EXPECT_EQ(sum, t) << "no pauses, so active time is the whole race";
}

// -- Invariant 2 ----------------------------------------------------------------

TEST(RaceTimingTest, Invariant2_RecordedCountEqualsCompletedSegments)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(Format::Full, true), 0u));
    EXPECT_EQ(m.plannedCount(), 31u);

    uint32_t t = 0u;
    for (uint8_t i = 0u; i < 31u; ++i) {
        t += 5000u;
        ASSERT_TRUE(m.split(t)) << "split " << int(i);
        EXPECT_EQ(m.recordedCount(), i + 1u);
    }
    EXPECT_EQ(m.recordedCount(), m.plannedCount());
}

// -- Invariant 3 ----------------------------------------------------------------

TEST(RaceTimingTest, Invariant3_ASplitInsideTheLockoutChangesNothing)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    ASSERT_TRUE(m.split(10000u));

    const auto before = snapshot(m);
    const uint32_t activeBefore = m.currentSegmentActiveMs(11000u);

    EXPECT_FALSE(m.split(11000u));
    EXPECT_FALSE(m.split(12500u));

    expectSameSegments(before, snapshot(m));
    EXPECT_EQ(m.currentIndex(), 1u);
    EXPECT_EQ(m.currentSegmentActiveMs(11000u), activeBefore);
}

TEST(RaceTimingTest, LockoutBoundaryIsInclusive)
{
    // Brief 12.1: exactly at N seconds is accepted.
    RaceModel m;
    ASSERT_TRUE(m.start(config(Format::HalfA, false, 3000u), 0u));
    ASSERT_TRUE(m.split(10000u));

    EXPECT_FALSE(m.split(12999u)) << "one millisecond short";
    EXPECT_TRUE(m.split(13000u)) << "exactly at the window";
}

TEST(RaceTimingTest, TheLockoutAlsoGuardsTheStart)
{
    // A double press on "Start race" must not split straight out of segment 0.
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 1000u));
    EXPECT_FALSE(m.split(1500u));
    EXPECT_EQ(m.recordedCount(), 0u);
    EXPECT_TRUE(m.split(4000u));
}

// -- Invariant 4 ----------------------------------------------------------------

TEST(RaceTimingTest, Invariant4_UndoThenTheSameSplitIsIndistinguishable)
{
    RaceModel plain;
    ASSERT_TRUE(plain.start(config(), 0u));
    ASSERT_TRUE(plain.split(10000u));
    ASSERT_TRUE(plain.split(25000u));

    RaceModel undone;
    ASSERT_TRUE(undone.start(config(), 0u));
    ASSERT_TRUE(undone.split(10000u));
    ASSERT_TRUE(undone.split(25000u));
    ASSERT_TRUE(undone.undoSplit());
    ASSERT_TRUE(undone.split(25000u));

    expectSameSegments(snapshot(plain), snapshot(undone));
    EXPECT_EQ(plain.currentIndex(), undone.currentIndex());
    EXPECT_EQ(plain.totalActiveMs(30000u), undone.totalActiveMs(30000u));
    EXPECT_EQ(plain.currentSegmentActiveMs(30000u), undone.currentSegmentActiveMs(30000u));
}

TEST(RaceTimingTest, UndoLeavesTotalTimeUntouched)
{
    // Brief 7.3: the undo merges two segments; it does not rewind the clock.
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    ASSERT_TRUE(m.split(10000u));
    ASSERT_TRUE(m.split(25000u));

    const uint32_t activeBefore = m.totalActiveMs(30000u);
    const uint32_t elapsedBefore = m.totalElapsedMs(30000u);

    ASSERT_TRUE(m.undoSplit());

    EXPECT_EQ(m.totalActiveMs(30000u), activeBefore);
    EXPECT_EQ(m.totalElapsedMs(30000u), elapsedBefore);
    EXPECT_EQ(m.currentSegmentActiveMs(30000u), 20000u)
            << "the reopened segment runs from 10000, not from 25000";
}

// -- Invariant 5 ----------------------------------------------------------------

TEST(RaceTimingTest, Invariant5_UndoFinishThenSplitEqualsOneFinishingSplit)
{
    RaceModel plain;
    ASSERT_TRUE(plain.start(config(), 0u));
    uint32_t t = 0u;
    for (uint8_t i = 0u; i < 8u; ++i) {
        t += 10000u;
        ASSERT_TRUE(plain.split(t));
    }

    RaceModel undone;
    ASSERT_TRUE(undone.start(config(), 0u));
    uint32_t u = 0u;
    for (uint8_t i = 0u; i < 8u; ++i) {
        u += 10000u;
        ASSERT_TRUE(undone.split(u));
    }
    ASSERT_TRUE(undone.undoFinish());
    ASSERT_TRUE(undone.split(u));

    EXPECT_EQ(undone.state(), State::Finished);
    EXPECT_TRUE(undone.completed());
    expectSameSegments(snapshot(plain), snapshot(undone));
    EXPECT_EQ(plain.totalActiveMs(u), undone.totalActiveMs(u));
    EXPECT_EQ(plain.totalElapsedMs(u), undone.totalElapsedMs(u));
}

// -- Invariant 6 ----------------------------------------------------------------

TEST(RaceTimingTest, Invariant6_PausingAddsToElapsedAndNotToSegmentActiveTime)
{
    constexpr uint32_t kPauseMs = 2000u;

    RaceModel unpaused;
    ASSERT_TRUE(unpaused.start(config(), 0u));

    RaceModel paused;
    ASSERT_TRUE(paused.start(config(), 0u));
    ASSERT_TRUE(paused.pause(3000u));
    ASSERT_TRUE(paused.resume(3000u + kPauseMs));

    // Same wall-clock instant for both.
    EXPECT_EQ(paused.totalElapsedMs(10000u), unpaused.totalElapsedMs(10000u))
            << "pause time is included in elapsed";
    EXPECT_EQ(paused.currentSegmentActiveMs(10000u),
              unpaused.currentSegmentActiveMs(10000u) - kPauseMs)
            << "and excluded from segment active time";
    EXPECT_EQ(paused.totalActiveMs(10000u), unpaused.totalActiveMs(10000u) - kPauseMs);
}

TEST(RaceTimingTest, PauseIsBankedOnTheSegmentItHappenedIn)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    ASSERT_TRUE(m.pause(2000u));
    ASSERT_TRUE(m.resume(7000u));       // 5 s paused inside segment 0
    ASSERT_TRUE(m.split(10000u));
    ASSERT_TRUE(m.split(20000u));

    ASSERT_NE(m.recorded(0u), nullptr);
    EXPECT_EQ(m.recorded(0u)->pausedMs, 5000u);
    EXPECT_EQ(m.recorded(0u)->activeMs, 5000u) << "10 s wall, 5 s paused";

    ASSERT_NE(m.recorded(1u), nullptr);
    EXPECT_EQ(m.recorded(1u)->pausedMs, 0u) << "the pause was not in this segment";
    EXPECT_EQ(m.recorded(1u)->activeMs, 10000u);
}

TEST(RaceTimingTest, AnUnresumedPauseIsStillExcludedWhenEndingEarly)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    ASSERT_TRUE(m.pause(4000u));
    ASSERT_TRUE(m.finishEarly(9000u));

    ASSERT_NE(m.recorded(0u), nullptr);
    EXPECT_EQ(m.recorded(0u)->pausedMs, 5000u);
    EXPECT_EQ(m.recorded(0u)->activeMs, 4000u) << "only the time before the pause";
}

// -- Invariant 7 ----------------------------------------------------------------

TEST(RaceTimingTest, Invariant7_SegmentCountsPerFormatAndRoxzoneMode)
{
    const struct
    {
        Format format;
        bool roxzone;
        uint8_t expected;
    } cases[] = {
        { Format::Full, false, 16u },  { Format::Full, true, 31u },
        { Format::HalfA, false, 8u },  { Format::HalfA, true, 15u },
        { Format::HalfB, false, 8u },  { Format::HalfB, true, 15u },
    };

    for (const auto &c : cases) {
        RaceModel m;
        ASSERT_TRUE(m.start(config(c.format, c.roxzone), 0u));
        EXPECT_EQ(m.plannedCount(), c.expected)
                << "format " << int(static_cast<uint8_t>(c.format))
                << " roxzone " << c.roxzone;
    }
}

// -- Clock wrap (brief 7.4, 14.6) -------------------------------------------------

TEST(RaceTimingTest, TimingSurvivesTheClockWrappingToZero)
{
    // The kernel's millisecond clock returns to zero. Start just short of the
    // wrap and run straight through it.
    constexpr uint32_t kStart = 0xFFFFF000u;

    RaceModel m;
    ASSERT_TRUE(m.start(config(), kStart));

    const uint32_t firstSplit = kStart + 10000u;   // wraps
    ASSERT_LT(firstSplit, kStart) << "the test must actually cross the wrap";
    ASSERT_TRUE(m.split(firstSplit));

    ASSERT_NE(m.recorded(0u), nullptr);
    EXPECT_EQ(m.recorded(0u)->activeMs, 10000u);

    const uint32_t secondSplit = firstSplit + 15000u;
    ASSERT_TRUE(m.split(secondSplit));
    EXPECT_EQ(m.recorded(1u)->activeMs, 15000u);
    EXPECT_EQ(m.totalElapsedMs(secondSplit), 25000u);
    EXPECT_EQ(m.totalActiveMs(secondSplit), 25000u);
}

TEST(RaceTimingTest, TheLockoutIsCorrectAcrossTheWrap)
{
    constexpr uint32_t kStart = 0xFFFFFF00u;

    RaceModel m;
    ASSERT_TRUE(m.start(config(Format::HalfA, false, 3000u), kStart));
    ASSERT_TRUE(m.split(kStart + 10000u));

    EXPECT_FALSE(m.split(kStart + 11000u)) << "still inside the lockout, post-wrap";
    EXPECT_TRUE(m.split(kStart + 13000u));
}

// -- Elapsed and totals -----------------------------------------------------------

TEST(RaceTimingTest, ElapsedFreezesOnceTheRaceIsOver)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    uint32_t t = 0u;
    for (uint8_t i = 0u; i < 8u; ++i) {
        t += 10000u;
        ASSERT_TRUE(m.split(t));
    }

    EXPECT_EQ(m.totalElapsedMs(t), 80000u);
    EXPECT_EQ(m.totalElapsedMs(t + 60000u), 80000u) << "the clock has stopped";
    EXPECT_EQ(m.totalActiveMs(t + 60000u), 80000u);
}

TEST(RaceTimingTest, ElapsedIsZeroBeforeTheRaceStarts)
{
    RaceModel m;
    EXPECT_EQ(m.totalElapsedMs(50000u), 0u);
    EXPECT_EQ(m.totalActiveMs(50000u), 0u);
    EXPECT_EQ(m.currentSegmentActiveMs(50000u), 0u);
}

TEST(RaceTimingTest, TotalsByTypeDriveTheSummary)
{
    // Brief 10.2 wants runs, stations and Roxzone totals separately.
    RaceModel m;
    ASSERT_TRUE(m.start(config(Format::HalfA, true), 0u));

    uint32_t t = 0u;
    const uint32_t steps[15] = {
        10000u, 4000u, 20000u, 4000u,  // round 1: run, rox in, station, rox out
        11000u, 4000u, 21000u, 4000u,  // round 2
        12000u, 4000u, 22000u, 4000u,  // round 3
        13000u, 4000u, 23000u,         // round 4, no rox out
    };
    for (const uint32_t step : steps) {
        t += step;
        ASSERT_TRUE(m.split(t));
    }
    ASSERT_EQ(m.state(), State::Finished);

    EXPECT_EQ(m.totalActiveMsOfType(Race::SegmentType::Run), 46000u);
    EXPECT_EQ(m.totalActiveMsOfType(Race::SegmentType::Station), 86000u);
    EXPECT_EQ(m.totalActiveMsOfType(Race::SegmentType::RoxIn), 16000u);
    EXPECT_EQ(m.totalActiveMsOfType(Race::SegmentType::RoxOut), 12000u);

    const uint32_t byType = m.totalActiveMsOfType(Race::SegmentType::Run) +
                            m.totalActiveMsOfType(Race::SegmentType::Station) +
                            m.totalActiveMsOfType(Race::SegmentType::RoxIn) +
                            m.totalActiveMsOfType(Race::SegmentType::RoxOut);
    EXPECT_EQ(byType, m.totalActiveMs(t)) << "the four totals must account for the race";
}

// -- A whole race, as the athlete would run it -------------------------------------

TEST(RaceTimingTest, AFullRaceOfNinetyMinutesAddsUp)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(Format::Full, false), 0u));

    // ~4:30 runs and ~1:30 stations: a 48-minute simulation, run instantly.
    uint32_t t = 0u;
    for (uint8_t i = 0u; i < 16u; ++i) {
        t += (i % 2u == 0u) ? 270000u : 90000u;
        ASSERT_TRUE(m.split(t)) << "split " << int(i);
    }

    EXPECT_EQ(m.state(), State::Finished);
    EXPECT_TRUE(m.completed());
    EXPECT_EQ(m.recordedCount(), 16u);
    EXPECT_EQ(m.totalElapsedMs(t), 8u * (270000u + 90000u));
    EXPECT_EQ(m.totalActiveMsOfType(Race::SegmentType::Run), 8u * 270000u);
    EXPECT_EQ(m.totalActiveMsOfType(Race::SegmentType::Station), 8u * 90000u);
}
