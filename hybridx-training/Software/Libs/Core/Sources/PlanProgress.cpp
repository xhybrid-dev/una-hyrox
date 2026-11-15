/**
 ******************************************************************************
 * @file    PlanProgress.cpp
 * @brief   Day marks and counts. See PlanProgress.hpp.
 ******************************************************************************
 */

#include "PlanProgress.hpp"

namespace XTraining {
namespace PlanProgress {

namespace {
bool counted(const LogEntry* e)
{
    return e != nullptr && (e->status == DayStatus::Done || e->status == DayStatus::PartDone);
}
}  // namespace

DayMark mark(const DaySession& day, Ymd today, const PlanState& state)
{
    const LogEntry* e = state.find(day.date);
    if (e != nullptr) {
        switch (e->status) {
        case DayStatus::Done:     return DayMark::Done;
        case DayStatus::PartDone: return DayMark::PartDone;
        case DayStatus::Skipped:  return DayMark::Skipped;
        default:                  break;
        }
    }
    if (day.asRun.kind == Kind::Rest) {
        return DayMark::Rest;
    }
    if (day.date == today) {
        return DayMark::Today;
    }
    return Date::toDayNum(day.date) > Date::toDayNum(today) ? DayMark::ToCome : DayMark::NotLogged;
}

uint8_t weekToShow(const Calendar& cal, Ymd today)
{
    const DayPlace p = PlanCalendar::locate(cal, today);
    switch (p.where) {
    case DayPlace::Where::In:    return p.runWeek;
    case DayPlace::Where::After: return cal.weeks > 0 ? static_cast<uint8_t>(cal.weeks - 1) : 0;
    default:                     return 0;
    }
}

Counts counts(const PlanState& state, const PlanDef& plan, const Paces& paces, Ymd today)
{
    Counts c;
    if (!state.active()) {
        return c;
    }
    const uint8_t shown = weekToShow(state.calendar, today);
    const DayNum todayNum = Date::toDayNum(today);
    for (uint8_t w = 0; w < state.calendar.weeks; ++w) {
        DaySession days[7];
        if (!PlanSchedule::week(state.setup, plan, state.calendar, paces, w, days)) {
            continue;
        }
        for (const DaySession& d : days) {
            if (d.asRun.kind == Kind::Rest) {
                continue;
            }
            const bool done = counted(state.find(d.date));
            ++c.total;
            c.done = static_cast<uint16_t>(c.done + (done ? 1 : 0));
            const DayNum n = Date::toDayNum(d.date);
            if (n < todayNum || (n == todayNum && done)) {
                ++c.soFar;
            }
            if (w == shown) {
                ++c.weekSessions;
                c.weekDone = static_cast<uint8_t>(c.weekDone + (done ? 1 : 0));
            }
        }
    }
    return c;
}

}  // namespace PlanProgress
}  // namespace XTraining
