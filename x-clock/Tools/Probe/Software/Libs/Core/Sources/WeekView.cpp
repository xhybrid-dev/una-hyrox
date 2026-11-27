/**
 ******************************************************************************
 * @file    WeekView.cpp
 * @brief   See the header.
 ******************************************************************************
 */

#include "WeekView.hpp"

namespace ClockProbe
{

namespace
{
uint16_t sat16(uint32_t v)
{
    return v > 0xFFFFu ? 0xFFFFu : static_cast<uint16_t>(v);
}
} // namespace

WeekView makeWeek(const ScanData& scan, int32_t todayDay)
{
    WeekView w;
    if (todayDay < 0) {
        return w;
    }
    w.monday = todayDay - weekdayOf(todayDay);
    w.today  = static_cast<int8_t>(weekdayOf(todayDay));

    for (uint8_t i = 0; i < kLinkFiles; ++i) {
        if (!scan.actOk[i]) {
            continue;
        }
        const HybridXLink::ActivityFile& f = scan.act[i];
        for (uint8_t e = 0; e < f.count; ++e) {
            const int32_t day = static_cast<int32_t>(f.entries[e].localStart() / 86400u);
            const int32_t idx = day - w.monday;
            if (idx < 0 || idx >= kRingDays) {
                continue;
            }
            DayStat& d = w.day[idx];
            if (d.sessions < 255) {
                ++d.sessions;
            }
            d.minutes = sat16(static_cast<uint32_t>(d.minutes) + (f.entries[e].durationSec + 30u) / 60u);
            d.load    = sat16(static_cast<uint32_t>(d.load) + f.entries[e].load);
            d.apps    = static_cast<uint8_t>(d.apps | (1u << i));
        }
    }
    for (DayStat& d : w.day) {
        d.level = levelFor(d.sessions, d.load, d.minutes);
    }
    return w;
}

} // namespace ClockProbe
