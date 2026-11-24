// A night's battery drain and coverage.
#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <string>

#include "NightSummary.hpp"

using namespace EnergyProbe;

TEST(NightSummary, DrainPerHour)
{
    EXPECT_EQ(drainPerHourX100(8000, 7650, 10 * 3600), 35);      // 3.5 % in 10 h
    EXPECT_EQ(drainPerHourX100(8000, 8000, 3600), 0);
    EXPECT_EQ(drainPerHourX100(7000, 7100, 3600), -100);         // it rose (charged)
    EXPECT_EQ(drainPerHourX100(-1, 7000, 3600), INT32_MIN);
    EXPECT_EQ(drainPerHourX100(8000, 7000, 30), INT32_MIN);      // under a minute
}

TEST(NightSummary, AWholeNight)
{
    NightTracker t;
    const uint32_t start = 1795000000u;
    t.open(20416, 1, start, 8000, 3100, 0);
    for (uint32_t m = 1; m <= 600; ++m) {   // ten hours of minute rows
        const int32_t pct = 8000 - static_cast<int32_t>(m * 350 / 600);
        const int32_t mah = 3100 - static_cast<int32_t>(m * 25 / 600);
        t.addRow(start + m * 60, 1, m > 30, m > 30, 0, pct, mah, true, -250);
    }
    const NightResult r = t.close(start + 600 * 60, 7650, 3075);
    EXPECT_FALSE(t.isOpen());
    EXPECT_EQ(r.hoursX100, 1000);
    EXPECT_EQ(r.drainX100, 35);
    EXPECT_EQ(r.mahDropX10, 25);
    EXPECT_EQ(r.capMaX100, 25);      // 2.5 mAh in 10 h = 0.25 mA
    EXPECT_EQ(r.gaugeMaX100, -250);
    EXPECT_EQ(r.rows, 600u);
    EXPECT_EQ(r.wornMin, 570u);
    EXPECT_EQ(r.hrMin, 570u);
    EXPECT_EQ(r.flags, 0);

    char row[256];
    ASSERT_GT(formatNight(row, sizeof(row), r, "full", 20261124u), 0u);
    EXPECT_EQ(std::string(row),
              "20261124,full,1795000000,1795036000,1000,8000,7650,35,25,25,-250,600,570,570,0\n");
    // The header has as many columns.
    const std::string h = nightHeader();
    EXPECT_EQ(std::count(h.begin(), h.end(), ','), std::count(row, row + std::strlen(row), ','));
}

TEST(NightSummary, Flags)
{
    NightTracker t;
    t.open(1, 0, 1000, 5000, -1, 45);    // 45 minutes late
    t.addRow(1600, 10, false, false, 1, 5100, -1, false, 0);   // on the charger
    NightResult r = t.close(4600, -1, -1);
    EXPECT_TRUE(r.flags & kFlagPartial);
    EXPECT_TRUE(r.flags & kFlagCharged);
    EXPECT_EQ(r.endPct, 5100);           // the last reading stands in
    EXPECT_EQ(r.mahDropX10, INT32_MIN);
    EXPECT_EQ(r.gaugeMaX100, INT32_MIN);

    char row[256];
    ASSERT_GT(formatNight(row, sizeof(row), r, "off", 19700102u), 0u);
    EXPECT_NE(std::string(row).find(",,,"), std::string::npos);   // the unknown currents are empty

    NightTracker none;
    none.open(1, 0, 1000, -1, -1, 0);
    r = none.close(9000, -1, -1);
    EXPECT_TRUE(r.flags & kFlagNoBattery);
    EXPECT_EQ(r.drainX100, INT32_MIN);
}

TEST(NightSummary, AReadingThatArrivesLateIsTheStart)
{
    NightTracker t;
    t.open(1, 1, 1000, -1, -1, 0);
    t.addRow(1060, 1, true, true, 0, 9000, 3200, false, 0);
    const NightResult r = t.close(1000 + 3600, 8900, 3190);
    EXPECT_EQ(r.startPct, 9000);
    EXPECT_EQ(r.drainX100, 100);
}

TEST(NightSummary, ARebootClosesAtTheLastRow)
{
    NightTracker t;
    t.open(5, 1, 1000, 6000, 2000, 0);
    t.addRow(4600, 60, true, true, 0, 5900, 1990, false, 0);
    // The service restarts: its state is saved and restored, then the night is over.
    NightTracker after;
    after.restore(t.state());
    EXPECT_TRUE(after.isOpen());
    const NightResult r = after.closeAtLastRow();
    EXPECT_TRUE(r.flags & kFlagRestarted);
    EXPECT_EQ(r.endUtc, 4600u);
    EXPECT_EQ(r.endPct, 5900);
    EXPECT_EQ(r.drainX100, 100);
}

TEST(NightSummary, AModeChangedMidNightIsFlagged)
{
    NightTracker t;
    t.open(1, 1, 1000, 8000, -1, 0);
    t.changeMode(1);   // the same mode: nothing to flag
    EXPECT_EQ(t.state().flags, 0);
    t.changeMode(0);
    const NightResult r = t.close(1000 + 3600, 7990, -1);
    EXPECT_EQ(r.mode, 0);
    EXPECT_TRUE(r.flags & kFlagModeChanged);
}

TEST(NightSummary, ClosingBeforeOpeningIsHarmless)
{
    NightTracker t;
    t.addRow(1, 1, true, true, 0, 1, 1, true, 1);   // ignored
    const NightResult r = t.close(100, 5000, 100);
    EXPECT_EQ(r.rows, 0u);
}

TEST(NightSummary, Brief)
{
    NightResult r;
    r.day = 9;
    r.mode = 3;
    r.drainX100 = 42;
    r.flags = kFlagCharged;
    const NightBrief b = briefOf(r);
    EXPECT_EQ(b.day, 9);
    EXPECT_EQ(b.mode, 3);
    EXPECT_EQ(b.drainX100, 42);
    EXPECT_EQ(b.flags, kFlagCharged);
}
