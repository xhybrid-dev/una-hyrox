#include "WeighEntry.hpp"

#include <gtest/gtest.h>

using namespace XWeighIn;

TEST(StartBase, LastLoggedThenProfileThenFallback)
{
    EXPECT_EQ(startBase(8234, 9000), 8234);   // the log wins
    EXPECT_EQ(startBase(0, 8200), 8200);      // no log: the UNA profile
    EXPECT_EQ(startBase(0, 0), kEntryFallback);
    EXPECT_EQ(startBase(-5, -5), kEntryFallback);
}

TEST(StartValue, KilogramsInTenthsHalvesUp)
{
    EXPECT_EQ(startValue(8234, WeightUnit::Kg), 823);
    EXPECT_EQ(startValue(8235, WeightUnit::Kg), 824);
    EXPECT_EQ(startValue(kEntryFallback, WeightUnit::Kg), 700);
}

TEST(StartValue, PoundsInTenthsFromTheUnroundedWeight)
{
    // 82.34 kg is 181.52 lb: rounding to 82.3 kg first would give 181.4.
    EXPECT_EQ(startValue(8234, WeightUnit::Lb), 1815);
    EXPECT_EQ(startValue(8234, WeightUnit::StLb), 1815);
    EXPECT_EQ(startValue(kEntryFallback, WeightUnit::Lb), 1543);
}

TEST(StartValue, ClampedToTheRange)
{
    for (WeightUnit u : { WeightUnit::Kg, WeightUnit::StLb, WeightUnit::Lb }) {
        EXPECT_EQ(startValue(60000, u), entryMax(u));
        EXPECT_EQ(startValue(50, u), entryMin(u));
    }
    EXPECT_EQ(entryMin(WeightUnit::Kg), 10);
    EXPECT_EQ(entryMax(WeightUnit::Kg), 5000);
}

TEST(StepEntry, TenthsWithinTheRange)
{
    EXPECT_EQ(stepEntry(823, 1, WeightUnit::Kg), 824);
    EXPECT_EQ(stepEntry(823, -3, WeightUnit::Kg), 820);
    EXPECT_EQ(stepEntry(823, 0, WeightUnit::Kg), 823);
    EXPECT_EQ(stepEntry(1815, 1, WeightUnit::Lb), 1816);
    for (WeightUnit u : { WeightUnit::Kg, WeightUnit::StLb, WeightUnit::Lb }) {
        EXPECT_EQ(stepEntry(entryMin(u), -1, u), entryMin(u));
        EXPECT_EQ(stepEntry(entryMax(u), 1, u), entryMax(u));
        EXPECT_EQ(stepEntry(entryMax(u) - 1, 4, u), entryMax(u));
    }
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
