/**
 ******************************************************************************
 * @file    Date.hpp
 * @brief   Calendar dates for the plan: yyyymmdd numbers, as the SDK's
 *          SDK::Workout::Ymd, with day arithmetic and weekdays.
 *
 * Header-only: the GUI uses it as well as the service. Pure integer maths,
 * the days-from-civil algorithm (proleptic Gregorian), valid from 2000 to 2199,
 * which covers every watch clock that is set (RunLVGL's localDate() returns 0
 * before 2020).
 ******************************************************************************
 */

#ifndef XTRAINING_DATE_HPP
#define XTRAINING_DATE_HPP

#include <cstdint>

namespace XTraining {

/// yyyymmdd, e.g. 20261116; 0 means no date.
using Ymd = uint32_t;

/// Days since 1 January 2000 (a Saturday); the plan's own day count.
using DayNum = int32_t;

/// Monday is 0, Sunday 6, as the plan's weeks are laid out.
enum class Weekday : uint8_t { Mon = 0, Tue, Wed, Thu, Fri, Sat, Sun };

namespace Date {

constexpr int kMinYear = 2000;
constexpr int kMaxYear = 2199;

constexpr bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

constexpr int daysInMonth(int y, int m)
{
    return m == 2 ? (isLeap(y) ? 29 : 28) : (m == 4 || m == 6 || m == 9 || m == 11) ? 30 : 31;
}

constexpr int year(Ymd d) { return static_cast<int>(d / 10000u); }
constexpr int month(Ymd d) { return static_cast<int>((d / 100u) % 100u); }
constexpr int day(Ymd d) { return static_cast<int>(d % 100u); }

constexpr Ymd make(int y, int m, int dd)
{
    return static_cast<Ymd>(y) * 10000u + static_cast<Ymd>(m) * 100u + static_cast<Ymd>(dd);
}

/// True for a real day between kMinYear and kMaxYear.
constexpr bool valid(Ymd d)
{
    const int y = year(d), m = month(d), dd = day(d);
    return y >= kMinYear && y <= kMaxYear && m >= 1 && m <= 12 && dd >= 1 && dd <= daysInMonth(y, m);
}

/// Days since 2000-01-01 for a valid date (Howard Hinnant's days_from_civil).
constexpr DayNum toDayNum(Ymd d)
{
    int y = year(d);
    const int m = month(d);
    const int dd = day(d);
    y -= m <= 2 ? 1 : 0;
    const int era = y / 400;  // y >= 1999 here, never negative
    const int yoe = y - era * 400;
    const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<DayNum>(era * 146097 + doe - 730425);  // 730425: 2000-01-01
}

/// The date @p n days after 2000-01-01 (civil_from_days).
constexpr Ymd fromDayNum(DayNum n)
{
    const int z = static_cast<int>(n) + 730425;
    const int era = z / 146097;
    const int doe = z - era * 146097;
    const int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int mp = (5 * doy + 2) / 153;
    const int dd = doy - (153 * mp + 2) / 5 + 1;
    const int m = mp + (mp < 10 ? 3 : -9);
    const int y = yoe + era * 400 + (m <= 2 ? 1 : 0);
    return make(y, m, dd);
}

/// Monday 0 .. Sunday 6. 2000-01-01 was a Saturday. Not an overload of
/// weekday(Ymd): on the watch uint32_t is unsigned long, and a literal date
/// would match neither exactly.
constexpr Weekday weekdayOfDay(DayNum n)
{
    const int w = ((static_cast<int>(n) % 7) + 7 + 5) % 7;
    return static_cast<Weekday>(w);
}

constexpr Weekday weekday(Ymd d) { return weekdayOfDay(toDayNum(d)); }

/// The Monday on or before @p n.
constexpr DayNum mondayOf(DayNum n) { return n - static_cast<DayNum>(weekdayOfDay(n)); }

constexpr Ymd addDays(Ymd d, int32_t days) { return fromDayNum(toDayNum(d) + days); }

}  // namespace Date
}  // namespace XTraining

#endif  // XTRAINING_DATE_HPP
