#include <gtest/gtest.h>

#include "HeatHint.hpp"

using Sauna::HeatHint;
using Sauna::HeatHintConfig;
using Sauna::kX100Invalid;

// Every curve here is INVENTED to exercise the logic. None of it says anything
// about how the real sensor behaves in a sauna or a plunge (there is no such
// data yet), and the thresholds in HeatHintConfig are untuned placeholders.

namespace {

HeatHintConfig on()
{
    HeatHintConfig c;
    c.enabled = true;
    return c;
}

/// A reading a second from `from` to `to`, all the same.
void hold(HeatHint &h, uint32_t from, uint32_t to, int32_t value)
{
    // Offsets, not `t <= to`: the loop must survive the clock wrapping.
    for (uint32_t off = 0; off <= to - from; off += 1000) {
        h.add(from + off, value);
    }
}

/// Straight line from a to b over [from, to] ms, a reading a second.
void ramp(HeatHint &h, uint32_t from, uint32_t to, int32_t a, int32_t b)
{
    for (uint32_t off = 0; off <= to - from; off += 1000) {
        const int64_t v = a + static_cast<int64_t>(b - a) * off / (to - from);
        h.add(from + off, static_cast<int32_t>(v));
    }
}

} // namespace

TEST(HeatHint, IsOffByDefaultAndSaysUnknown)
{
    HeatHint h;
    hold(h, 0, 200000, 3000);
    ramp(h, 201000, 300000, 3000, 6000);
    EXPECT_EQ(h.level(), HeatHint::Level::Unknown);
    EXPECT_EQ(h.trend(), HeatHint::Trend::Unknown);
}

TEST(HeatHint, UnknownUntilTheBaselineIsIn)
{
    HeatHint h(on());
    hold(h, 0, 30000, 3000);
    EXPECT_FALSE(h.baselineReady());
    EXPECT_EQ(h.level(), HeatHint::Level::Unknown);
    hold(h, 31000, 60000, 3000);
    EXPECT_TRUE(h.baselineReady());
    EXPECT_EQ(h.baselineX100(), 3000);
    EXPECT_EQ(h.level(), HeatHint::Level::Similar);
}

TEST(HeatHint, TooFewSamplesKeepTheBaselineOpen)
{
    HeatHint h(on());
    h.add(0, 3000);
    h.add(60000, 3000);                    // a minute, but only two samples
    EXPECT_FALSE(h.baselineReady());
}

TEST(HeatHint, WarmerThenBackToSimilarWithHysteresis)
{
    HeatHint h(on());
    hold(h, 0, 60000, 3000);
    hold(h, 61000, 120000, 3400);          // +4.00, over the 3.00 threshold
    EXPECT_EQ(h.level(), HeatHint::Level::Warmer);
    EXPECT_NEAR(h.deltaX100(), 400, 20);

    // Down to +2.00: below the 3.00 that entered, above the 1.50 that leaves.
    hold(h, 121000, 200000, 3200);
    EXPECT_EQ(h.level(), HeatHint::Level::Warmer);
    // Down to +1.00: now it leaves.
    hold(h, 201000, 280000, 3100);
    EXPECT_EQ(h.level(), HeatHint::Level::Similar);
}

TEST(HeatHint, CoolerThenBack)
{
    HeatHint h(on());
    hold(h, 0, 60000, 3000);
    hold(h, 61000, 120000, 2600);          // -4.00
    EXPECT_EQ(h.level(), HeatHint::Level::Cooler);
    hold(h, 121000, 200000, 3000);
    EXPECT_EQ(h.level(), HeatHint::Level::Similar);
}

TEST(HeatHint, ASmallWobbleStaysSimilar)
{
    HeatHint h(on());
    hold(h, 0, 60000, 3000);
    for (uint32_t t = 61000; t <= 300000; t += 1000) {
        h.add(t, 3000 + ((t / 1000) % 2 == 0 ? 80 : -80));
    }
    EXPECT_EQ(h.level(), HeatHint::Level::Similar);
    EXPECT_EQ(h.trend(), HeatHint::Trend::Steady);
}

TEST(HeatHint, TrendNeedsAMinuteOfReadings)
{
    HeatHint h(on());
    hold(h, 0, 50000, 3000);
    EXPECT_EQ(h.trend(), HeatHint::Trend::Unknown);
    hold(h, 51000, 90000, 3000);
    EXPECT_EQ(h.trend(), HeatHint::Trend::Steady);
}

TEST(HeatHint, RisingAndFalling)
{
    HeatHint h(on());
    hold(h, 0, 90000, 3000);
    ramp(h, 91000, 190000, 3000, 3800);    // +8.00 in 100 s
    EXPECT_EQ(h.trend(), HeatHint::Trend::Rising);
    hold(h, 191000, 300000, 3800);
    EXPECT_EQ(h.trend(), HeatHint::Trend::Steady);
    ramp(h, 301000, 400000, 3800, 3000);
    EXPECT_EQ(h.trend(), HeatHint::Trend::Falling);
}

TEST(HeatHint, InvalidReadingsAreIgnored)
{
    HeatHint h(on());
    hold(h, 0, 60000, 3000);
    for (uint32_t t = 61000; t <= 120000; t += 1000) {
        h.add(t, kX100Invalid);
    }
    EXPECT_EQ(h.level(), HeatHint::Level::Similar);
    EXPECT_EQ(h.deltaX100(), 0);
}

TEST(HeatHint, WorksAcrossTheClockWrap)
{
    const uint32_t s = 0xFFFFFFFFu - 70000u;
    HeatHint h(on());
    hold(h, s, s + 60000u, 3000);
    EXPECT_TRUE(h.baselineReady());
    hold(h, s + 61000u, s + 150000u, 3500);
    EXPECT_EQ(h.level(), HeatHint::Level::Warmer);
}

TEST(HeatHint, ResetStartsAgainKeepingTheConfig)
{
    HeatHint h(on());
    hold(h, 0, 120000, 3000);
    EXPECT_TRUE(h.baselineReady());
    h.reset();
    EXPECT_FALSE(h.baselineReady());
    EXPECT_EQ(h.level(), HeatHint::Level::Unknown);
    hold(h, 200000, 260000, 2000);
    EXPECT_EQ(h.baselineX100(), 2000);
    EXPECT_EQ(h.level(), HeatHint::Level::Similar);   // still enabled
}
