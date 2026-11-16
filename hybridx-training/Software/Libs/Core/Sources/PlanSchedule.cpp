/**
 ******************************************************************************
 * @file    PlanSchedule.cpp
 * @brief   Which session falls on a day. See PlanSchedule.hpp.
 ******************************************************************************
 */

#include "PlanSchedule.hpp"

namespace XTraining {
namespace PlanSchedule {

const Week& weekFor(const PlanDef& plan, const Calendar& cal, uint8_t runWeek, bool fiveKRecent)
{
    const uint8_t pw = PlanCalendar::planWeekOf(cal, runWeek);
    const uint8_t last = static_cast<uint8_t>(plan.weekCount - 1);
    const Week& written = plan.weeks[pw <= last ? pw : last];
    if (runWeek == 0 && pw != last) {
        if (!fiveKRecent && plan.trialWeek != nullptr) {
            return *plan.trialWeek;
        }
        if (fiveKRecent && plan.recentWeek != nullptr && pw == 0) {
            return *plan.recentWeek;
        }
    }
    return written;
}

Layout layoutFor(const PlanSetup& setup, const Calendar& cal, uint8_t runWeek)
{
    DayMask days = setup.days;
    uint8_t longDay = setup.longDay;
    bool allowSeven = false;
    if (setup.raceDate != 0 && runWeek + 1 == cal.weeks) {
        const DayPlace race = PlanCalendar::locate(cal, setup.raceDate);
        if (race.where == DayPlace::Where::In && race.runWeek == runWeek) {
            longDay = race.weekday;
            days = static_cast<DayMask>(days | (1u << longDay));
            allowSeven = true;
        }
    }
    return WeekLayout::place(days, longDay, allowSeven);
}

bool week(const PlanSetup& setup, const PlanDef& plan, const Calendar& cal, const Paces& paces,
          uint8_t runWeek, DaySession out[7])
{
    if (!cal.usable() || runWeek >= cal.weeks) {
        return false;
    }
    const Layout layout = layoutFor(setup, cal, runWeek);
    if (!layout.ok) {
        return false;
    }
    const Week& wk = weekFor(plan, cal, runWeek, setup.fiveKRecent);
    Session written[7];
    WeekLayout::sessions(wk, layout, written);
    const Scaling sc = PlanBuilder::scaling(plan, setup.level, paces, WeekLayout::dayCount(setup.days));
    const uint8_t keySlot = wk.key;
    for (uint8_t d = 0; d < 7; ++d) {
        DaySession& s = out[d];
        s = DaySession{};
        s.date = PlanCalendar::dateOf(cal, runWeek, d);
        s.runWeek = runWeek;
        s.planWeek = PlanCalendar::planWeekOf(cal, runWeek);
        s.weekday = d;
        s.phase = wk.phase;
        s.role = layout.role[d];
        s.written = written[d];
        s.asRun = PlanBuilder::forLevel(written[d], sc);
        switch (s.role) {
        case Role::Quality1: s.key = keySlot == kSlotQ1; break;
        case Role::Quality2: s.key = keySlot == kSlotQ2; break;
        case Role::Long:     s.key = keySlot == kSlotLong; break;
        case Role::KeyOnly:  s.key = keySlot == kSlotQ1 || keySlot == kSlotQ2; break;
        default:             s.key = false; break;
        }
    }
    return true;
}

bool day(const PlanSetup& setup, const PlanDef& plan, const Calendar& cal, const Paces& paces, Ymd date,
         DaySession& out)
{
    const DayPlace p = PlanCalendar::locate(cal, date);
    if (p.where != DayPlace::Where::In) {
        return false;
    }
    DaySession days[7];
    if (!week(setup, plan, cal, paces, p.runWeek, days)) {
        return false;
    }
    out = days[p.weekday];
    return true;
}

}  // namespace PlanSchedule
}  // namespace XTraining
