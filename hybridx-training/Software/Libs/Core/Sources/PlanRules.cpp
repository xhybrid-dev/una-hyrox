/**
 ******************************************************************************
 * @file    PlanRules.cpp
 * @brief   The plan's memory and recalibration. See PlanRules.hpp.
 ******************************************************************************
 */

#include "PlanRules.hpp"

namespace XTraining {
namespace PlanRules {

namespace {

bool counted(const LogEntry* e)
{
    return e != nullptr && (e->status == DayStatus::Done || e->status == DayStatus::PartDone);
}

Session easySession() { return Session{Kind::Easy, 0, WeekLayout::kExtraEasyMinutes, 0, 0}; }

bool plain(const PlanState& st, const PlanDef& def, const Paces& paces, Ymd date, DaySession& out)
{
    return PlanSchedule::day(st.setup, def, st.calendar, paces, date, out);
}

/// The memory applied to @p base (the plan's own session on @p date).
void applyMemory(const PlanState& st, const PlanDef& def, const Paces& paces, Ymd date, const DaySession& base,
                 DaySession& out)
{
    out = base;
    const Scaling sc = PlanBuilder::scaling(def, st.setup.level, paces, WeekLayout::dayCount(st.setup.days));
    auto set = [&](const Session& w, bool key) {
        out.written = w;
        out.asRun = PlanBuilder::forLevel(w, sc);
        out.key = key;
    };
    // A paused plan rests from the day it was paused, except a day already run: pausing after today's
    // session must not take the session away (its week count, the glance's kind).
    if (st.pausedFrom != 0 && Date::toDayNum(date) >= Date::toDayNum(st.pausedFrom) && !counted(st.find(date))) {
        set(Session{}, false);
        return;
    }
    if (const Override* o = st.findOverride(date)) {
        switch (o->mode) {
        case OverrideMode::Session: {
            DaySession src;
            if (plain(st, def, paces, o->from, src)) {
                out.written = src.written;
                out.asRun = src.asRun;
                out.key = src.key;
            }
        } break;
        case OverrideMode::Easy: set(easySession(), false); break;
        case OverrideMode::Rest: set(Session{}, false); break;
        }
        return;
    }
    if (st.retestFrom != 0 && Date::toDayNum(date) >= Date::toDayNum(st.retestFrom) && base.role != Role::Rest
        && base.asRun.kind != Kind::Rest) {
        if (base.role == Role::Long) {
            set(Session{Kind::TimeTrial5k, 0, 0, 0, 0}, true);
        } else {
            set(easySession(), false);
        }
    }
}

bool eff(const PlanState& st, const PlanDef& def, const Paces& paces, Ymd date, DaySession& out)
{
    return sessionOn(st, def, paces, date, out);
}

/// The nearest later quality day of the run week of @p d, from @p firstFree on, free to take it.
bool keyReplace(PlanState& st, const PlanDef& def, const Paces& paces, Ymd d, DayNum firstFree)
{
    const DayPlace p = PlanCalendar::locate(st.calendar, d);
    if (p.where != DayPlace::Where::In) {
        return false;
    }
    for (uint8_t wd = static_cast<uint8_t>(p.weekday + 1); wd < 7; ++wd) {
        const Ymd d2 = Date::addDays(d, wd - p.weekday);
        if (Date::toDayNum(d2) < firstFree) {
            continue;
        }
        DaySession q;
        if (!eff(st, def, paces, d2, q) || st.find(d2) != nullptr || st.findOverride(d2) != nullptr) {
            continue;
        }
        const bool quality = q.role == Role::Quality1 || q.role == Role::Quality2 || q.role == Role::KeyOnly;
        if (quality && isHard(q.asRun.kind)) {
            st.setOverride(Override{d2, d, OverrideMode::Session});
            return true;
        }
    }
    return false;
}

/// The latest cut-back week at or before plan week @p upTo; @p upTo itself if there is none.
uint8_t lastCutBack(const PlanDef& def, uint8_t upTo)
{
    for (int w = upTo; w >= 0; --w) {
        if (w < def.weekCount && def.weeks[w].phase == Phase::CutBack) {
            return static_cast<uint8_t>(w);
        }
    }
    return upTo;
}

/// Repeat run week @p rw in the week after it (or its cut-back, if @p nothingDone). False when
/// the rules do not allow it, and then @p why says which.
bool repeatWeek(PlanState& st, const PlanDef& def, uint8_t rw, bool nothingDone,
                Settled::Denied* why = nullptr)
{
    Calendar& cal = st.calendar;
    const uint8_t next = static_cast<uint8_t>(rw + 1);
    Settled::Denied none = Settled::Denied::None;
    Settled::Denied& deny = why != nullptr ? *why : none;
    if (next >= cal.weeks) {
        deny = Settled::Denied::PlanEnd;
        return false;
    }
    const bool race = st.setup.raceDate != 0;
    const int pn = PlanCalendar::planWeekOf(cal, next);       // the plan week next would have run
    const int pn1 = PlanCalendar::planWeekOf(cal, static_cast<uint8_t>(next + 1));
    if (pn >= def.weekCount) {
        deny = Settled::Denied::PlanEnd;
        return false;
    }
    if (race) {
        if (pn >= static_cast<int>(def.weekCount) - protectedWeeks(def.id)) {
            deny = Settled::Denied::LastWeeks;   // never the last weeks before the race
            return false;
        }
    } else {
        const int growth = static_cast<int>(cal.weeks) - (static_cast<int>(def.weekCount) - cal.firstWeek);
        if (growth >= kMaxGrowth) {
            deny = Settled::Denied::GrowthLimit;
            return false;
        }
    }
    const uint8_t p = PlanCalendar::planWeekOf(cal, rw);
    const int x = nothingDone ? lastCutBack(def, p > def.weekCount - 1 ? static_cast<uint8_t>(def.weekCount - 1) : p)
                              : (p > def.weekCount - 1 ? def.weekCount - 1 : p);
    const int d1 = x - pn;
    const int d2 = race ? -d1 : (pn - pn1 - d1);
    const uint8_t need = static_cast<uint8_t>((d1 != 0 ? 1 : 0) + (d2 != 0 ? 1 : 0));
    if (need == 0 || cal.adjCount + need > kMaxAdjust || d1 < -8 || d1 > 7 || d2 < -8 || d2 > 7) {
        return false;
    }
    if (d1 != 0) {
        cal.adj[cal.adjCount++] = WeekAdjust{next, static_cast<int8_t>(d1)};
    }
    if (d2 != 0) {
        cal.adj[cal.adjCount++] = WeekAdjust{static_cast<uint8_t>(next + 1), static_cast<int8_t>(d2)};
    }
    if (!race) {
        ++cal.weeks;
    }
    return true;
}

/// Run week @p rw (and the weeks after it) to run plan week @p want instead of the one the
/// calendar gives: the plan shifts later, the end with it. For a plan with no race date.
bool shiftTo(PlanState& st, uint8_t rw, int want)
{
    Calendar& cal = st.calendar;
    const int p = PlanCalendar::planWeekOf(cal, rw);
    if (want < 0) {
        want = 0;
    }
    if (want < p - 8) {
        want = p - 8;   // one adjustment holds at most 8 weeks
    }
    if (want >= p || cal.adjCount >= kMaxAdjust || rw >= cal.weeks) {
        return false;
    }
    int weeks = static_cast<int>(cal.weeks) + (p - want);
    if (weeks > 20) {
        weeks = 20;
    }
    cal.adj[cal.adjCount++] = WeekAdjust{rw, static_cast<int8_t>(want - p)};
    cal.weeks = static_cast<uint8_t>(weeks);
    return true;
}

}  // namespace

uint8_t protectedWeeks(PlanId plan)
{
    switch (plan) {
    case PlanId::TenK: return 2;
    case PlanId::Half: return 3;
    default:           return 0;
    }
}

bool sessionOn(const PlanState& st, const PlanDef& def, const Paces& paces, Ymd date, DaySession& out)
{
    DaySession base;
    if (!st.active() || !plain(st, def, paces, date, base)) {
        return false;
    }
    applyMemory(st, def, paces, date, base, out);
    return true;
}

bool week(const PlanState& st, const PlanDef& def, const Paces& paces, uint8_t runWeek, DaySession out[7])
{
    if (!st.active() || !PlanSchedule::week(st.setup, def, st.calendar, paces, runWeek, out)) {
        return false;
    }
    for (uint8_t d = 0; d < 7; ++d) {
        const DaySession base = out[d];
        applyMemory(st, def, paces, base.date, base, out[d]);
    }
    return true;
}

Settled settle(PlanState& st, const PlanDef& def, const Paces& paces, Ymd today)
{
    Settled r;
    if (!st.active() || !Date::valid(today)) {
        return r;
    }
    DayNum limit = Date::toDayNum(today);
    if (st.pausedFrom != 0 && Date::toDayNum(st.pausedFrom) < limit) {
        limit = Date::toDayNum(st.pausedFrom);
    }
    const DayNum start = Date::toDayNum(st.calendar.start);
    DayNum n = st.settledTo != 0 ? Date::toDayNum(st.settledTo) : start;
    if (n < start) {
        n = start;
    }
    for (; n < limit; ++n) {
        const Ymd d = Date::fromDayNum(n);
        const DayPlace p = PlanCalendar::locate(st.calendar, d);
        if (p.where != DayPlace::Where::In) {
            continue;
        }
        DaySession s;
        if (eff(st, def, paces, d, s) && s.asRun.kind != Kind::Rest && st.find(d) == nullptr) {
            st.record(LogEntry{d, s.asRun.kind, DayStatus::Missed, 0});
            ++r.missed;
            if (s.key && s.role != Role::Long && keyReplace(st, def, paces, d, limit)) {
                ++r.keyMoved;
            }
        }
        if (p.weekday == 6 && st.retestFrom == 0) {
            DaySession days[7];
            if (week(st, def, paces, p.runWeek, days)) {
                uint8_t planned = 0, done = 0;
                for (const DaySession& ds : days) {
                    if (ds.asRun.kind != Kind::Rest) {
                        ++planned;
                        done = static_cast<uint8_t>(done + (counted(st.find(ds.date)) ? 1 : 0));
                    }
                }
                if (planned > 0 && done * 2 < planned) {
                    Settled::Denied why = Settled::Denied::None;
                    if (repeatWeek(st, def, p.runWeek, done == 0, &why)) {
                        r.repeated = true;
                        r.nothingDone = done == 0;
                        r.weekRepeated = p.runWeek;
                    } else {
                        r.denied = why;
                        r.weekDenied = p.runWeek;
                    }
                }
            }
        }
    }
    st.settledTo = Date::fromDayNum(n);   // n is at least where it began, and at least the limit
    st.pruneOverrides(Date::addDays(today, -7));
    return r;
}

bool easeNextWeek(PlanState& st, const PlanDef& def, Ymd today, Settled::Denied* why)
{
    Settled::Denied none = Settled::Denied::None;
    Settled::Denied& deny = why != nullptr ? *why : none;
    deny = Settled::Denied::None;
    if (!st.active() || !Date::valid(today)) {
        deny = Settled::Denied::NoRoom;
        return false;
    }
    const DayPlace p = PlanCalendar::locate(st.calendar, today);
    if (p.where != DayPlace::Where::In) {
        deny = Settled::Denied::NoRoom;
        return false;
    }
    // Once is enough: if the next week already runs this plan week (or an earlier one), holding the plan
    // again would repeat it a second time (repeatWeek is not idempotent).
    if (p.runWeek + 1 < st.calendar.weeks
        && PlanCalendar::planWeekOf(st.calendar, static_cast<uint8_t>(p.runWeek + 1))
               <= PlanCalendar::planWeekOf(st.calendar, p.runWeek)) {
        deny = Settled::Denied::NoRoom;
        return false;
    }
    if (repeatWeek(st, def, p.runWeek, false, &deny)) {
        return true;
    }
    if (deny == Settled::Denied::None) {
        deny = Settled::Denied::NoRoom;   // the next week already runs this plan week, or no adjustment is left
    }
    return false;
}

Offer offer(const PlanState& st, const PlanDef& def, const Paces& paces, Ymd today)
{
    Offer o;
    if (!st.active() || !Date::valid(today) || st.pausedFrom != 0) {
        return o;
    }
    DaySession t;
    if (!eff(st, def, paces, today, t) || st.find(today) != nullptr || st.findOverride(today) != nullptr
        || isHard(t.asRun.kind)) {
        return o;
    }
    const Ymd y = Date::addDays(today, -1);
    DaySession ys;
    const LogEntry* ly = st.find(y);
    if (eff(st, def, paces, y, ys) && ly != nullptr && ly->status == DayStatus::Missed && st.offerDeclined != y
        && ys.role == Role::Long && (ys.asRun.kind == Kind::Long || ys.asRun.kind == Kind::LongFinish)) {
        o.kind = Offer::Kind::LongRun;
        o.from = y;
        o.session = ys.written;
        return o;
    }
    if (t.asRun.kind == Kind::Rest) {
        return o;   // a missed key session is offered in place of a session, not a rest day
    }
    const DayPlace p = PlanCalendar::locate(st.calendar, today);
    if (p.where != DayPlace::Where::In) {
        return o;
    }
    for (int back = 1; back <= p.weekday; ++back) {
        const Ymd d = Date::addDays(today, -back);
        const LogEntry* l = st.find(d);
        DaySession s;
        if (l == nullptr || l->status != DayStatus::Missed || st.offerDeclined == d || !eff(st, def, paces, d, s)
            || !s.key || s.role == Role::Long) {
            continue;
        }
        bool moved = false;
        for (uint8_t i = 0; i < st.overrideCount; ++i) {
            moved = moved || (st.overrides[i].mode == OverrideMode::Session && st.overrides[i].from == d);
        }
        if (!moved) {
            o.kind = Offer::Kind::KeySession;
            o.from = d;
            o.session = s.written;
            return o;
        }
    }
    return o;
}

void acceptOffer(PlanState& st, const Offer& o, Ymd today)
{
    if (o.kind != Offer::Kind::None) {
        st.setOverride(Override{today, o.from, OverrideMode::Session});
    }
}

void declineOffer(PlanState& st, const Offer& o)
{
    if (o.kind != Offer::Kind::None) {
        st.offerDeclined = o.from;
    }
}

MoveResult doTomorrowToday(PlanState& st, const PlanDef& def, const Paces& paces, Ymd today)
{
    const Ymd tm = Date::addDays(today, 1);
    DaySession t, m;
    if (!eff(st, def, paces, today, t) || !eff(st, def, paces, tm, m) || m.asRun.kind == Kind::Rest
        || st.find(today) != nullptr || st.find(tm) != nullptr) {
        return MoveResult::NothingToMove;
    }
    if (st.findOverride(today) != nullptr || st.findOverride(tm) != nullptr || st.pausedFrom != 0) {
        return MoveResult::NotAllowed;
    }
    Override ovT{today, tm, OverrideMode::Session};
    Override ovM = t.asRun.kind == Kind::Rest ? Override{tm, 0, OverrideMode::Rest}
                                              : Override{tm, today, OverrideMode::Session};
    DaySession y, a;
    const Ymd yd = Date::addDays(today, -1);
    const bool yHard = eff(st, def, paces, yd, y) && counted(st.find(yd)) && isHard(y.asRun.kind);
    const bool aHard = eff(st, def, paces, Date::addDays(today, 2), a) && isHard(a.asRun.kind);
    bool hardT = isHard(m.asRun.kind);
    bool hardM = isHard(t.asRun.kind);
    if (hardT && yHard) {
        ovT = Override{today, 0, OverrideMode::Easy};
        hardT = false;
    }
    if (hardT && hardM) {
        ovT = Override{today, 0, OverrideMode::Easy};
        hardT = false;
    }
    if (hardM && aHard) {
        ovM = Override{tm, 0, OverrideMode::Easy};
    }
    st.setOverride(ovT);
    st.setOverride(ovM);
    return MoveResult::Ok;
}

MoveResult moveTodayTomorrow(PlanState& st, const PlanDef& def, const Paces& paces, Ymd today)
{
    const Ymd tm = Date::addDays(today, 1);
    DaySession t, m, a;
    if (!eff(st, def, paces, today, t) || !eff(st, def, paces, tm, m) || t.asRun.kind == Kind::Rest
        || st.find(today) != nullptr) {
        return MoveResult::NothingToMove;
    }
    if (st.findOverride(today) != nullptr || st.findOverride(tm) != nullptr || st.pausedFrom != 0
        || isHard(m.asRun.kind) || st.find(tm) != nullptr) {
        return MoveResult::NotAllowed;
    }
    const bool aHard = eff(st, def, paces, Date::addDays(today, 2), a) && isHard(a.asRun.kind);
    st.setOverride(Override{today, 0, OverrideMode::Rest});
    st.setOverride(isHard(t.asRun.kind) && aHard ? Override{tm, 0, OverrideMode::Easy}
                                                 : Override{tm, today, OverrideMode::Session});
    return MoveResult::Ok;
}

MoveResult skipToday(PlanState& st, const PlanDef& def, const Paces& paces, Ymd today)
{
    DaySession t;
    if (!eff(st, def, paces, today, t) || t.asRun.kind == Kind::Rest || st.find(today) != nullptr) {
        return MoveResult::NothingToMove;
    }
    st.record(LogEntry{today, t.asRun.kind, DayStatus::Skipped, 0});
    if (t.key && t.role != Role::Long) {
        keyReplace(st, def, paces, today, Date::toDayNum(today) + 1);
    }
    return MoveResult::Ok;
}

void pause(PlanState& st, Ymd today)
{
    if (st.pausedFrom == 0 && Date::valid(today)) {
        st.pausedFrom = today;
    }
}

Resumed resume(PlanState& st, const PlanDef& def, const Paces& paces, Ymd today, uint32_t* daysPaused)
{
    (void)paces;
    if (st.pausedFrom == 0 || !Date::valid(today)) {
        return Resumed::NotPaused;
    }
    const int32_t n = static_cast<int32_t>(Date::toDayNum(today)) - static_cast<int32_t>(Date::toDayNum(st.pausedFrom));
    const uint32_t days = n > 0 ? static_cast<uint32_t>(n) : 0u;
    if (daysPaused != nullptr) {
        *daysPaused = days;
    }
    const DayPlace was = PlanCalendar::locate(st.calendar, st.pausedFrom);   // the week it was paused in
    const DayPlace now = PlanCalendar::locate(st.calendar, today);
    const bool race = st.setup.raceDate != 0;
    st.pausedFrom = 0;
    st.settledTo = today;     // the paused days are not missed
    st.pruneOverrides(today);
    if (days > kRetestDays) {
        st.retestFrom = today;
        if (!race && was.where == DayPlace::Where::In) {
            st.resumeWeek = static_cast<uint8_t>(PlanCalendar::planWeekOf(st.calendar, was.runWeek) + 1);
        }
        return Resumed::Retest;
    }
    if (days >= kRepeatPauseDays && now.where == DayPlace::Where::In && was.where == DayPlace::Where::In) {
        if (!race) {
            // The last completed week is the one before the week it was paused in.
            const int last = was.runWeek > 0 ? PlanCalendar::planWeekOf(st.calendar, static_cast<uint8_t>(was.runWeek - 1))
                                             : PlanCalendar::planWeekOf(st.calendar, 0);
            if (shiftTo(st, now.runWeek, last)) {
                return Resumed::RepeatedWeek;
            }
        } else if (now.runWeek > 0 && repeatWeek(st, def, static_cast<uint8_t>(now.runWeek - 1), false)) {
            return Resumed::RepeatedWeek;
        }
    }
    return Resumed::Resumed;
}

Recal recalibrate(uint32_t currentFiveKSec, uint32_t distanceM, uint32_t timeSec)
{
    Recal r;
    if (distanceM == 0 || timeSec == 0 || currentFiveKSec == 0) {
        return r;
    }
    const float fit = PaceModel::fitness(static_cast<float>(distanceM), timeSec);
    const uint32_t eq = PaceModel::raceSeconds(fit, 5000.0f);
    if (eq < PaceConfig::kMin5kSec || eq > PaceConfig::kMax5kSec) {
        return r;
    }
    r.fiveKSec = eq;
    // Cross-multiplied, so that 6.8% is more than 6%, not 6 (integer division).
    if (eq < currentFiveKSec) {
        r.jump = (currentFiveKSec - eq) * 100u > kJumpPercent * currentFiveKSec;
        r.action = r.jump ? Recal::Action::Ask : Recal::Action::Apply;
    } else if (eq > currentFiveKSec) {
        r.action = (eq - currentFiveKSec) * 100u > kSlowerAskPercent * currentFiveKSec ? Recal::Action::Ask
                                                                                        : Recal::Action::Keep;
    }
    return r;
}

Recal onTrial(PlanState& st, Kind kind, uint32_t raceM, uint32_t timeSec, Ymd today)
{
    uint32_t dist = 0;
    switch (kind) {
    case Kind::TimeTrial5k:  dist = 5000; break;
    case Kind::TimeTrial10k: dist = 10000; break;
    case Kind::Race:         dist = raceM; break;
    default:                 return Recal{};
    }
    if (st.retestFrom != 0) {
        // A trial was run: the plan resumes, at the week it was paused in if it has no race date.
        const DayPlace now = PlanCalendar::locate(st.calendar, today);
        if (st.resumeWeek != 0 && now.where == DayPlace::Where::In) {
            shiftTo(st, static_cast<uint8_t>(now.runWeek + 1), st.resumeWeek - 1);
        }
        st.retestFrom = 0;
        st.resumeWeek = 0;
    }
    const Recal r = recalibrate(st.setup.fiveKSec, dist, timeSec);
    if (r.action == Recal::Action::Apply) {
        st.setup.fiveKSec = r.fiveKSec;
        st.setup.fiveKDate = today;
        st.setup.fiveKRecent = true;
        st.askFiveKSec = 0;
    } else if (r.action == Recal::Action::Ask) {
        st.askFiveKSec = r.fiveKSec;
    }
    return r;
}

void answerAsk(PlanState& st, bool update, Ymd today)
{
    if (st.askFiveKSec != 0 && update) {
        st.setup.fiveKSec = st.askFiveKSec;
        st.setup.fiveKDate = today;
        st.setup.fiveKRecent = true;
    }
    st.askFiveKSec = 0;
}

}  // namespace PlanRules
}  // namespace XTraining
