// The Energy Probe's state.txt.
#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "StateFile.hpp"

using namespace EnergyProbe;

namespace
{
ProbeState sample()
{
    ProbeState s;
    s.settings.window = 2;
    s.settings.dayMode = Mode::Off;
    s.settings.firstNight = 20416;
    s.settings.overrideDay = 20418;
    s.settings.overrideMode = 1 + static_cast<uint8_t>(Mode::Motion);
    s.starts = 4;
    s.night.open = true;
    s.night.day = 20417;
    s.night.mode = static_cast<uint8_t>(Mode::Off);
    s.night.flags = kFlagRestarted;
    s.night.startUtc = 1795000000u;
    s.night.startPct = 8125;
    s.night.startMah = -1;
    s.night.rows = 33;
    s.night.wornMin = 300;
    s.night.hrMin = 0;
    s.night.gaugeSum = -5000000000LL;   // beyond 32 bits
    s.night.gaugeN = 20000;
    s.night.lastUtc = 1795019800u;
    s.night.lastPct = 7990;
    s.night.lastMah = -1;
    for (int i = 0; i < 3; ++i) {
        NightBrief b;
        b.day = 20413 + i;
        b.mode = static_cast<uint8_t>(i % kModeCount);
        b.flags = static_cast<uint8_t>(i);
        b.drainX100 = 30 + i;
        b.capMaX100 = i == 1 ? INT32_MIN : 25;
        b.gaugeMaX100 = -200 - i;
        b.hoursX100 = 1000;
        pushHistory(s, b);
    }
    return s;
}
} // namespace

TEST(StateFile, RoundTrip)
{
    const ProbeState s = sample();
    char buf[2048];
    const size_t n = writeState(buf, sizeof(buf), s);
    ASSERT_GT(n, 0u);

    ProbeState r;
    ASSERT_TRUE(readState(buf, n, r));
    EXPECT_EQ(r.settings.window, 2);
    EXPECT_EQ(r.settings.dayMode, Mode::Off);
    EXPECT_EQ(r.settings.firstNight, 20416);
    EXPECT_EQ(r.settings.overrideDay, 20418);
    EXPECT_EQ(r.settings.overrideMode, 1 + static_cast<uint8_t>(Mode::Motion));
    EXPECT_EQ(r.starts, 4u);
    EXPECT_TRUE(r.night.open);
    EXPECT_EQ(r.night.day, 20417);
    EXPECT_EQ(r.night.flags, kFlagRestarted);
    EXPECT_EQ(r.night.startPct, 8125);
    EXPECT_EQ(r.night.startMah, -1);
    EXPECT_EQ(r.night.gaugeSum, -5000000000LL);
    EXPECT_EQ(r.night.gaugeN, 20000u);
    EXPECT_EQ(r.night.lastPct, 7990);
    ASSERT_EQ(r.historyCount, 3);
    EXPECT_EQ(r.history[0].day, 20415);   // newest first
    EXPECT_EQ(r.history[1].capMaX100, INT32_MIN);
    EXPECT_EQ(r.history[2].day, 20413);
    EXPECT_EQ(r.history[2].gaugeMaX100, -200);
}

TEST(StateFile, HistoryKeepsTheNewestSeven)
{
    ProbeState s;
    for (int i = 0; i < 10; ++i) {
        NightBrief b;
        b.day = i;
        pushHistory(s, b);
    }
    EXPECT_EQ(s.historyCount, kHistory);
    EXPECT_EQ(s.history[0].day, 9);
    EXPECT_EQ(s.history[kHistory - 1].day, 3);
}

TEST(StateFile, NothingSavedWithoutTheVersion)
{
    ProbeState r;
    const char* text = "window=2\nfirst=5\n";
    EXPECT_FALSE(readState(text, std::strlen(text), r));
    EXPECT_EQ(r.settings.window, kDefaultWindow);
    EXPECT_EQ(r.settings.firstNight, -1);
    const char* wrong = "v=9\nwindow=2\n";
    EXPECT_FALSE(readState(wrong, std::strlen(wrong), r));
    EXPECT_FALSE(readState(nullptr, 10, r));
}

TEST(StateFile, BadValuesKeepTheirDefaults)
{
    const char* text =
        "v=1\n"
        "window=9\n"            // out of range
        "day_mode=3\n"          // only 0 or 1
        "first=abc\n"           // not a number
        "ovr_mode=40\n"
        "garbage line\n"
        "unknown=5\n"
        "h0=1,2,3\n"            // too short
        "h5=1,2,0,4,5,6,7\n"    // out of order
        "starts=99999999999\n"  // too big
        "=7\n"
        "n_open=1\n"
        "n_day=12\n";
    ProbeState r;
    ASSERT_TRUE(readState(text, std::strlen(text), r));
    EXPECT_EQ(r.settings.window, kDefaultWindow);
    EXPECT_EQ(r.settings.dayMode, Mode::Full);
    EXPECT_EQ(r.settings.firstNight, -1);
    EXPECT_EQ(r.settings.overrideMode, kOverrideAuto);
    EXPECT_EQ(r.historyCount, 0);
    EXPECT_EQ(r.starts, 0u);
    EXPECT_TRUE(r.night.open);
    EXPECT_EQ(r.night.day, 12);
}

TEST(StateFile, NoNewlineAtTheEndAndCrLf)
{
    const char* text = "v=1\r\nwindow=3\r\nfirst=-7";
    ProbeState r;
    ASSERT_TRUE(readState(text, std::strlen(text), r));
    EXPECT_EQ(r.settings.window, 3);
    EXPECT_EQ(r.settings.firstNight, -7);
}

TEST(StateFile, TooSmallABuffer)
{
    char tiny[20];
    EXPECT_EQ(writeState(tiny, sizeof(tiny), sample()), 0u);
    EXPECT_EQ(tiny[0], '\0');
}
