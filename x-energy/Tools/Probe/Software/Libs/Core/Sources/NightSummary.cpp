/**
 ******************************************************************************
 * @file    NightSummary.cpp
 * @brief   A night's battery drain and coverage (see the header).
 ******************************************************************************
 */

#include "NightSummary.hpp"

#include <cstdio>

namespace EnergyProbe
{

namespace
{
constexpr uint32_t kPartialMinutes = 10;

int64_t roundDiv(int64_t num, int64_t den)
{
    if (den == 0) {
        return 0;
    }
    const bool neg = (num < 0) != (den < 0);
    const int64_t a = num < 0 ? -num : num;
    const int64_t b = den < 0 ? -den : den;
    const int64_t q = (a + b / 2) / b;
    return neg ? -q : q;
}
} // namespace

int32_t drainPerHourX100(int32_t startPct, int32_t endPct, uint32_t seconds)
{
    if (startPct < 0 || endPct < 0 || seconds < 60) {
        return INT32_MIN;
    }
    // (start - end) is % x100; per hour x100 = drop * 3600 / seconds.
    return static_cast<int32_t>(roundDiv(static_cast<int64_t>(startPct - endPct) * 3600, seconds));
}

void NightTracker::open(int32_t day, uint8_t mode, uint32_t utc, int32_t pctX100, int32_t mahX10,
                        uint32_t minutesLate)
{
    mOpen = NightOpen{};
    mOpen.open = true;
    mOpen.day = day;
    mOpen.mode = mode;
    mOpen.startUtc = utc;
    mOpen.startPct = pctX100;
    mOpen.startMah = mahX10;
    mOpen.lastUtc = utc;
    mOpen.lastPct = pctX100;
    mOpen.lastMah = mahX10;
    if (minutesLate > kPartialMinutes) {
        mOpen.flags |= kFlagPartial;
    }
}

void NightTracker::addRow(uint32_t utc, uint32_t minutes, bool worn, bool heart, int8_t charging,
                          int32_t pctX100, int32_t mahX10, bool hasGauge, int32_t gaugeMaX100)
{
    if (!mOpen.open) {
        return;
    }
    ++mOpen.rows;
    if (worn) {
        mOpen.wornMin += minutes;
    }
    if (heart) {
        mOpen.hrMin += minutes;
    }
    if (charging == 1) {
        mOpen.flags |= kFlagCharged;
    }
    if (hasGauge) {
        mOpen.gaugeSum += gaugeMaX100;
        ++mOpen.gaugeN;
    }
    // A battery reading that arrives only after the night opened is its start.
    if (mOpen.startPct < 0 && pctX100 >= 0) {
        mOpen.startPct = pctX100;
    }
    if (mOpen.startMah < 0 && mahX10 >= 0) {
        mOpen.startMah = mahX10;
    }
    mOpen.lastUtc = utc;
    if (pctX100 >= 0) {
        mOpen.lastPct = pctX100;
    }
    if (mahX10 >= 0) {
        mOpen.lastMah = mahX10;
    }
}

NightResult NightTracker::close(uint32_t utc, int32_t pctX100, int32_t mahX10)
{
    NightResult r;
    r.day = mOpen.day;
    r.mode = mOpen.mode;
    r.flags = mOpen.flags;
    r.startUtc = mOpen.startUtc;
    r.endUtc = utc > mOpen.startUtc ? utc : mOpen.startUtc;
    r.startPct = mOpen.startPct;
    r.endPct = pctX100 >= 0 ? pctX100 : mOpen.lastPct;
    const int32_t endMah = mahX10 >= 0 ? mahX10 : mOpen.lastMah;
    const uint32_t seconds = r.endUtc - r.startUtc;
    r.hoursX100 = static_cast<int32_t>(roundDiv(static_cast<int64_t>(seconds) * 100, 3600));
    r.drainX100 = drainPerHourX100(r.startPct, r.endPct, seconds);
    if (mOpen.startMah >= 0 && endMah >= 0) {
        r.mahDropX10 = mOpen.startMah - endMah;
        if (seconds >= 60) {
            // mA x100 = (mAh x10 / 10) / (s / 3600) * 100 = mAh x10 * 36000 / s
            r.capMaX100 = static_cast<int32_t>(roundDiv(static_cast<int64_t>(r.mahDropX10) * 36000, seconds));
        }
    }
    if (mOpen.gaugeN > 0) {
        r.gaugeMaX100 = static_cast<int32_t>(roundDiv(mOpen.gaugeSum, mOpen.gaugeN));
    }
    if (r.startPct < 0 || r.endPct < 0) {
        r.flags |= kFlagNoBattery;
    }
    r.rows = mOpen.rows;
    r.wornMin = mOpen.wornMin;
    r.hrMin = mOpen.hrMin;
    mOpen = NightOpen{};
    return r;
}

NightResult NightTracker::closeAtLastRow()
{
    mOpen.flags |= kFlagRestarted;
    return close(mOpen.lastUtc, mOpen.lastPct, mOpen.lastMah);
}

const char* nightHeader()
{
    return "night,mode,start_utc,end_utc,hours_x100,batt_start_x100,batt_end_x100,drain_pct_h_x100,"
           "mah_drop_x10,cap_ma_x100,gauge_ma_x100,rows,worn_min,hr_min,flags\n";
}

size_t formatNight(char* out, size_t cap, const NightResult& n, const char* modeCode, uint32_t yyyymmdd)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    char drain[16] = "";
    char drop[16]  = "";
    char capMa[16] = "";
    char gauge[16] = "";
    if (n.drainX100 != INT32_MIN) {
        std::snprintf(drain, sizeof(drain), "%ld", static_cast<long>(n.drainX100));
    }
    if (n.mahDropX10 != INT32_MIN) {
        std::snprintf(drop, sizeof(drop), "%ld", static_cast<long>(n.mahDropX10));
    }
    if (n.capMaX100 != INT32_MIN) {
        std::snprintf(capMa, sizeof(capMa), "%ld", static_cast<long>(n.capMaX100));
    }
    if (n.gaugeMaX100 != INT32_MIN) {
        std::snprintf(gauge, sizeof(gauge), "%ld", static_cast<long>(n.gaugeMaX100));
    }
    char startPct[12] = "";
    char endPct[12]   = "";
    if (n.startPct >= 0) {
        std::snprintf(startPct, sizeof(startPct), "%ld", static_cast<long>(n.startPct));
    }
    if (n.endPct >= 0) {
        std::snprintf(endPct, sizeof(endPct), "%ld", static_cast<long>(n.endPct));
    }
    const int len = std::snprintf(out, cap, "%lu,%s,%lu,%lu,%ld,%s,%s,%s,%s,%s,%s,%lu,%lu,%lu,%u\n",
                                  static_cast<unsigned long>(yyyymmdd), modeCode != nullptr ? modeCode : "",
                                  static_cast<unsigned long>(n.startUtc), static_cast<unsigned long>(n.endUtc),
                                  static_cast<long>(n.hoursX100), startPct, endPct, drain, drop, capMa, gauge,
                                  static_cast<unsigned long>(n.rows), static_cast<unsigned long>(n.wornMin),
                                  static_cast<unsigned long>(n.hrMin), static_cast<unsigned>(n.flags));
    if (len < 0 || static_cast<size_t>(len) >= cap) {
        out[0] = '\0';
        return 0;
    }
    return static_cast<size_t>(len);
}

} // namespace EnergyProbe
