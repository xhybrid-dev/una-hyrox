/**
 ******************************************************************************
 * @file    WristMode.hpp
 * @brief   Whether the probe listens for wrist raises today.
 *
 * To measure what the subscription costs, the probe alternates by local day:
 * on for an even day number, off for an odd one, with the battery gauge read
 * every minute either way. An optional mode.txt beside the probe (put there over
 * USB) pins it: "on", "off" or "auto" (the default).
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_WRIST_MODE_HPP
#define CLOCK_PROBE_WRIST_MODE_HPP

#include <cstddef>
#include <cstdint>

namespace ClockProbe
{

enum class ModeSetting : uint8_t { Auto = 0, On = 1, Off = 2 };

/// Is the wrist subscription on for the local day @p dayNumber (days since 1970-01-01)?
constexpr bool wristOn(ModeSetting setting, int32_t dayNumber)
{
    return setting == ModeSetting::On || (setting == ModeSetting::Auto && (dayNumber % 2 == 0));
}

/// Read mode.txt's text: the first word, case-insensitive, "on" or "off"; anything else is auto.
inline ModeSetting parseModeText(const char* text, size_t len)
{
    if (text == nullptr) {
        return ModeSetting::Auto;
    }
    size_t i = 0;
    while (i < len && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n')) {
        ++i;
    }
    auto lower = [](char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c; };
    auto word = [&](const char* w) {
        size_t k = 0;
        while (w[k] != '\0') {
            if (i + k >= len || lower(text[i + k]) != w[k]) {
                return false;
            }
            ++k;
        }
        const size_t end = i + k;
        return end >= len || text[end] == ' ' || text[end] == '\r' || text[end] == '\n' || text[end] == '\t' ||
               text[end] == '\0';
    };
    if (word("on")) {
        return ModeSetting::On;
    }
    if (word("off")) {
        return ModeSetting::Off;
    }
    return ModeSetting::Auto;
}

inline const char* modeName(ModeSetting s)
{
    return s == ModeSetting::On ? "on" : (s == ModeSetting::Off ? "off" : "auto");
}

} // namespace ClockProbe

#endif // CLOCK_PROBE_WRIST_MODE_HPP
