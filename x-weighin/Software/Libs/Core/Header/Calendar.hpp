/**
 ******************************************************************************
 * @file    Calendar.hpp
 * @brief   Calendar dates as day numbers (days since 1970-01-01), and back.
 *
 * H. Hinnant's days_from_civil and civil_from_days, proleptic Gregorian.
 * Header-only, because the GUI will format dates too and the GUI ELF links no
 * Libs sources. Pure: no time zones here. The service turns UTC into a local
 * day with localtime_r, as HybridX Streak does.
 ******************************************************************************
 */

#ifndef XWEIGHIN_CALENDAR_HPP
#define XWEIGHIN_CALENDAR_HPP

#include <cstdint>

namespace XWeighIn
{

/// A civil date.
struct Date {
    int32_t year  = 0;
    uint8_t month = 0;   ///< 1-12
    uint8_t day   = 0;   ///< 1-31
};

/// Day number (days since 1970-01-01) of a civil date. No range check: see
/// isValidDate().
inline int32_t daysFromCivil(int32_t y, uint32_t m, uint32_t d)
{
    y -= m <= 2 ? 1 : 0;
    const int32_t  era = (y >= 0 ? y : y - 399) / 400;
    const uint32_t yoe = static_cast<uint32_t>(y - era * 400);
    const uint32_t doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int32_t>(doe) - 719468;
}

/// Civil date of a day number.
inline Date civilFromDays(int32_t z)
{
    z += 719468;
    const int32_t  era = (z >= 0 ? z : z - 146096) / 146097;
    const uint32_t doe = static_cast<uint32_t>(z - era * 146097);
    const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const uint32_t mp  = (5 * doy + 2) / 153;
    Date out;
    out.day   = static_cast<uint8_t>(doy - (153 * mp + 2) / 5 + 1);
    out.month = static_cast<uint8_t>(mp < 10 ? mp + 3 : mp - 9);
    out.year  = static_cast<int32_t>(yoe) + era * 400 + (out.month <= 2 ? 1 : 0);
    return out;
}

/// The years the app accepts in its log: a corrupt date outside them is
/// rejected rather than allowed to drive a long gap-filling loop.
constexpr int32_t kFirstYear = 2000;
constexpr int32_t kLastYear  = 2099;

/// Day numbers of the first and last accepted days.
inline int32_t firstValidDay() { return daysFromCivil(kFirstYear, 1, 1); }
inline int32_t lastValidDay()  { return daysFromCivil(kLastYear, 12, 31); }

/// True for a real calendar date (30 February is not) within the accepted years.
inline bool isValidDate(int32_t y, uint32_t m, uint32_t d)
{
    if (y < kFirstYear || y > kLastYear || m < 1 || m > 12 || d < 1) {
        return false;
    }
    const Date back = civilFromDays(daysFromCivil(y, m, d));
    return back.year == y && back.month == m && back.day == d;
}

}  // namespace XWeighIn

#endif  // XWEIGHIN_CALENDAR_HPP
