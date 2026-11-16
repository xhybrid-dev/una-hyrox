/**
 ******************************************************************************
 * @file    TodayBuild.cpp
 * @brief   Fill training_today.json's data from the plan. See TodayBuild.hpp.
 ******************************************************************************
 */

#include "TodayBuild.hpp"

#include "PlanBuilder.hpp"
#include "PlanRules.hpp"
#include "PlanText.hpp"
#include "data/PlanData.hpp"

#include <cstdio>

namespace XTraining {
namespace TodayBuild {

void paceText(uint16_t secPerKm, bool imperial, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return;
    }
    uint32_t s = secPerKm;
    if (imperial) {
        s = (s * 1609u + 500u) / 1000u;   // as the app's pace bands convert
    }
    std::snprintf(buf, cap, "%u:%02u/%s", static_cast<unsigned>(s / 60), static_cast<unsigned>(s % 60),
                  imperial ? "mi" : "km");
}

namespace {

TodayStatus statusOf(const PlanState& st, Ymd date)
{
    const LogEntry* e = st.find(date);
    if (e == nullptr) {
        return TodayStatus::ToCome;
    }
    switch (e->status) {
    case DayStatus::Done:     return TodayStatus::Done;
    case DayStatus::PartDone: return TodayStatus::PartDone;
    case DayStatus::Missed:   return TodayStatus::Missed;
    case DayStatus::Skipped:  return TodayStatus::Skipped;
    }
    return TodayStatus::ToCome;
}

}  // namespace

void fill(const PlanState& st, const Paces* paces, bool imperial, Ymd today, SDK::Workout::Program* scratch,
          TodayData& out)
{
    out = TodayData{};
    const PlanDef* def = PlanData::find(st.setup.plan);
    if (def == nullptr || !st.active() || paces == nullptr || today == 0 || !Date::valid(today)
        || !Date::valid(st.calendar.start)) {
        return;   // plan 0: no plan to show
    }
    out.plan       = static_cast<uint8_t>(st.setup.plan);
    out.weeks      = st.calendar.weeks;
    out.pausedFrom = st.pausedFrom;
    out.start      = st.calendar.start;
    out.end        = Date::addDays(st.calendar.start, static_cast<int32_t>(st.calendar.weeks) * 7 - 1);
    const DayNum todayNum = Date::toDayNum(today);
    const DayNum firstNum = Date::mondayOf(todayNum);
    out.first = Date::fromDayNum(firstNum);
    out.last  = Date::fromDayNum(todayNum + TodayFile::kWindowDays);

    BuildOptions opt;
    opt.level          = st.setup.level;
    opt.easyByHr       = st.setup.easyByHr;
    opt.repsByDistance = st.setup.repsByDistance;

    for (DayNum n = firstNum; n <= todayNum + TodayFile::kWindowDays && out.count < TodayData::kMaxDays; ++n) {
        const Ymd date = Date::fromDayNum(n);
        if (date < out.start || date > out.end) {
            continue;   // not a day of the plan
        }
        DaySession day;
        if (!PlanRules::sessionOn(st, *def, *paces, date, day)) {
            continue;
        }
        TodayDay& row = out.day[out.count++];
        row.date    = date;
        row.kind    = day.asRun.kind;
        row.status  = statusOf(st, date);
        row.runWeek = static_cast<uint8_t>(day.runWeek + 1);
        if (day.asRun.kind != Kind::Rest) {
            char w[TodayDay::kWhatLen * 2];
            PlanText::glanceWhat(day.asRun, def->raceCm, w, sizeof(w));
            TodayFile::setText(row.what, sizeof(row.what), w);
            SessionMeta meta;
            if (scratch != nullptr && PlanBuilder::build(day.asRun, *paces, opt, def->raceCm, *scratch, meta)
                && meta.targetPace != 0) {
                char p[TodayDay::kPaceLen * 2];
                paceText(meta.targetPace, imperial, p, sizeof(p));
                TodayFile::setText(row.pace, sizeof(row.pace), p);
            }
        }
    }
}

}  // namespace TodayBuild
}  // namespace XTraining
