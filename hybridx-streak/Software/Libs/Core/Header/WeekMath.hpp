/**
 ******************************************************************************
 * @file    WeekMath.hpp
 * @brief   Calendar arithmetic for weekly periods, integer only.
 *
 * Dates are local day numbers: days since 1970-01-01 in the local calendar.
 * A session's day number is fixed when it is recorded, so a later time-zone
 * change can never move it into another week (PLAN 6.1).
 *
 * Header-only and SDK-free: used by the service, the glance and the host tests.
 ******************************************************************************
 */

#ifndef STREAK_WEEK_MATH_HPP
#define STREAK_WEEK_MATH_HPP

#include <cstdint>

namespace Streak::WeekMath
{

/// Weekday numbering follows std::tm::tm_wday: 0 = Sunday ... 6 = Saturday.
constexpr uint8_t kSunday = 0;
constexpr uint8_t kMonday = 1;

/// 1970-01-01 was a Thursday.
constexpr int32_t kEpochWeekday = 4;

/// Floor division: rounds towards minus infinity, unlike C++'s `/`.
constexpr int32_t floorDiv(int32_t a, int32_t b)
{
    const int32_t q = a / b;
    return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}

/// Non-negative remainder in [0, b).
constexpr int32_t floorMod(int32_t a, int32_t b)
{
    return a - floorDiv(a, b) * b;
}

/**
 * Days since 1970-01-01 for a proleptic Gregorian date (month 1-12, day 1-31).
 * Howard Hinnant's days_from_civil: exact, branch-light, no tables.
 */
constexpr int32_t daysFromCivil(int32_t year, uint32_t month, uint32_t day)
{
    year -= month <= 2 ? 1 : 0;
    const int32_t  era = floorDiv(year, 400);
    const uint32_t yoe = static_cast<uint32_t>(year - era * 400);                   // [0, 399]
    const uint32_t mp  = month > 2 ? month - 3u : month + 9u;                       // March = 0
    const uint32_t doy = (153u * mp + 2u) / 5u + day - 1u;                          // [0, 365]
    const uint32_t doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;                  // [0, 146096]
    return era * 146097 + static_cast<int32_t>(doe) - 719468;
}

/// Weekday of a local day number (0 = Sunday).
constexpr uint8_t weekdayOf(int32_t localDay)
{
    return static_cast<uint8_t>(floorMod(localDay + kEpochWeekday, 7));
}

/**
 * The week a day belongs to, for weeks starting on `weekStart`.
 * Consecutive weeks have consecutive indices; the index changes exactly on
 * `weekStart` days (checked by WeekMathTest for every start day).
 */
constexpr int32_t periodOf(int32_t localDay, uint8_t weekStart)
{
    return floorDiv(localDay + kEpochWeekday - static_cast<int32_t>(weekStart), 7);
}

/// First local day of a period.
constexpr int32_t periodStartDay(int32_t period, uint8_t weekStart)
{
    return period * 7 - kEpochWeekday + static_cast<int32_t>(weekStart);
}

/// Days left in the day's week, counting that day itself (1..7).
constexpr uint8_t daysLeftInPeriod(int32_t localDay, uint8_t weekStart)
{
    return static_cast<uint8_t>(periodStartDay(periodOf(localDay, weekStart), weekStart) + 7 - localDay);
}

} // namespace Streak::WeekMath

#endif // STREAK_WEEK_MATH_HPP
