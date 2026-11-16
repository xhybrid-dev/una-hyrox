/**
 ******************************************************************************
 * @file    PlanCalendar.cpp
 * @brief   When a plan runs. See PlanCalendar.hpp.
 ******************************************************************************
 */

#include "PlanCalendar.hpp"

namespace XTraining {
namespace PlanCalendar {

Ymd firstStart(Ymd setupDay)
{
    if (!Date::valid(setupDay)) {
        return 0;
    }
    const DayNum n = Date::toDayNum(setupDay);
    const DayNum monday = Date::mondayOf(n);
    // Monday to Wednesday: this week. Thursday on: next Monday.
    const bool thisWeek = static_cast<uint8_t>(Date::weekdayOfDay(n)) <= static_cast<uint8_t>(Weekday::Wed);
    return Date::fromDayNum(thisWeek ? monday : monday + 7);
}

Calendar make(const CalendarInput& in)
{
    Calendar c;
    if (in.weekCount == 0 || in.minWeeks == 0 || in.minWeeks > in.weekCount) {
        return c;
    }
    const Ymd first = firstStart(in.setupDay);
    if (first == 0) {
        return c;
    }
    if (in.raceDate == 0) {
        c.status = Calendar::Status::Ok;
        c.start = first;
        c.firstWeek = 0;
        c.weeks = in.weekCount;
        return c;
    }
    if (!Date::valid(in.raceDate)) {
        return c;
    }
    if (in.raceDate < in.setupDay) {
        c.status = Calendar::Status::RaceGone;
        return c;
    }
    const DayNum raceMonday = Date::mondayOf(Date::toDayNum(in.raceDate));
    const DayNum full = raceMonday - static_cast<DayNum>(in.weekCount - 1) * 7;
    const DayNum firstN = Date::toDayNum(first);
    if (full >= firstN) {
        c.status = Calendar::Status::Ok;
        c.start = Date::fromDayNum(full);
        c.firstWeek = 0;
        c.weeks = in.weekCount;
        return c;
    }
    const int32_t dropped = (firstN - full) / 7;  // both Mondays: a whole number of weeks
    const int32_t left = static_cast<int32_t>(in.weekCount) - dropped;
    if (left < 1) {
        c.status = Calendar::Status::RaceGone;
        return c;
    }
    c.status = left >= in.minWeeks ? Calendar::Status::Truncated : Calendar::Status::TooClose;
    c.start = first;
    c.firstWeek = static_cast<uint8_t>(dropped);
    c.weeks = static_cast<uint8_t>(left);
    return c;
}

DayPlace locate(const Calendar& c, Ymd day)
{
    DayPlace p;
    if (!c.usable() || !Date::valid(day) || !Date::valid(c.start)) {
        return p;
    }
    const DayNum d = Date::toDayNum(day);
    const DayNum s = Date::toDayNum(c.start);
    p.weekday = static_cast<uint8_t>(Date::weekdayOfDay(d));
    if (d < s) {
        p.where = DayPlace::Where::Before;
        return p;
    }
    const int32_t week = (d - s) / 7;
    if (week >= c.weeks) {
        p.where = DayPlace::Where::After;
        return p;
    }
    p.where = DayPlace::Where::In;
    p.runWeek = static_cast<uint8_t>(week);
    p.planWeek = planWeekOf(c, p.runWeek);
    return p;
}

uint8_t planWeekOf(const Calendar& c, uint8_t runWeek)
{
    int32_t pw = static_cast<int32_t>(c.firstWeek) + runWeek;
    for (uint8_t i = 0; i < c.adjCount && i < kMaxAdjust; ++i) {
        if (c.adj[i].runWeek <= runWeek) {
            pw += c.adj[i].delta;
        }
    }
    return static_cast<uint8_t>(pw < 0 ? 0 : (pw > 255 ? 255 : pw));
}

Ymd dateOf(const Calendar& c, uint8_t runWeek, uint8_t weekday)
{
    if (!Date::valid(c.start) || weekday > 6) {
        return 0;
    }
    return Date::addDays(c.start, static_cast<int32_t>(runWeek) * 7 + weekday);
}

}  // namespace PlanCalendar
}  // namespace XTraining
