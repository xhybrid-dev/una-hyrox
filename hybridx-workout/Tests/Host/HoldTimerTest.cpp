#include <gtest/gtest.h>

#include "HoldTimer.hpp"

using Workout::HoldTimer;

TEST(HoldTimer, FiresOnceAtTheHoldLengthAndNotBefore)
{
    HoldTimer t(1500);
    t.press(1000);
    EXPECT_FALSE(t.poll(1000));
    EXPECT_FALSE(t.poll(2499));
    EXPECT_TRUE(t.poll(2500));
    EXPECT_FALSE(t.poll(2501));         // only once
    EXPECT_FALSE(t.poll(9000));
}

TEST(HoldTimer, ReleasingEarlyCancelsAndNeverFires)
{
    HoldTimer t(1500);
    t.press(0);
    EXPECT_FALSE(t.poll(1000));
    t.release(1000);
    EXPECT_FALSE(t.poll(5000));
    EXPECT_FALSE(t.held());
}

TEST(HoldTimer, ARepeatedPressCodeDoesNotRestartTheHold)
{
    HoldTimer t(1500);
    t.press(0);
    t.press(1000);                      // ignored
    EXPECT_TRUE(t.poll(1500));
}

TEST(HoldTimer, ProgressRunsFromZeroToAThousand)
{
    HoldTimer t(1500);
    EXPECT_EQ(t.progressPermille(0), 0u);
    t.press(100);
    EXPECT_EQ(t.progressPermille(100), 0u);
    EXPECT_EQ(t.progressPermille(850), 500u);
    EXPECT_EQ(t.progressPermille(1600), 1000u);
    EXPECT_EQ(t.progressPermille(90000), 1000u);   // clamped
    t.release(1700);
    EXPECT_EQ(t.progressPermille(1700), 0u);
}

TEST(HoldTimer, SurvivesTheMillisecondClockWrapping)
{
    HoldTimer t(1500);
    const uint32_t start = 0xFFFFFF00u;            // 256 ms before the wrap
    t.press(start);
    EXPECT_FALSE(t.poll(start + 1000));            // wrapped: now 744
    EXPECT_EQ(t.progressPermille(start + 750), 500u);
    EXPECT_TRUE(t.poll(start + 1500));
}

// The order the PC simulator (which follows the kernel) sends a quick press:
// press, click, release.
TEST(HoldTimer, AQuickTapCountsOnTheClickAndNotAgainOnTheRelease)
{
    HoldTimer t(800);
    t.press(0);
    EXPECT_TRUE(t.takeClick(100));
    EXPECT_FALSE(t.release(105));
}

// A press of 500 ms or more sends no click, but it is not a hold either.
TEST(HoldTimer, ASlowTapWithNoClickCountsOnTheRelease)
{
    HoldTimer t(800);
    t.press(0);
    EXPECT_FALSE(t.poll(600));
    EXPECT_TRUE(t.release(600));
}

// The other order a kernel might use: press, release, click.
TEST(HoldTimer, AReleaseThenItsClickCountsOnce)
{
    HoldTimer t(800);
    t.press(0);
    EXPECT_TRUE(t.release(120));
    EXPECT_FALSE(t.takeClick(125));                // the duplicate
    t.press(500);
    EXPECT_TRUE(t.release(620));                   // and the next tap is a new one
    EXPECT_FALSE(t.takeClick(625));
}

TEST(HoldTimer, AClickWithNoPressCodesAtAllCounts)
{
    HoldTimer t(800);
    EXPECT_TRUE(t.takeClick(100));
    EXPECT_TRUE(t.takeClick(200));
}

TEST(HoldTimer, AFiredHoldNeverCountsAsATap)
{
    HoldTimer t(800);
    t.press(0);
    ASSERT_TRUE(t.poll(800));
    EXPECT_FALSE(t.release(1000));                 // not a tap
    EXPECT_FALSE(t.takeClick(1005));               // and a click after it is the hold's
    EXPECT_TRUE(t.takeClick(1500));                // a later click is a new tap
}

TEST(HoldTimer, AClickWhileHeldAfterFiringIsNotATap)
{
    HoldTimer t(800);
    t.press(0);
    ASSERT_TRUE(t.poll(800));
    EXPECT_FALSE(t.takeClick(900));
    EXPECT_FALSE(t.release(1000));
}

TEST(HoldTimer, ANeverArrivingClickCannotEatTheNextTap)
{
    HoldTimer t(800);
    t.press(0);
    ASSERT_TRUE(t.poll(800));
    t.release(1000);                               // no click came
    EXPECT_TRUE(t.takeClick(1000 + HoldTimer::kClickMs));
}

TEST(HoldTimer, ACancelledHoldIsATapOnRelease)
{
    HoldTimer t(800);
    t.press(0);
    EXPECT_FALSE(t.poll(500));
    EXPECT_TRUE(t.release(500));                   // let go before it fired
}

TEST(HoldTimer, CancelDropsAHoldAndItsSwallowedClick)
{
    HoldTimer t(800);
    t.press(0);
    t.cancel();                                    // a suspend swallowed the release
    EXPECT_FALSE(t.held());
    EXPECT_FALSE(t.poll(5000));

    t.press(6000);
    ASSERT_TRUE(t.poll(6800));
    t.release(7000);
    t.cancel();                                    // the duplicate-click guard goes too
    EXPECT_TRUE(t.takeClick(7005));
}

TEST(HoldTimer, ACanBeHeldAgainAfterFiring)
{
    HoldTimer t(800);
    t.press(0);
    ASSERT_TRUE(t.poll(800));
    t.release(900);
    t.press(2000);
    EXPECT_TRUE(t.poll(2800));
}

TEST(HoldTimer, AZeroLengthIsTreatedAsOneMillisecond)
{
    HoldTimer t(0);
    t.press(10);
    EXPECT_TRUE(t.poll(11));
}

TEST(HoldTimer, PendingIsTrueOnlyBetweenPressAndFiring)
{
    HoldTimer t(800);
    EXPECT_FALSE(t.pending());
    t.press(0);
    EXPECT_TRUE(t.pending());
    ASSERT_TRUE(t.poll(800));
    EXPECT_TRUE(t.held());              // still down, waiting for the release
    EXPECT_FALSE(t.pending());          // but nothing left to show
    t.release(900);
    EXPECT_FALSE(t.pending());
}
