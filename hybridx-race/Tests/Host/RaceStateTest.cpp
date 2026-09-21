/**
 * @file RaceStateTest.cpp
 * @brief Every transition of brief 7.3, including the illegal ones.
 */

#include <gtest/gtest.h>

#include "RaceModel.hpp"

using Race::Format;
using Race::RaceModel;
using State = Race::RaceModel::State;

namespace
{

/// A half race with Roxzone off: 8 segments, short enough to drive by hand.
RaceModel::Config halfConfig()
{
    RaceModel::Config c;
    c.format = Format::HalfA;
    c.roxzone = false;
    c.lockoutMs = 3000u;
    return c;
}

/// Start a race at t=0 and split past the lockout `count` times.
void runSplits(RaceModel &m, uint8_t count, uint32_t stepMs = 10000u)
{
    uint32_t t = 0u;
    for (uint8_t i = 0u; i < count; ++i) {
        t += stepMs;
        m.split(t);
    }
}

}  // namespace

// -- START ---------------------------------------------------------------------

TEST(RaceStateTest, StartsFromIdleIntoRunning)
{
    RaceModel m;
    EXPECT_EQ(m.state(), State::Idle);
    EXPECT_EQ(m.plannedCount(), 0u);

    ASSERT_TRUE(m.start(halfConfig(), 1000u));
    EXPECT_EQ(m.state(), State::Running);
    EXPECT_EQ(m.plannedCount(), 8u);
    EXPECT_EQ(m.currentIndex(), 0u);
    EXPECT_EQ(m.recordedCount(), 0u);
    ASSERT_NE(m.currentSegment(), nullptr);
    EXPECT_EQ(m.currentSegment()->type, Race::SegmentType::Run);
}

TEST(RaceStateTest, StartingTwiceIsRefused)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    EXPECT_FALSE(m.start(halfConfig(), 5000u)) << "a race is already under way";
    EXPECT_EQ(m.recordedCount(), 0u);
}

// -- SPLIT ---------------------------------------------------------------------

TEST(RaceStateTest, SplitClosesOneSegmentAndOpensTheNext)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));

    ASSERT_TRUE(m.split(10000u));
    EXPECT_EQ(m.recordedCount(), 1u);
    EXPECT_EQ(m.currentIndex(), 1u);
    EXPECT_EQ(m.state(), State::Running);
    ASSERT_NE(m.currentSegment(), nullptr);
    EXPECT_EQ(m.currentSegment()->type, Race::SegmentType::Station);
}

TEST(RaceStateTest, SplitInsideTheLockoutIsIgnoredEntirely)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    ASSERT_TRUE(m.split(10000u));

    EXPECT_FALSE(m.split(10001u)) << "a double press must not register";
    EXPECT_FALSE(m.split(12999u));
    EXPECT_EQ(m.recordedCount(), 1u) << "nothing was recorded";
    EXPECT_EQ(m.currentIndex(), 1u);
}

TEST(RaceStateTest, TheFinalSplitFinishesTheRace)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    runSplits(m, 8u);

    EXPECT_EQ(m.state(), State::Finished);
    EXPECT_EQ(m.recordedCount(), 8u);
    EXPECT_TRUE(m.completed());
    EXPECT_EQ(m.currentSegment(), nullptr) << "no segment is open once finished";
}

TEST(RaceStateTest, SplittingAfterTheFinishDoesNothing)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    runSplits(m, 8u);

    EXPECT_FALSE(m.split(200000u));
    EXPECT_EQ(m.recordedCount(), 8u);
    EXPECT_EQ(m.state(), State::Finished);
}

// -- UNDO_SPLIT -----------------------------------------------------------------

TEST(RaceStateTest, UndoSplitStepsBackOneSegment)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    ASSERT_TRUE(m.split(10000u));
    ASSERT_TRUE(m.split(20000u));
    ASSERT_EQ(m.recordedCount(), 2u);

    EXPECT_TRUE(m.undoSplit());
    EXPECT_EQ(m.recordedCount(), 1u);
    EXPECT_EQ(m.currentIndex(), 1u);
    EXPECT_EQ(m.state(), State::Running);
}

TEST(RaceStateTest, UndoSplitAtIndexZeroDoesNothing)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));

    EXPECT_FALSE(m.undoSplit()) << "there is no split to take back";
    EXPECT_EQ(m.recordedCount(), 0u);
    EXPECT_EQ(m.currentIndex(), 0u);
    EXPECT_EQ(m.state(), State::Running);
}

TEST(RaceStateTest, UndoSplitIsRefusedWhenNotRunning)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    ASSERT_TRUE(m.split(10000u));
    ASSERT_TRUE(m.pause(12000u));

    EXPECT_FALSE(m.undoSplit()) << "brief 7.3 allows UNDO_SPLIT from RUNNING only";
    EXPECT_EQ(m.recordedCount(), 1u);
}

TEST(RaceStateTest, ASplitImmediatelyAfterAnUndoIsNotBlockedByTheLockout)
{
    // The press that was taken back must not go on suppressing later presses.
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    ASSERT_TRUE(m.split(10000u));
    ASSERT_TRUE(m.split(20000u));
    ASSERT_TRUE(m.undoSplit());

    EXPECT_TRUE(m.split(20100u)) << "20100 is well past the lockout from 10000";
    EXPECT_EQ(m.recordedCount(), 2u);
}

// -- PAUSE and RESUME ------------------------------------------------------------

TEST(RaceStateTest, PauseAndResumeToggleTheState)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));

    EXPECT_TRUE(m.pause(5000u));
    EXPECT_EQ(m.state(), State::Paused);
    EXPECT_TRUE(m.resume(8000u));
    EXPECT_EQ(m.state(), State::Running);
}

TEST(RaceStateTest, RedundantPauseAndResumeAreRefused)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));

    EXPECT_FALSE(m.resume(1000u)) << "not paused";
    EXPECT_TRUE(m.pause(5000u));
    EXPECT_FALSE(m.pause(6000u)) << "already paused";
    EXPECT_EQ(m.state(), State::Paused);
}

TEST(RaceStateTest, SplittingWhilePausedIsRefused)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    ASSERT_TRUE(m.pause(5000u));

    EXPECT_FALSE(m.split(20000u));
    EXPECT_EQ(m.recordedCount(), 0u);
}

// -- FINISH_EARLY ----------------------------------------------------------------

TEST(RaceStateTest, FinishEarlyFromRunningClosesTheOpenSegment)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    ASSERT_TRUE(m.split(10000u));

    EXPECT_TRUE(m.finishEarly(15000u));
    EXPECT_EQ(m.state(), State::Finished);
    EXPECT_EQ(m.recordedCount(), 2u) << "the open segment is banked too";
    EXPECT_FALSE(m.completed()) << "the race is incomplete";
}

TEST(RaceStateTest, FinishEarlyWorksFromPaused)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    ASSERT_TRUE(m.pause(5000u));

    EXPECT_TRUE(m.finishEarly(9000u));
    EXPECT_EQ(m.state(), State::Finished);
    EXPECT_FALSE(m.completed());
}

// -- UNDO_FINISH -----------------------------------------------------------------

TEST(RaceStateTest, UndoFinishReopensTheLastSegment)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    runSplits(m, 8u);
    ASSERT_EQ(m.state(), State::Finished);

    EXPECT_TRUE(m.undoFinish());
    EXPECT_EQ(m.state(), State::Running);
    EXPECT_EQ(m.recordedCount(), 7u);
    EXPECT_EQ(m.currentIndex(), 7u);
    EXPECT_FALSE(m.completed());
}

TEST(RaceStateTest, UndoFinishIsRefusedAfterEndingEarly)
{
    // Brief 7.3: only a race reached by the final SPLIT can be undone. Ending
    // early was a menu choice, not a press to take back.
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    ASSERT_TRUE(m.split(10000u));
    ASSERT_TRUE(m.finishEarly(15000u));

    EXPECT_FALSE(m.undoFinish());
    EXPECT_EQ(m.state(), State::Finished);
}

TEST(RaceStateTest, UndoFinishIsRefusedWhenNotFinished)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    EXPECT_FALSE(m.undoFinish());
}

// -- SAVE and DISCARD -------------------------------------------------------------

TEST(RaceStateTest, SaveOnlyFromFinished)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    EXPECT_FALSE(m.save()) << "cannot save a running race";

    runSplits(m, 8u);
    EXPECT_TRUE(m.save());
    EXPECT_EQ(m.state(), State::Saved);
    EXPECT_FALSE(m.save()) << "already saved";
}

TEST(RaceStateTest, DiscardFromRunningPausedAndFinished)
{
    {
        RaceModel m;
        ASSERT_TRUE(m.start(halfConfig(), 0u));
        EXPECT_TRUE(m.discard());
        EXPECT_EQ(m.state(), State::Discarded);
    }
    {
        RaceModel m;
        ASSERT_TRUE(m.start(halfConfig(), 0u));
        ASSERT_TRUE(m.pause(1000u));
        EXPECT_TRUE(m.discard());
    }
    {
        RaceModel m;
        ASSERT_TRUE(m.start(halfConfig(), 0u));
        runSplits(m, 8u);
        EXPECT_TRUE(m.discard());
    }
}

TEST(RaceStateTest, DiscardIsRefusedFromIdleAndSaved)
{
    RaceModel idle;
    EXPECT_FALSE(idle.discard());

    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    runSplits(m, 8u);
    ASSERT_TRUE(m.save());
    EXPECT_FALSE(m.discard()) << "a saved race is no longer ours to abandon";
}

// -- Queries stay safe -------------------------------------------------------------

TEST(RaceStateTest, RecordedIsBoundsChecked)
{
    RaceModel m;
    EXPECT_EQ(m.recorded(0u), nullptr);

    ASSERT_TRUE(m.start(halfConfig(), 0u));
    ASSERT_TRUE(m.split(10000u));

    EXPECT_NE(m.recorded(0u), nullptr);
    EXPECT_EQ(m.recorded(1u), nullptr) << "not closed yet";
    EXPECT_EQ(m.recorded(200u), nullptr);
}

TEST(RaceStateTest, NextSegmentIsNullOnTheLastSegment)
{
    RaceModel m;
    ASSERT_TRUE(m.start(halfConfig(), 0u));
    ASSERT_NE(m.nextSegment(), nullptr);

    runSplits(m, 7u);
    EXPECT_EQ(m.currentIndex(), 7u) << "on the final segment";
    EXPECT_EQ(m.nextSegment(), nullptr);
}
