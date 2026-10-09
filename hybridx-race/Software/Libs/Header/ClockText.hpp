/**
 ******************************************************************************
 * @file    ClockText.hpp
 * @brief   Time of day as the race status face shows it.
 ******************************************************************************
 * The watch's own setting decides between a 24-hour clock ("07:05") and a
 * 12-hour one ("7:05"). The 12-hour form shows 12 for the hour after midnight
 * and after noon, and has no leading zero.
 *
 * Pure C++: no SDK, no LVGL. Host-tested.
 ******************************************************************************
 */

#ifndef CLOCK_TEXT_HPP
#define CLOCK_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace Race
{

/**
 * @param buf        Destination; 6 bytes always hold the result.
 * @param size       Size of @p buf.
 * @param hour       0 to 23.
 * @param minute     0 to 59.
 * @param twelveHour The watch's 12-hour setting.
 */
inline void clockText(char *buf, size_t size, uint8_t hour, uint8_t minute, bool twelveHour)
{
    if (buf == nullptr || size == 0u) {
        return;
    }
    if (twelveHour) {
        const unsigned h12 = (hour % 12u == 0u) ? 12u : (hour % 12u);
        snprintf(buf, size, "%u:%02u", h12, static_cast<unsigned>(minute));
    } else {
        snprintf(buf, size, "%02u:%02u", static_cast<unsigned>(hour),
                 static_cast<unsigned>(minute));
    }
}

}  // namespace Race

#endif  // CLOCK_TEXT_HPP
