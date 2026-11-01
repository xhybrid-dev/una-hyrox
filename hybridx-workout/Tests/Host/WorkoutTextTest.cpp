#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "WorkoutText.hpp"

using namespace Workout;

namespace {

template <typename F>
std::string text(F f)
{
    char buf[32];
    f(buf, sizeof(buf));
    return buf;
}

} // namespace

TEST(SessionText, Clock)
{
    auto c = [](uint32_t ms) { return text([&](char *b, size_t n) { formatClock(b, n, ms); }); };
    EXPECT_EQ(c(0), "00:00");
    EXPECT_EQ(c(999), "00:00");
    EXPECT_EQ(c(61000), "01:01");
    EXPECT_EQ(c(59 * 60000 + 59000), "59:59");
    EXPECT_EQ(c(3600000), "1:00:00");
    EXPECT_EQ(c(3600000 + 125000), "1:02:05");
}

TEST(SessionText, ClockIsCappedNotWrapped)
{
    char buf[16];
    formatClock(buf, sizeof(buf), 0xFFFFFFFFu);
    EXPECT_STREQ(buf, "99:59:59");
}

TEST(SessionText, Bpm)
{
    auto b = [](uint16_t v) { return text([&](char *o, size_t n) { formatBpm(o, n, v); }); };
    EXPECT_EQ(b(0), "--");
    EXPECT_EQ(b(72), "72");
    EXPECT_EQ(b(65535), "65535");
}

TEST(SessionText, EveryWriterCutsShortAndNeverPastTheBuffer)
{
    for (size_t cap = 1; cap <= 12; ++cap) {
        for (int which = 0; which < 4; ++which) {
            char region[40];
            std::memset(region, 'G', sizeof(region));
            switch (which) {
            case 0: formatClock(region, cap, 3600000u + 125000u); break;
            case 1: formatBpm(region, cap, 65535); break;
            case 2: formatKcal(region, cap, UINT32_MAX); break;
            default: formatAutoLap(region, cap, 255); break;
            }
            for (size_t i = cap; i < sizeof(region); ++i) {
                ASSERT_EQ(region[i], 'G') << "writer " << which << " cap " << cap << " index " << i;
            }
            ASSERT_LT(std::strlen(region), cap) << "writer " << which << " cap " << cap;
        }
    }
}

TEST(SessionText, ZeroCapacityAndNullWriteNothing)
{
    char c = 'x';
    EXPECT_EQ(formatClock(&c, 0, 1000), 0u);
    EXPECT_EQ(formatBpm(nullptr, 8, 70), 0u);
    EXPECT_EQ(formatKcal(&c, 0, 100), 0u);
    EXPECT_EQ(formatZone(nullptr, 8, 3), 0u);
    EXPECT_EQ(c, 'x');
}

TEST(SessionText, HourMinuteAndPercent)
{
    char b[16];
    formatHourMinute(b, sizeof(b), 9, 5);
    EXPECT_STREQ(b, "09:05");
    formatHourMinute(b, sizeof(b), 23, 59);
    EXPECT_STREQ(b, "23:59");
    formatPercent(b, sizeof(b), 87, 255);
    EXPECT_STREQ(b, "87%");
    formatPercent(b, sizeof(b), 255, 255);
    EXPECT_STREQ(b, "--%");
    formatPercent(b, sizeof(b), 101, 255);
    EXPECT_STREQ(b, "--%");
    formatPercent(b, sizeof(b), 100, 255);
    EXPECT_STREQ(b, "100%");
}

TEST(SessionText, CaloriesAreWholeAndRounded)
{
    EXPECT_EQ(text([](char *o, size_t n) { formatKcal(o, n, 0); }), "0");
    EXPECT_EQ(text([](char *o, size_t n) { formatKcal(o, n, 1234); }), "123");
    EXPECT_EQ(text([](char *o, size_t n) { formatKcal(o, n, 1235); }), "124");
    EXPECT_EQ(text([](char *o, size_t n) { formatKcal(o, n, UINT32_MAX); }), "99999");
    EXPECT_EQ(text([](char *o, size_t n) { formatKcal(o, n, 999994); }), "99999");
}

TEST(SessionText, AutoLapChoicesGoRoundToOff)
{
    EXPECT_EQ(text([](char *o, size_t n) { formatAutoLap(o, n, 0); }), "Off");
    EXPECT_EQ(text([](char *o, size_t n) { formatAutoLap(o, n, 15); }), "15 min");
    uint8_t i = 0;
    std::string seen;
    for (int k = 0; k < kAutoLapCount; ++k) {
        i = nextAutoLap(i);
        seen += std::to_string(kAutoLapMinutes[i]) + " ";
    }
    EXPECT_EQ(seen, "1 5 10 15 20 30 0 ");
    EXPECT_EQ(autoLapIndex(20), 5);
    EXPECT_EQ(autoLapIndex(7), 0);      // not a choice: off
    EXPECT_EQ(nextAutoLap(200), 0);
}

TEST(SessionText, Zones)
{
    EXPECT_EQ(text([](char *o, size_t n) { formatZone(o, n, 0); }), "Z0");
    EXPECT_EQ(text([](char *o, size_t n) { formatZone(o, n, 4); }), "Z4");
    EXPECT_EQ(text([](char *o, size_t n) { formatZone(o, n, 9); }), "Z5");
}
