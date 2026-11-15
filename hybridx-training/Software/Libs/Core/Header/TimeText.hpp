/**
 ******************************************************************************
 * @file    TimeText.hpp
 * @brief   Reading the phone's time and date strings, and writing times and
 *          paces as text. Header-only (the GUI uses it); integers only.
 *
 * The phone's fields are checked by a pattern on the phone, but the watch
 * has no pattern check of its own (NOTES.md, "What was read in the SDK"), so
 * every string is read strictly here: anything else reads as 0.
 ******************************************************************************
 */

#ifndef XTRAINING_TIME_TEXT_HPP
#define XTRAINING_TIME_TEXT_HPP

#include "Date.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace XTraining {
namespace TimeText {

namespace detail {
inline bool digit(char c) { return c >= '0' && c <= '9'; }

/// Reads 1 to @p maxDigits digits at @p s[i]; false if none or too many.
inline bool number(const char* s, size_t& i, size_t maxDigits, uint32_t& out)
{
    size_t n = 0;
    uint32_t v = 0;
    while (digit(s[i])) {
        if (++n > maxDigits) {
            return false;
        }
        v = v * 10u + static_cast<uint32_t>(s[i] - '0');
        ++i;
    }
    out = v;
    return n > 0;
}
}  // namespace detail

/// "m:ss" or "mm:ss" (minutes up to 59, seconds two digits under 60) as
/// seconds; 0 if it is anything else.
inline uint32_t parseMinSec(const char* s)
{
    if (s == nullptr) {
        return 0;
    }
    size_t i = 0;
    uint32_t m = 0, sec = 0;
    if (!detail::number(s, i, 2, m) || s[i] != ':') {
        return 0;
    }
    const size_t secStart = ++i;
    if (!detail::number(s, i, 2, sec) || i - secStart != 2 || s[i] != '\0') {
        return 0;
    }
    if (m > 59 || sec > 59) {
        return 0;
    }
    return m * 60u + sec;
}

/// "h:mm:ss" (hours 0 to 9, minutes and seconds two digits under 60) as
/// seconds; 0 if it is anything else.
inline uint32_t parseHms(const char* s)
{
    if (s == nullptr) {
        return 0;
    }
    size_t i = 0;
    uint32_t h = 0, m = 0, sec = 0;
    if (!detail::number(s, i, 1, h) || s[i] != ':') {
        return 0;
    }
    size_t start = ++i;
    if (!detail::number(s, i, 2, m) || i - start != 2 || s[i] != ':') {
        return 0;
    }
    start = ++i;
    if (!detail::number(s, i, 2, sec) || i - start != 2 || s[i] != '\0') {
        return 0;
    }
    if (m > 59 || sec > 59) {
        return 0;
    }
    return h * 3600u + m * 60u + sec;
}

/// "YYYY-MM-DD" as a valid date, or 0.
inline Ymd parseDate(const char* s)
{
    if (s == nullptr) {
        return 0;
    }
    for (size_t i = 0; i < 10; ++i) {
        const bool dash = i == 4 || i == 7;
        if (dash ? s[i] != '-' : !detail::digit(s[i])) {
            return 0;
        }
    }
    if (s[10] != '\0') {
        return 0;
    }
    const auto d = [&](size_t i) { return static_cast<uint32_t>(s[i] - '0'); };
    const Ymd ymd = (d(0) * 1000u + d(1) * 100u + d(2) * 10u + d(3)) * 10000u
                  + (d(5) * 10u + d(6)) * 100u + d(8) * 10u + d(9);
    return Date::valid(ymd) ? ymd : 0;
}

/// "4:18" for a pace or a duration under an hour; "1:41:07" from an hour.
/// Returns the length written.
inline int duration(uint32_t seconds, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    const unsigned h = static_cast<unsigned>(seconds / 3600u);
    const unsigned m = static_cast<unsigned>((seconds / 60u) % 60u);
    const unsigned s = static_cast<unsigned>(seconds % 60u);
    const int n = h > 0 ? std::snprintf(buf, cap, "%u:%02u:%02u", h, m, s)
                        : std::snprintf(buf, cap, "%u:%02u", m, s);
    return n < 0 ? 0 : n;
}

/// A date as "YYYY-MM-DD", for the values file.
inline int date(Ymd d, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    const int n = std::snprintf(buf, cap, "%04u-%02u-%02u", static_cast<unsigned>(Date::year(d)),
                                static_cast<unsigned>(Date::month(d)),
                                static_cast<unsigned>(Date::day(d)));
    return n < 0 ? 0 : n;
}

}  // namespace TimeText
}  // namespace XTraining

#endif  // XTRAINING_TIME_TEXT_HPP
