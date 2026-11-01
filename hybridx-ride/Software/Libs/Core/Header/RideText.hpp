/**
 ******************************************************************************
 * @file    RideText.hpp
 * @brief   Text for the ride screens: clocks, heart rate, zones, calories,
 *          speed, distance, climb and the auto lap. Pure C++, header-only, integer only. Every
 *          writer fills the caller's buffer, never past it, always terminates
 *          it (when cap > 0), and returns the characters written.
 ******************************************************************************
 */

#ifndef RIDE_SESSION_TEXT_HPP
#define RIDE_SESSION_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "RideModel.hpp"

namespace Ride {

namespace detail {
inline size_t copyOut(char *out, size_t cap, const char *text)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    size_t n = 0;
    while (text[n] != '\0' && n + 1 < cap) {
        out[n] = text[n];
        ++n;
    }
    out[n] = '\0';
    return n;
}
} // namespace detail

/// "mm:ss", or "h:mm:ss" from an hour on; capped at 99:59:59.
inline size_t formatClock(char *out, size_t cap, uint32_t ms)
{
    uint32_t s = ms / 1000u;
    constexpr uint32_t kMaxS = 99u * 3600u + 59u * 60u + 59u;
    if (s > kMaxS) {
        s = kMaxS;
    }
    const unsigned long h   = s / 3600u;
    const unsigned long m   = (s / 60u) % 60u;
    const unsigned long sec = s % 60u;
    char tmp[16];
    if (h > 0) {
        std::snprintf(tmp, sizeof(tmp), "%lu:%02lu:%02lu", h, m, sec);
    } else {
        std::snprintf(tmp, sizeof(tmp), "%02lu:%02lu", m, sec);
    }
    return detail::copyOut(out, cap, tmp);
}

/// Heart rate: the number, or "--" when there is none (0).
inline size_t formatBpm(char *out, size_t cap, uint16_t bpm)
{
    if (bpm == 0) {
        return detail::copyOut(out, cap, "--");
    }
    char tmp[8];
    std::snprintf(tmp, sizeof(tmp), "%u", static_cast<unsigned>(bpm));
    return detail::copyOut(out, cap, tmp);
}

/// Time of day, "09:28". Values past 23 and 59 are shown as they are, not wrapped.
inline size_t formatHourMinute(char *out, size_t cap, uint8_t hour, uint8_t minute)
{
    char tmp[12];
    std::snprintf(tmp, sizeof(tmp), "%02u:%02u", static_cast<unsigned>(hour), static_cast<unsigned>(minute));
    return detail::copyOut(out, cap, tmp);
}

/// A battery level as "87%", or "--%" when it is not known.
inline size_t formatPercent(char *out, size_t cap, uint8_t percent, uint8_t unknown)
{
    char tmp[8];
    if (percent == unknown || percent > 100u) {
        std::snprintf(tmp, sizeof(tmp), "--%%");
    } else {
        std::snprintf(tmp, sizeof(tmp), "%u%%", static_cast<unsigned>(percent));
    }
    return detail::copyOut(out, cap, tmp);
}

/// Calories from tenths of a kcal, rounded to whole kcal: "123"; capped at 99999,
/// as the clock is at 99:59:59 (a 100-hour ride at 12.5 MET and 75 kg is 93750).
inline size_t formatKcal(char *out, size_t cap, uint32_t kcalX10)
{
    char tmp[12];
    unsigned long k = kcalX10 / 10u + (kcalX10 % 10u >= 5u ? 1u : 0u);
    if (k > 99999ul) {
        k = 99999ul;
    }
    std::snprintf(tmp, sizeof(tmp), "%lu", k);
    return detail::copyOut(out, cap, tmp);
}

/// Speed from mm/s as km/h (or mph) with a tenth: "27.4"; "0.0" when still; capped at 999.9.
inline size_t formatSpeed(char *out, size_t cap, uint32_t mmPerS, bool imperial)
{
    // km/h x10 = mm/s x 36 / 1000; mph x10 = mm/s x 36000 / 1609344. Both rounded.
    const uint64_t t = imperial ? (static_cast<uint64_t>(mmPerS) * 36000u + 804672u) / 1609344u
                                : (static_cast<uint64_t>(mmPerS) * 36u + 500u) / 1000u;
    const unsigned long v = t > 9999u ? 9999ul : static_cast<unsigned long>(t);
    char tmp[12];
    std::snprintf(tmp, sizeof(tmp), "%lu.%lu", v / 10u, v % 10u);
    return detail::copyOut(out, cap, tmp);
}

/// Distance from centimetres as km (or miles) with two places: "12.34"; capped at 999.99, as the
/// clock is at 99:59:59 (the activity link holds at most 1000 km).
inline size_t formatDistance(char *out, size_t cap, uint32_t cm, bool imperial)
{
    // km x100 = cm / 1000; miles x100 = cm x 100 / 160934.4.
    const uint64_t t = imperial ? (static_cast<uint64_t>(cm) * 1000u + 804672u) / 1609344u
                                : (static_cast<uint64_t>(cm) + 500u) / 1000u;
    const unsigned long v = t > 99999u ? 99999ul : static_cast<unsigned long>(t);
    char tmp[16];
    std::snprintf(tmp, sizeof(tmp), "%lu.%02lu", v / 100u, v % 100u);
    return detail::copyOut(out, cap, tmp);
}

/// Climb from centimetres as whole metres (or feet): "120"; capped at 99999.
inline size_t formatClimb(char *out, size_t cap, uint32_t cm, bool imperial)
{
    // feet = cm / 30.48
    const uint64_t t = imperial ? (static_cast<uint64_t>(cm) * 100u + 1524u) / 3048u
                                : (static_cast<uint64_t>(cm) + 50u) / 100u;
    const unsigned long v = t > 99999u ? 99999ul : static_cast<unsigned long>(t);
    char tmp[12];
    std::snprintf(tmp, sizeof(tmp), "%lu", v);
    return detail::copyOut(out, cap, tmp);
}

/// The auto-lap choice: "Off", or "5 min".
inline size_t formatAutoLap(char *out, size_t cap, uint8_t minutes)
{
    if (minutes == 0) {
        return detail::copyOut(out, cap, "Off");
    }
    char tmp[12];
    std::snprintf(tmp, sizeof(tmp), "%u min", static_cast<unsigned>(minutes));
    return detail::copyOut(out, cap, tmp);
}

/// The index in kAutoLapMinutes after @p index, round to Off.
inline uint8_t nextAutoLap(uint8_t index)
{
    return static_cast<uint8_t>(index + 1u < kAutoLapCount ? index + 1u : 0u);
}

/// The index of @p minutes in kAutoLapMinutes, or 0 (Off) when it is not one of them.
inline uint8_t autoLapIndex(uint8_t minutes)
{
    for (uint8_t i = 0; i < kAutoLapCount; ++i) {
        if (kAutoLapMinutes[i] == minutes) {
            return i;
        }
    }
    return 0;
}

/// A zone's short name: "Z1" to "Z5", and "Z0" below zone 1.
inline size_t formatZone(char *out, size_t cap, uint8_t zone)
{
    char tmp[8];
    std::snprintf(tmp, sizeof(tmp), "Z%u", static_cast<unsigned>(zone > 5 ? 5 : zone));
    return detail::copyOut(out, cap, tmp);
}

} // namespace Ride

#endif // RIDE_SESSION_TEXT_HPP
