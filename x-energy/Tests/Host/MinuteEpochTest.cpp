// One row of the Energy Probe's minute log.
#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "MinuteEpoch.hpp"

using namespace EnergyProbe;

namespace
{

std::vector<std::string> split(const std::string& row)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : row) {
        if (c == ',' || c == '\n') {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) {
        out.push_back(cur);
    }
    return out;
}

std::vector<std::string> headerNames()
{
    return split(MinuteEpoch::header());
}

/// The value of column `name` in `row`.
std::string col(const std::string& row, const char* name)
{
    const auto names = headerNames();
    const auto cells = split(row);
    for (size_t i = 0; i < names.size(); ++i) {
        if (names[i] == name) {
            return i < cells.size() ? cells[i] : "<missing>";
        }
    }
    return "<no column>";
}

std::string formatRow(const MinuteEpoch& e)
{
    RowContext ctx;
    ctx.utc = 1795000000u;
    ctx.local = "2026-11-24T22:01";
    ctx.uptimeS = 3600;
    ctx.mode = "full";
    ctx.night = true;
    char buf[1024];
    const size_t n = e.format(buf, sizeof(buf), ctx);
    return std::string(buf, n);
}

} // namespace

TEST(MinuteEpoch, HeaderAndRowHaveTheSameColumns)
{
    MinuteEpoch e;
    const std::string row = formatRow(e);
    ASSERT_FALSE(row.empty());
    EXPECT_EQ(row.back(), '\n');
    EXPECT_EQ(split(row).size(), headerNames().size());
    EXPECT_EQ(MinuteEpoch::columns(), headerNames().size());

    // A busy row as well.
    e.batteryLevel(83.25f);
    e.batteryMetrics(3.91f, -1.5f, -1.25f, 310.4f);
    for (uint32_t ms = 0; ms < 60000; ms += 100) {
        e.accel(ms, 0.0f, 0.0f, 9.80665f);
    }
    e.heart(60.0f, 2.0f, 1, 60.0f);
    e.temperature(31.2f);
    const std::string busy = formatRow(e);
    EXPECT_EQ(split(busy).size(), headerNames().size());
}

TEST(MinuteEpoch, EmptyFieldsWhenNothingArrived)
{
    MinuteEpoch e;
    const std::string row = formatRow(e);
    EXPECT_EQ(col(row, "batt_pct_x100"), "");
    EXPECT_EQ(col(row, "hr_mean_x10"), "");
    EXPECT_EQ(col(row, "ang0"), "");
    EXPECT_EQ(col(row, "acc_n"), "0");
    EXPECT_EQ(col(row, "mode"), "full");
    EXPECT_EQ(col(row, "period"), "night");
    EXPECT_EQ(col(row, "local"), "2026-11-24T22:01");
}

TEST(MinuteEpoch, ArmAngles)
{
    EXPECT_EQ(armAngleX10(0.0f, 0.0f, 9.8f), 900);     // face up
    EXPECT_EQ(armAngleX10(0.0f, 0.0f, -9.8f), -900);   // face down
    EXPECT_EQ(armAngleX10(9.8f, 0.0f, 0.0f), 0);       // on edge
    EXPECT_EQ(armAngleX10(1.0f, 0.0f, 1.0f), 450);
    EXPECT_EQ(armAngleX10(0.0f, 0.0f, 0.0f), MinuteEpoch::kNoAngle);
}

TEST(MinuteEpoch, FiveSecondBucketsAndTheirChange)
{
    MinuteEpoch e;
    // Face up for 30 s, then at 45 degrees for 30 s, at 10 Hz.
    for (uint32_t ms = 0; ms < 60000; ms += 100) {
        if (ms < 30000) {
            e.accel(ms, 0.0f, 0.0f, 9.80665f);
        } else {
            e.accel(ms, 6.934f, 0.0f, 6.934f);
        }
    }
    const std::string row = formatRow(e);
    EXPECT_EQ(col(row, "acc_n"), "600");
    EXPECT_EQ(col(row, "ang0"), "900");
    EXPECT_EQ(col(row, "ang5"), "900");
    EXPECT_EQ(col(row, "ang6"), "450");
    EXPECT_EQ(col(row, "ang11"), "450");
    EXPECT_EQ(col(row, "ang_change_x10"), "450");
    // Gravity alone: ENMO 0, magnitude 9.81 x100.
    EXPECT_EQ(col(row, "enmo_mg_x10"), "0");
    EXPECT_EQ(col(row, "acc_mag_x100"), "981");

    // The next row's change starts from this row's last angle.
    e.reset();
    for (uint32_t ms = 0; ms < 5000; ms += 100) {
        e.accel(ms, 0.0f, 0.0f, 9.80665f);
    }
    const std::string next = formatRow(e);
    EXPECT_EQ(col(next, "ang0"), "900");
    EXPECT_EQ(col(next, "ang1"), "");
    EXPECT_EQ(col(next, "ang_change_x10"), "450");
}

TEST(MinuteEpoch, LateSamplesGoInTheLastBucket)
{
    MinuteEpoch e;
    e.accel(70000, 0.0f, 0.0f, 9.80665f);   // a batch that arrived after the minute ended
    const std::string row = formatRow(e);
    EXPECT_EQ(col(row, "ang11"), "900");
}

TEST(MinuteEpoch, EnmoInMilliG)
{
    MinuteEpoch e;
    // 1.5 g straight up: ENMO 500 mg.
    for (int i = 0; i < 10; ++i) {
        e.accel(0, 0.0f, 0.0f, 1.5f * MinuteEpoch::kGravity);
    }
    EXPECT_EQ(col(formatRow(e), "enmo_mg_x10"), "5000");

    // Samples in g (an older SDK document said g): 1.5 g is still 500 mg.
    MinuteEpoch g;
    for (int i = 0; i < 10; ++i) {
        g.accel(0, 0.0f, 0.0f, 1.5f);
    }
    EXPECT_EQ(col(formatRow(g), "enmo_mg_x10"), "5000");

    // Raw units: no ENMO, the magnitude still shown.
    MinuteEpoch raw;
    raw.setAccelUnitsMs2(false);
    raw.accel(0, 0.0f, 0.0f, 4096.0f);
    const std::string r = formatRow(raw);
    EXPECT_EQ(col(r, "enmo_mg_x10"), "");
    EXPECT_EQ(col(r, "acc_mag_x100"), "409600");
}

TEST(MinuteEpoch, HeartTrustGate)
{
    MinuteEpoch e;
    e.heart(58.0f, 2.0f, 1, 58.0f);
    e.heart(62.0f, 3.0f, 1, 62.0f);
    e.heart(140.0f, 0.0f, 1, 140.0f);   // trust 0: counted, not used
    e.heart(0.0f, 0.0f, 0, 0.0f);       // off the wrist
    const std::string row = formatRow(e);
    EXPECT_EQ(col(row, "hr_n"), "4");
    EXPECT_EQ(col(row, "hr_ok"), "2");
    EXPECT_EQ(col(row, "hr_min"), "58");
    EXPECT_EQ(col(row, "hr_max"), "62");
    EXPECT_EQ(col(row, "hr_mean_x10"), "600");
    EXPECT_EQ(col(row, "hr_opt_zero"), "1");
    EXPECT_EQ(col(row, "trust_mean_x10"), "13");
    EXPECT_EQ(col(row, "hr_src"), "0");
    EXPECT_TRUE(e.worn());
    EXPECT_EQ(e.lastBpm(), 62);
    e.heartFirstTrusted(4400);
    e.heartFirstTrusted(9000);   // only the first counts
    EXPECT_EQ(col(formatRow(e), "hr_first_s"), "4");
}

TEST(MinuteEpoch, BeatsGiveRrAndRmssd)
{
    MinuteEpoch e;
    // RR 1000, 1100, 1000, 1100 ms: successive differences 100 each -> RMSSD 100 ms.
    uint64_t t = 5000000;
    e.beat(t);
    const int rr[] = {1000, 1100, 1000, 1100};
    for (int r : rr) {
        t += static_cast<uint64_t>(r) * 1000u;
        e.beat(t);
    }
    std::string row = formatRow(e);
    EXPECT_EQ(col(row, "beat_n"), "5");
    EXPECT_EQ(col(row, "rr_n"), "4");
    EXPECT_EQ(col(row, "rr_mean_ms"), "1050");
    EXPECT_EQ(col(row, "rmssd_ms_x10"), "1000");
    EXPECT_EQ(col(row, "rr_bad"), "0");

    // A 5 s gap is rejected and restarts the differences; a repeat is bad too.
    e.reset();
    t += 5000000;
    e.beat(t);
    e.beat(t);
    t += 900000;
    e.beat(t);
    row = formatRow(e);
    EXPECT_EQ(col(row, "rr_bad"), "2");
    EXPECT_EQ(col(row, "rr_n"), "1");
    EXPECT_EQ(col(row, "rmssd_ms_x10"), "");
}

TEST(MinuteEpoch, StatesCarryOverCountsDoNot)
{
    MinuteEpoch e;
    e.batteryLevel(77.5f);
    e.batteryCharging(true, false);
    e.touch(true);
    e.touch(false);
    e.dailyHeart(71.0f, 52.5f);
    e.dailySteps(4321);
    e.wrist();
    e.motionDetect(0);
    e.motionDetect(2);
    e.motionDetect(9);   // unknown: ignored
    e.activity(1, 80);
    std::string row = formatRow(e);
    EXPECT_EQ(col(row, "batt_pct_x100"), "7750");
    EXPECT_EQ(col(row, "usb"), "1");
    EXPECT_EQ(col(row, "charging"), "0");
    EXPECT_EQ(col(row, "touch_changes"), "1");
    EXPECT_EQ(col(row, "rhr_x10"), "525");
    EXPECT_EQ(col(row, "steps_day"), "4321");
    EXPECT_EQ(col(row, "wrist_n"), "1");
    EXPECT_EQ(col(row, "md_still"), "1");
    EXPECT_EQ(col(row, "md_sig"), "1");
    EXPECT_EQ(col(row, "ar_id"), "1");

    e.reset();
    row = formatRow(e);
    EXPECT_EQ(col(row, "batt_pct_x100"), "7750");
    EXPECT_EQ(col(row, "touch"), "0");
    EXPECT_EQ(col(row, "touch_changes"), "0");
    EXPECT_EQ(col(row, "steps_day"), "4321");
    EXPECT_EQ(col(row, "wrist_n"), "0");
    EXPECT_EQ(col(row, "ar_n"), "0");
    EXPECT_EQ(col(row, "ar_id"), "1");
    EXPECT_FALSE(e.worn());

    e.resetAll();
    EXPECT_EQ(col(formatRow(e), "batt_pct_x100"), "");
}

TEST(MinuteEpoch, BatteryMetricsAndBadValues)
{
    MinuteEpoch e;
    e.batteryMetrics(3.912f, -2.345f, -2.1f, 305.25f);
    std::string row = formatRow(e);
    EXPECT_EQ(col(row, "batt_mv"), "3912");
    EXPECT_EQ(col(row, "batt_ma_x100"), "-235");
    EXPECT_EQ(col(row, "batt_avg_ma_x100"), "-210");
    EXPECT_EQ(col(row, "batt_mah_x10"), "3053");
    EXPECT_TRUE(e.hasAvgCurrent());

    MinuteEpoch bad;
    bad.batteryLevel(NAN);
    bad.batteryLevel(140.0f);
    bad.batteryMetrics(NAN, 0.0f, 0.0f, 0.0f);
    bad.temperature(NAN);
    bad.heart(NAN, NAN, 1, NAN);
    row = formatRow(bad);
    EXPECT_EQ(col(row, "batt_pct_x100"), "");
    EXPECT_EQ(col(row, "batt_mv"), "");
    EXPECT_EQ(col(row, "temp_n"), "0");
    EXPECT_EQ(col(row, "hr_ok"), "0");
}

TEST(MinuteEpoch, TemperatureMean)
{
    MinuteEpoch e;
    e.temperature(30.0f);
    e.temperature(31.0f);
    const std::string row = formatRow(e);
    EXPECT_EQ(col(row, "temp_x100"), "3050");
    EXPECT_EQ(col(row, "temp_n"), "2");
}

TEST(MinuteEpoch, TooSmallABufferWritesNothing)
{
    MinuteEpoch e;
    RowContext ctx;
    char tiny[16];
    EXPECT_EQ(e.format(tiny, sizeof(tiny), ctx), 0u);
    EXPECT_EQ(tiny[0], '\0');
    EXPECT_EQ(e.format(nullptr, 100, ctx), 0u);
}
