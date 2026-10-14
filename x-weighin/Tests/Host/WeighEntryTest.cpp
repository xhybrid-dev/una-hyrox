#include "WeighEntry.hpp"

#include <gtest/gtest.h>

using namespace XWeighIn;

TEST(StartValue, LastLoggedThenProfileThenFallback)
{
    EXPECT_EQ(startValue(8234, 9000), 8230);   // the log wins, rounded to 0.1
    EXPECT_EQ(startValue(8235, 0), 8240);      // halves up
    EXPECT_EQ(startValue(0, 8200), 8200);      // no log: the UNA profile
    EXPECT_EQ(startValue(0, 8234), 8230);
    EXPECT_EQ(startValue(0, 0), kEntryFallback);
    EXPECT_EQ(startValue(-5, -5), kEntryFallback);
}

TEST(StartValue, ClampedToTheRange)
{
    EXPECT_EQ(startValue(60000, 0), kEntryMax);
    EXPECT_EQ(startValue(50, 0), kEntryMin);
}

TEST(StepEntry, TenthsWithinTheRange)
{
    EXPECT_EQ(stepEntry(8230, 1), 8240);
    EXPECT_EQ(stepEntry(8230, -3), 8200);
    EXPECT_EQ(stepEntry(8230, 0), 8230);
    EXPECT_EQ(stepEntry(kEntryMin, -1), kEntryMin);
    EXPECT_EQ(stepEntry(kEntryMax, 1), kEntryMax);
    EXPECT_EQ(stepEntry(kEntryMax - 5, 1), kEntryMax);
}

TEST(HoldRepeat, ATapIsOneClick)
{
    HoldRepeat h;
    h.press(1000);
    EXPECT_EQ(h.tick(1100), 0u);
    h.release(1150);
    EXPECT_TRUE(h.takeClick(1160));
    EXPECT_EQ(h.tick(2000), 0u);   // nothing held any more
}

TEST(HoldRepeat, AClickWithoutAPressStillCounts)
{
    // The simulator's keyboard sends clicks only.
    HoldRepeat h;
    EXPECT_TRUE(h.takeClick(500));
}

TEST(HoldRepeat, AHoldRepeatsAfterTheStartDelayAndQuickens)
{
    HoldRepeat h;
    h.press(0);
    EXPECT_EQ(h.tick(599), 0u);
    EXPECT_EQ(h.tick(600), 1u);    // first repeat
    EXPECT_EQ(h.tick(799), 0u);
    EXPECT_EQ(h.tick(800), 1u);    // +200
    EXPECT_EQ(h.tick(974), 0u);
    EXPECT_EQ(h.tick(975), 1u);    // +175: quicker
    // Ticking every 50 ms for three seconds: it settles at one step per tick.
    uint32_t steps = 0;
    for (uint32_t t = 1000; t <= 4000; t += 50) {
        steps += h.tick(t);
    }
    EXPECT_GE(steps, 50u);
    EXPECT_LE(steps, 61u);
    EXPECT_EQ(h.tick(4050), 1u);
}

TEST(HoldRepeat, TheClickEndingAHoldIsNotAnExtraStep)
{
    HoldRepeat h;
    h.press(0);
    EXPECT_EQ(h.tick(600), 1u);
    h.release(700);
    EXPECT_FALSE(h.takeClick(720));
    // The next tap counts again.
    h.press(2000);
    h.release(2100);
    EXPECT_TRUE(h.takeClick(2110));
}

TEST(HoldRepeat, ALateClickIsATap)
{
    HoldRepeat h;
    h.press(0);
    EXPECT_EQ(h.tick(600), 1u);
    h.release(700);
    EXPECT_TRUE(h.takeClick(700 + HoldRepeat::kClickMs + 1));
}

TEST(HoldRepeat, AHoldThatNeverRepeatedEndsInAClick)
{
    HoldRepeat h;
    h.press(0);
    EXPECT_EQ(h.tick(500), 0u);
    h.release(550);
    EXPECT_TRUE(h.takeClick(560));
}

TEST(HoldRepeat, SurvivesTheClockWrapping)
{
    HoldRepeat h;
    const uint32_t start = 0xFFFFFF00u;
    h.press(start);
    EXPECT_EQ(h.tick(start + 599u), 0u);
    EXPECT_EQ(h.tick(start + 600u), 1u);   // start + 600 wraps past zero
}

TEST(HoldRepeat, AStalledGuiDoesNotLeap)
{
    HoldRepeat h;
    h.press(0);
    EXPECT_EQ(h.tick(10000), HoldRepeat::kMaxBurst);
    EXPECT_LE(h.tick(10050), 1u);
}
