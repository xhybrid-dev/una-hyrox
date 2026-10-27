// HybridX Run: the treadmill mode's arithmetic (Treadmill.hpp).

#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <limits>

#include "Treadmill.hpp"

namespace TM = RunVo2::Treadmill;

TEST(Treadmill, AnAcceptedEntryScalesTheTotals)
{
    // 4.8 km estimated over 30 minutes, the console said 5.0 km.
    const TM::Totals t = TM::correct(4800.0f, 5000.0f, 1800.0f, 3.2f);
    EXPECT_FLOAT_EQ(t.distanceM, 5000.0f);
    EXPECT_NEAR(t.k, 5000.0f / 4800.0f, 1e-6f);
    EXPECT_NEAR(t.speedAvg, 5000.0f / 1800.0f, 1e-5f);
    EXPECT_NEAR(t.speedMax, 3.2f * 5000.0f / 4800.0f, 1e-5f);
}

TEST(Treadmill, SkippedKeepsTheEstimate)
{
    // The stride model hands back the estimate when the entry is skipped or rejected.
    const TM::Totals t = TM::correct(4800.0f, 4800.0f, 1800.0f, 3.2f);
    EXPECT_FLOAT_EQ(t.distanceM, 4800.0f);
    EXPECT_FLOAT_EQ(t.k, 1.0f);
    EXPECT_FLOAT_EQ(t.speedMax, 3.2f);
}

TEST(Treadmill, NothingUsableIsZeroNotNan)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const TM::Totals a = TM::correct(0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(a.distanceM, 0.0f);
    EXPECT_EQ(a.speedAvg, 0.0f);
    EXPECT_EQ(a.k, 1.0f);
    const TM::Totals b = TM::correct(nan, nan, nan, nan);
    EXPECT_EQ(b.distanceM, 0.0f);
    EXPECT_EQ(b.speedAvg, 0.0f);
    EXPECT_EQ(b.speedMax, 0.0f);
    // No estimate (the cadence never came) but an entry: the entry stands, k stays 1.
    const TM::Totals c = TM::correct(0.0f, 1000.0f, 600.0f, 0.0f);
    EXPECT_FLOAT_EQ(c.distanceM, 1000.0f);
    EXPECT_FLOAT_EQ(c.k, 1.0f);
    EXPECT_NEAR(c.speedAvg, 1000.0f / 600.0f, 1e-5f);
}

TEST(Treadmill, HundredthsRoundTripInKmAndMiles)
{
    EXPECT_EQ(TM::toHundredths(5030.0f, false), 503u);
    EXPECT_EQ(TM::toHundredths(5034.9f, false), 503u);
    EXPECT_EQ(TM::toHundredths(5035.0f, false), 504u);
    EXPECT_FLOAT_EQ(TM::toMetres(503, false), 5030.0f);
    EXPECT_EQ(TM::toHundredths(TM::kMetresPerMile * 3.1f, true), 310u);
    EXPECT_NEAR(TM::toMetres(310, true), 3.1f * TM::kMetresPerMile, 0.01f);
}

TEST(Treadmill, TheEntryStaysInItsRange)
{
    EXPECT_EQ(TM::toHundredths(0.0f, false), TM::kMinHundredths);
    EXPECT_EQ(TM::toHundredths(-5.0f, false), TM::kMinHundredths);
    EXPECT_EQ(TM::toHundredths(std::numeric_limits<float>::infinity(), false), TM::kMinHundredths);
    EXPECT_EQ(TM::toHundredths(500000.0f, false), TM::kMaxHundredths);
    EXPECT_EQ(TM::step(TM::kMinHundredths, -1), TM::kMinHundredths);
    EXPECT_EQ(TM::step(TM::kMaxHundredths, 10), TM::kMaxHundredths);
    EXPECT_EQ(TM::step(500, 10), 510u);
    EXPECT_EQ(TM::step(500, -1), 499u);
}

TEST(Treadmill, TheRangeIsWhatTheStrideModelAccepts)
{
    // 4.80 km estimated, the SDK's band 0.5 to 2.0: 2.40 to 9.60 km.
    TM::Range r = TM::range(4800.0f, false, 0.5f, 2.0f);
    EXPECT_EQ(r.lo, 240u);
    EXPECT_EQ(r.hi, 960u);
    EXPECT_EQ(TM::step(955, 10, r), 960u);   // held at the top
    EXPECT_EQ(TM::step(245, -10, r), 240u);  // and at the bottom
    // Rounded inwards: 0.4111 km x 0.5 = 0.2056 -> 0.21; x 2 = 0.8222 -> 0.82.
    r = TM::range(411.1f, false, 0.5f, 2.0f);
    EXPECT_EQ(r.lo, 21u);
    EXPECT_EQ(r.hi, 82u);
    EXPECT_GE(TM::toMetres(r.lo, false), 411.1f * 0.5f);
    EXPECT_LE(TM::toMetres(r.hi, false), 411.1f * 2.0f);
    // Miles: 3 miles estimated, 1.50 to 6.00.
    r = TM::range(3.0f * TM::kMetresPerMile, true, 0.5f, 2.0f);
    EXPECT_EQ(r.lo, 150u);
    EXPECT_EQ(r.hi, 600u);
    // A tiny estimate keeps the picker's own floor; a huge one its ceiling.
    r = TM::range(50.0f, false, 0.5f, 2.0f);
    EXPECT_EQ(r.lo, TM::kMinHundredths);
    EXPECT_GE(r.hi, r.lo);
    r = TM::range(80000.0f, false, 0.5f, 2.0f);
    EXPECT_EQ(r.hi, TM::kMaxHundredths);
    // No estimate, or nonsense ratios: the whole range.
    r = TM::range(0.0f, false, 0.5f, 2.0f);
    EXPECT_EQ(r.lo, TM::kMinHundredths);
    EXPECT_EQ(r.hi, TM::kMaxHundredths);
    r = TM::range(4800.0f, false, 2.0f, 0.5f);
    EXPECT_EQ(r.hi, TM::kMaxHundredths);
}

TEST(Treadmill, TextIsTwoDecimals)
{
    char buf[8];
    EXPECT_EQ(TM::text(buf, sizeof(buf), 503), 4u);
    EXPECT_STREQ(buf, "5.03");
    TM::text(buf, sizeof(buf), 10);
    EXPECT_STREQ(buf, "0.10");
    TM::text(buf, sizeof(buf), TM::kMaxHundredths);
    EXPECT_STREQ(buf, "99.99");
    char tiny[3];
    EXPECT_EQ(TM::text(tiny, sizeof(tiny), 503), 2u);   // truncated, still terminated
    EXPECT_EQ(std::strlen(tiny), 2u);
    EXPECT_EQ(TM::text(nullptr, 4, 503), 0u);
}
