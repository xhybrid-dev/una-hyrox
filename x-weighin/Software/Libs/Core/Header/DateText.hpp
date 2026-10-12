/**
 ******************************************************************************
 * @file    DateText.hpp
 * @brief   Dates as the screens show them: "Mon 12 Oct", "today", "3 days ago".
 *
 * Header-only (the GUI uses it), host-tested, British English, ASCII only
 * (the watch's Poppins faces hold printable ASCII).
 ******************************************************************************
 */

#ifndef XWEIGHIN_DATE_TEXT_HPP
#define XWEIGHIN_DATE_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "Calendar.hpp"

namespace XWeighIn
{

/// 0 = Monday .. 6 = Sunday. Day 0 (1970-01-01) was a Thursday.
inline uint8_t weekday(int32_t day)
{
    const int32_t w = (day + 3) % 7;
    return static_cast<uint8_t>(w < 0 ? w + 7 : w);
}

inline const char* weekdayShort(int32_t day)
{
    static const char* const kNames[7] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    return kNames[weekday(day)];
}

inline const char* monthShort(uint8_t month)
{
    static const char* const kNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    return (month >= 1 && month <= 12) ? kNames[month - 1] : "---";
}

/// "Mon 12 Oct" (no leading zero: "Mon 5 Oct"). Returns snprintf's length.
inline size_t formatShortDate(int32_t day, char* out, size_t outSize)
{
    if (out == nullptr || outSize == 0) {
        return 0;
    }
    const Date d = civilFromDays(day);
    const int  n = std::snprintf(out, outSize, "%s %u %s", weekdayShort(day),
                                 static_cast<unsigned>(d.day), monthShort(d.month));
    return n > 0 ? static_cast<size_t>(n) : 0;
}

/// "12 Oct": for date spans where the weekday would crowd the line.
inline size_t formatDayMonth(int32_t day, char* out, size_t outSize)
{
    if (out == nullptr || outSize == 0) {
        return 0;
    }
    const Date d = civilFromDays(day);
    const int  n = std::snprintf(out, outSize, "%u %s", static_cast<unsigned>(d.day), monthShort(d.month));
    return n > 0 ? static_cast<size_t>(n) : 0;
}

/**
 * @brief   How long ago @p day was, seen from @p today: "today",
 *          "yesterday", "3 days ago"; a day in the future (the clock moved
 *          back since) is given as its date.
 */
inline size_t agoText(int32_t today, int32_t day, char* out, size_t outSize)
{
    if (out == nullptr || outSize == 0) {
        return 0;
    }
    const int32_t ago = today - day;
    int n = 0;
    if (ago == 0) {
        n = std::snprintf(out, outSize, "today");
    } else if (ago == 1) {
        n = std::snprintf(out, outSize, "yesterday");
    } else if (ago > 1) {
        n = std::snprintf(out, outSize, "%d days ago", static_cast<int>(ago));
    } else {
        return formatShortDate(day, out, outSize);
    }
    return n > 0 ? static_cast<size_t>(n) : 0;
}

}  // namespace XWeighIn

#endif  // XWEIGHIN_DATE_TEXT_HPP
