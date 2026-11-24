/**
 ******************************************************************************
 * @file    CivilDate.hpp
 * @brief   Dates as day numbers (days since 1970-01-01) and back, for the
 *          night labels and the file names. Header-only (the GUI shows dates).
 *
 * Howard Hinnant's public-domain civil-calendar algorithms
 * (https://howardhinnant.github.io/date_algorithms.html), in integers only.
 ******************************************************************************
 */

#ifndef ENERGY_PROBE_CIVIL_DATE_HPP
#define ENERGY_PROBE_CIVIL_DATE_HPP

#include <cstdint>

namespace EnergyProbe
{

struct CivilDate {
    int32_t year;
    uint8_t month;   ///< 1-12
    uint8_t day;     ///< 1-31
};

inline int32_t daysFromCivil(int32_t y, uint32_t m, uint32_t d)
{
    y -= m <= 2 ? 1 : 0;
    const int32_t  era = (y >= 0 ? y : y - 399) / 400;
    const uint32_t yoe = static_cast<uint32_t>(y - era * 400);
    const uint32_t doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int32_t>(doe) - 719468;
}

inline CivilDate civilFromDays(int32_t z)
{
    z += 719468;
    const int32_t  era = (z >= 0 ? z : z - 146096) / 146097;
    const uint32_t doe = static_cast<uint32_t>(z - era * 146097);
    const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int32_t  y   = static_cast<int32_t>(yoe) + era * 400;
    const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const uint32_t mp  = (5 * doy + 2) / 153;
    const uint32_t d   = doy - (153 * mp + 2) / 5 + 1;
    const uint32_t m   = mp < 10 ? mp + 3 : mp - 9;
    return CivilDate{y + (m <= 2 ? 1 : 0), static_cast<uint8_t>(m), static_cast<uint8_t>(d)};
}

/// "Nov" for 11. British short month names.
inline const char* monthShort(uint8_t month)
{
    static const char* const kNames[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    return (month >= 1 && month <= 12) ? kNames[month - 1] : "?";
}

/// yyyymmdd as a number, for file names.
inline uint32_t yyyymmdd(int32_t dayNumber)
{
    const CivilDate c = civilFromDays(dayNumber);
    const int32_t   y = c.year < 0 ? 0 : (c.year > 9999 ? 9999 : c.year);
    return static_cast<uint32_t>(y) * 10000u + c.month * 100u + c.day;
}

} // namespace EnergyProbe

#endif // ENERGY_PROBE_CIVIL_DATE_HPP
