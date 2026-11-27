/**
 ******************************************************************************
 * @file    WeekView.hpp
 * @brief   This week's seven days from the activity apps' files: how many
 *          sessions, minutes and training load each day, and its grade for the
 *          ring. Monday is day 0, as Streak's weeks start by default.
 *
 * A day is the local day of a session's start (ActivityLink's localStart, the
 * time-zone offset already applied). Sessions outside this week are ignored.
 * Pure C++, no file I/O.
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_WEEK_VIEW_HPP
#define CLOCK_PROBE_WEEK_VIEW_HPP

#include <cstdint>

#include "ActivityLevel.hpp"
#include "ProbeView.hpp"
#include "Scan.hpp"

namespace ClockProbe
{

struct DayStat {
    uint8_t  sessions = 0;
    uint16_t minutes  = 0;
    uint16_t load     = 0;
    uint8_t  apps     = 0;   ///< bit i: an entry from kKnown[i]'s file (the activity apps')
    Level    level    = Level::None;
};

struct WeekView {
    DayStat day[kRingDays];
    int8_t  today  = -1;   ///< 0 to 6; -1 when no clock
    int32_t monday = 0;    ///< day number (days since 1970-01-01, local) of this week's Monday
};

/// 0 = Monday ... 6 = Sunday for a day number (1 January 1970 was a Thursday).
constexpr int32_t weekdayOf(int32_t dayNumber)
{
    const int32_t w = (dayNumber + 3) % 7;
    return w < 0 ? w + 7 : w;
}

/// The week @p todayDay (days since 1970-01-01, local) falls in; a negative @p todayDay means the
/// clock is unset and the week is empty.
WeekView makeWeek(const ScanData& scan, int32_t todayDay);

} // namespace ClockProbe

#endif // CLOCK_PROBE_WEEK_VIEW_HPP
