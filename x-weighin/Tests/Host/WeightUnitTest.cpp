#include "WeightUnit.hpp"

#include "WeighEntry.hpp"
#include "WeighText.hpp"

#include <gtest/gtest.h>

#include <string>

using namespace XWeighIn;

namespace
{
std::string w(int32_t h, WeightUnit u)
{
    char buf[32];
    formatWeight(h, u, buf, sizeof(buf));
    return buf;
}
std::string wu(int32_t h, WeightUnit u)
{
    char buf[32];
    formatWeightWithUnit(h, u, buf, sizeof(buf));
    return buf;
}
std::string ws(int32_t h, WeightUnit u)
{
    char buf[32];
    formatWeightShort(h, u, buf, sizeof(buf));
    return buf;
}
std::string wc(int32_t h, WeightUnit u, uint8_t decimals = 1)
{
    char buf[32];
    formatWeightChange(h, u, decimals, buf, sizeof(buf));
    return buf;
}
}  // namespace

TEST(WeightUnit, TheConfigValue)
{
    EXPECT_EQ(weightUnitFromConfig(0), WeightUnit::Kg);
    EXPECT_EQ(weightUnitFromConfig(1), WeightUnit::StLb);
    EXPECT_EQ(weightUnitFromConfig(2), WeightUnit::Lb);
    EXPECT_EQ(weightUnitFromConfig(3), WeightUnit::Kg);
    EXPECT_EQ(weightUnitFromConfig(-1), WeightUnit::Kg);
}

TEST(WeightUnit, Conversions)
{
    EXPECT_EQ(hundredthsToLbTenths(10000), 2205);   // 100 kg = 220.46 lb
    EXPECT_EQ(hundredthsToLbTenths(4536), 1000);    // 45.36 kg = 100.0 lb
    EXPECT_EQ(hundredthsToLbTenths(0), 0);
    EXPECT_EQ(hundredthsToLbTenths(-5), 0);
    EXPECT_EQ(lbTenthsToHundredths(1000), 4536);    // 100.0 lb = 45.36 kg
    EXPECT_EQ(lbTenthsToHundredths(0), 0);
    EXPECT_EQ(hundredthsToLbTenths(50000), 11023);  // the log's top, 500 kg
}

TEST(WeightUnit, TheLogsLimitsSurviveTheEntry)
{
    // The entry's ends convert to weights the log accepts, in every unit.
    for (WeightUnit u : { WeightUnit::Kg, WeightUnit::StLb, WeightUnit::Lb }) {
        EXPECT_GE(fromEntry(entryMin(u), u), kEntryMinHundredths);
        EXPECT_LE(fromEntry(entryMax(u), u), kEntryMaxHundredths);
        EXPECT_GE(fromEntry(entryMax(u), u), kEntryMaxHundredths - 10);
    }
}

TEST(WeightUnit, EveryPoundTenthSurvivesTheRoundTrip)
{
    // Entering 181.4 lb stores 0.01 kg, and shows 181.4 lb again.
    for (int32_t t = entryMin(WeightUnit::Lb); t <= entryMax(WeightUnit::Lb); ++t) {
        ASSERT_EQ(toEntry(fromEntry(t, WeightUnit::Lb), WeightUnit::Lb), t) << t;
    }
}

TEST(WeightUnit, Names)
{
    EXPECT_STREQ(weightUnitName(WeightUnit::Kg), "kg");
    EXPECT_STREQ(weightUnitName(WeightUnit::StLb), "st lb");
    EXPECT_STREQ(weightUnitName(WeightUnit::Lb), "lb");
    EXPECT_STREQ(changeUnitName(WeightUnit::Kg), "kg");
    EXPECT_STREQ(changeUnitName(WeightUnit::StLb), "lb");
    EXPECT_STREQ(changeUnitName(WeightUnit::Lb), "lb");
}

TEST(FormatWeight, Kilograms)
{
    EXPECT_EQ(w(8230, WeightUnit::Kg), "82.3");
    EXPECT_EQ(wu(8230, WeightUnit::Kg), "82.3 kg");
    EXPECT_EQ(ws(8250, WeightUnit::Kg), "82.5");
}

TEST(FormatWeight, Pounds)
{
    EXPECT_EQ(w(8230, WeightUnit::Lb), "181.4");   // 181.43
    EXPECT_EQ(wu(8230, WeightUnit::Lb), "181.4 lb");
    EXPECT_EQ(w(4536, WeightUnit::Lb), "100.0");
    EXPECT_EQ(w(500, WeightUnit::Lb), "11.0");
    EXPECT_EQ(ws(8230, WeightUnit::Lb), "181");
    EXPECT_EQ(ws(8250, WeightUnit::Lb), "182");    // 181.88
    EXPECT_EQ(w(50000, WeightUnit::Lb), "1102.3");
}

TEST(FormatWeight, StonesAndPounds)
{
    EXPECT_EQ(w(8230, WeightUnit::StLb), "12st 13.4");   // 181.4 lb = 12 st 13.4 lb
    EXPECT_EQ(wu(8230, WeightUnit::StLb), "12st 13.4lb");
    EXPECT_EQ(w(6350, WeightUnit::StLb), "10st 0.0");    // 140.0 lb exactly: 63.50 kg
    EXPECT_EQ(w(6340, WeightUnit::StLb), "9st 13.8");    // 139.8 lb
    EXPECT_EQ(w(6349, WeightUnit::StLb), "10st 0.0");    // 139.97 lb rounds up into the next stone
    EXPECT_EQ(w(500, WeightUnit::StLb), "0st 11.0");
    EXPECT_EQ(ws(8230, WeightUnit::StLb), "12st13");
    EXPECT_EQ(ws(6350, WeightUnit::StLb), "10st0");
    // Whole pounds carry into the stone: 139.6 lb rounds to 140.
    EXPECT_EQ(ws(6332, WeightUnit::StLb), "10st0");
    int st = -1, lb = -1;
    splitStoneLb(8230, st, lb);
    EXPECT_EQ(st, 12);
    EXPECT_EQ(lb, 13);
    splitStoneLb(0, st, lb);
    EXPECT_EQ(st, 0);
    EXPECT_EQ(lb, 0);
}

TEST(FormatWeight, NotSetIsDashesInEveryUnit)
{
    for (WeightUnit u : { WeightUnit::Kg, WeightUnit::StLb, WeightUnit::Lb }) {
        EXPECT_EQ(w(0, u), "--");
        EXPECT_EQ(w(-5, u), "--");
        EXPECT_EQ(w(10000000, u), "--");
        EXPECT_EQ(ws(0, u), "--");
    }
}

TEST(FormatWeight, TruncatesAndTerminates)
{
    char tiny[4];
    formatWeight(8230, WeightUnit::StLb, tiny, sizeof(tiny));
    EXPECT_EQ(std::string(tiny), "12s");
    EXPECT_EQ(formatWeight(8230, WeightUnit::Lb, nullptr, 8), 0u);
    char one[1];
    EXPECT_EQ(formatWeight(8230, WeightUnit::Lb, one, 0), 0u);
}

TEST(FormatWeightChange, InPoundsForBothImperialUnits)
{
    EXPECT_EQ(wc(-40, WeightUnit::Kg), "-0.4");
    EXPECT_EQ(wc(-40, WeightUnit::Lb), "-0.9");     // 0.88 lb
    EXPECT_EQ(wc(-40, WeightUnit::StLb), "-0.9");
    EXPECT_EQ(wc(35, WeightUnit::Lb), "+0.8");      // 0.77 lb
    EXPECT_EQ(wc(0, WeightUnit::Lb), "0.0");
    EXPECT_EQ(wc(1, WeightUnit::Lb), "0.0");        // 0.002 lb: no sign
    EXPECT_EQ(wc(-1040, WeightUnit::Lb), "-22.9");
    EXPECT_EQ(wc(12345, WeightUnit::Lb, 0), "+272");
    EXPECT_EQ(wc(10000000, WeightUnit::Lb), "--");
}
