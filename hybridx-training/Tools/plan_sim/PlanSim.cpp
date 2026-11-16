/**
 * @file    PlanSim.cpp
 * @brief   The plan-memory scenarios. See PlanSim.hpp.
 */

#include "PlanSim.hpp"

#include "PaceModel.hpp"
#include "PlanRules.hpp"
#include "PlanText.hpp"
#include "TimeText.hpp"
#include "data/PlanData.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace XTraining {

namespace {

std::string fmt(const char* f, ...) __attribute__((format(printf, 1, 2)));
std::string fmt(const char* f, ...)
{
    char buf[400];
    va_list ap;
    va_start(ap, f);
    std::vsnprintf(buf, sizeof(buf), f, ap);
    va_end(ap);
    return buf;
}

const char* const kDay[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

std::string dayDate(Ymd d)
{
    char b[24];
    TimeText::dayDate(d, b, sizeof(b));
    return b;
}

std::string clock(uint32_t s) { return fmt("%u:%02u", static_cast<unsigned>(s / 60), static_cast<unsigned>(s % 60)); }

struct DayRec {
    Ymd date = 0;
    uint8_t runWeek = 0;
    uint8_t planWeek = 0;
    std::string cell;
};

}  // namespace

std::string runScenario(const SimScenario& sc)
{
    const PlanDef* def = PlanData::find(sc.plan);
    PlanState st;
    st.setup.plan = sc.plan;
    st.setup.level = sc.level;
    st.setup.days = sc.days;
    st.setup.longDay = sc.longDay;
    st.setup.fiveKSec = sc.fiveKSec;
    st.setup.fiveKDate = sc.start;
    st.setup.fiveKRecent = true;
    st.setup.raceDate = sc.raceDate;
    st.calendar = PlanCalendar::make({sc.start, sc.raceDate, def->weekCount, def->minWeeks});
    PaceResult pr = PaceModel::compute({st.setup.fiveKSec, 0});
    std::string notes;
    std::vector<DayRec> recs;

    auto dayDays = [](DayMask m) {
        std::string s;
        for (int d = 0; d < 7; ++d) {
            if ((m >> d) & 1) {
                s += std::string(s.empty() ? "" : " ") + kDay[d];
            }
        }
        return s;
    };
    std::string o = fmt("### %s\n\n%s\n\n", sc.title, sc.what);
    o += fmt("Runner: %s plan, level %u, 5K %s, %s, long run %s, from %s%s.\n\n", def->name,
             static_cast<unsigned>(sc.level), clock(sc.fiveKSec).c_str(), dayDays(sc.days).c_str(), kDay[sc.longDay],
             dayDate(sc.start).c_str(), sc.raceDate != 0 ? (", race " + dayDate(sc.raceDate)).c_str() : "");

    Ymd lastOffer = 0;
    const DayNum endN = Date::toDayNum(sc.until);
    for (DayNum n = Date::toDayNum(sc.start); n <= endN; ++n) {
        const Ymd day = Date::fromDayNum(n);
        const std::string when = dayDate(day);
        // Morning: settle, then any offer.
        const uint8_t ovBefore = st.overrideCount;
        const uint8_t weeksBefore = st.calendar.weeks;
        const PlanRules::Settled se = PlanRules::settle(st, *def, pr.paces, day);
        if (se.keyMoved > 0) {
            for (uint8_t i = ovBefore; i < st.overrideCount; ++i) {
                const Override& ov = st.overrides[i];
                DaySession s;
                if (ov.mode == OverrideMode::Session && PlanSchedule::day(st.setup, *def, st.calendar, pr.paces, ov.from, s)) {
                    notes += fmt("- %s: the missed key session (%s, %s) takes the place of %s's session.\n",
                                 when.c_str(), PlanText::name(s.asRun.kind), dayDate(ov.from).c_str(),
                                 dayDate(ov.date).c_str());
                }
            }
        }
        if (se.repeated) {
            notes += fmt("- %s: week %u %s It is repeated next week (plan week %u)%s.\n", when.c_str(),
                         static_cast<unsigned>(se.weekRepeated + 1),
                         se.nothingDone ? "had nothing done: the last cut-back week comes again." : "was less than half done.",
                         static_cast<unsigned>(PlanCalendar::planWeekOf(st.calendar, static_cast<uint8_t>(se.weekRepeated + 1)) + 1),
                         st.calendar.weeks != weeksBefore ? (fmt("; the plan grows to %u weeks", static_cast<unsigned>(st.calendar.weeks))).c_str()
                                                           : "; the week it replaced is dropped");
        }
        if (se.denied != PlanRules::Settled::Denied::None) {
            const char* why = se.denied == PlanRules::Settled::Denied::LastWeeks
                                  ? "repeating it would drop one of the last weeks before the race, which are never touched"
                              : se.denied == PlanRules::Settled::Denied::GrowthLimit
                                  ? "the plan has already grown twice, the most"
                                  : "it was the plan's last week";
            notes += fmt("- %s: week %u was less than half done. It is not repeated: %s.\n", when.c_str(),
                         static_cast<unsigned>(se.weekDenied + 1), why);
        }
        const PlanRules::Offer of = PlanRules::offer(st, *def, pr.paces, day);
        if (of.kind != PlanRules::Offer::Kind::None && of.from != lastOffer) {
            lastOffer = of.from;
            notes += fmt("- %s: offer: %s was missed (%s). Do it in place of today's session?\n", when.c_str(),
                         PlanText::name(of.session.kind), dayDate(of.from).c_str());
        }
        // The runner's moves.
        bool trial = false;
        uint32_t trialSec = 0;
        bool miss = false, part = false;
        for (const SimEvent& e : sc.events) {
            if (e.date != day) {
                continue;
            }
            switch (e.type) {
            case SimEvent::Type::Miss: miss = true; break;
            case SimEvent::Type::Part: part = true; break;
            case SimEvent::Type::Skip: {
                const auto r = PlanRules::skipToday(st, *def, pr.paces, day);
                notes += fmt("- %s: skip today's session%s.\n", when.c_str(), r == PlanRules::MoveResult::Ok ? "" : " (nothing to skip)");
            } break;
            case SimEvent::Type::DoTomorrow: {
                const auto r = PlanRules::doTomorrowToday(st, *def, pr.paces, day);
                notes += fmt("- %s: do tomorrow's session today: %s.\n", when.c_str(),
                             r == PlanRules::MoveResult::Ok ? "done" : (r == PlanRules::MoveResult::NotAllowed ? "not allowed" : "nothing to move"));
            } break;
            case SimEvent::Type::MoveTomorrow: {
                const auto r = PlanRules::moveTodayTomorrow(st, *def, pr.paces, day);
                notes += fmt("- %s: move today's session to tomorrow: %s.\n", when.c_str(),
                             r == PlanRules::MoveResult::Ok ? "done" : (r == PlanRules::MoveResult::NotAllowed ? "not allowed" : "nothing to move"));
            } break;
            case SimEvent::Type::AcceptOffer:
                PlanRules::acceptOffer(st, of, day);
                notes += fmt("- %s: accepts the offer.\n", when.c_str());
                break;
            case SimEvent::Type::DeclineOffer:
                PlanRules::declineOffer(st, of);
                notes += fmt("- %s: declines the offer.\n", when.c_str());
                break;
            case SimEvent::Type::Pause:
                PlanRules::pause(st, day);
                notes += fmt("- %s: pauses the plan.\n", when.c_str());
                break;
            case SimEvent::Type::Resume: {
                uint32_t n2 = 0;
                const auto r = PlanRules::resume(st, *def, pr.paces, day, &n2);
                const char* t = r == PlanRules::Resumed::RepeatedWeek ? "the week before is repeated"
                              : r == PlanRules::Resumed::Retest ? "easy running until a time trial (the long-run day)"
                              : "the plan carries on";
                notes += fmt("- %s: resumes after %u days: %s.\n", when.c_str(), static_cast<unsigned>(n2), t);
            } break;
            case SimEvent::Type::Trial: trial = true; trialSec = e.value; break;
            case SimEvent::Type::AnswerKeep:
                PlanRules::answerAsk(st, false, day);
                notes += fmt("- %s: keeps the paces.\n", when.c_str());
                break;
            case SimEvent::Type::AnswerUpdate:
                PlanRules::answerAsk(st, true, day);
                const PaceResult was = pr;
                pr = PaceModel::compute({st.setup.fiveKSec, 0});
                notes += fmt("- %s: updates the paces (5K %s): threshold %s /km (was %s).\n", when.c_str(),
                             clock(st.setup.fiveKSec).c_str(), clock(pr.paces.threshold).c_str(), clock(was.paces.threshold).c_str());
                break;
            }
        }
        // The day.
        DaySession s;
        DayRec rec;
        rec.date = day;
        const bool in = PlanRules::sessionOn(st, *def, pr.paces, day, s);
        const DayPlace pl = PlanCalendar::locate(st.calendar, day);
        rec.runWeek = pl.runWeek;
        rec.planWeek = pl.planWeek;
        if (!in) {
            rec.cell = "-";
        } else if (s.asRun.kind == Kind::Rest) {
            rec.cell = st.pausedFrom != 0 ? "paused" : "rest";
        } else if (const LogEntry* l = st.find(day)) {
            rec.cell = std::string(PlanText::shortName(s.asRun.kind)) + (l->status == DayStatus::Skipped ? " (skipped)" : "");
        } else if (miss) {
            rec.cell = std::string(PlanText::shortName(s.asRun.kind)) + " (missed)";
        } else {
            st.record(LogEntry{day, s.asRun.kind, part ? DayStatus::PartDone : DayStatus::Done, 1});
            rec.cell = std::string(PlanText::shortName(s.asRun.kind)) + (part ? " (part)" : "") + (s.key ? " *" : "");
            if (trial && (s.asRun.kind == Kind::TimeTrial5k || s.asRun.kind == Kind::TimeTrial10k || s.asRun.kind == Kind::Race)) {
                const PlanRules::Recal r = PlanRules::onTrial(st, s.asRun.kind, def->raceCm / 100, trialSec, day);
                const char* verdict = r.action == PlanRules::Recal::Action::Apply ? "faster: new paces from the next run"
                                    : r.action == PlanRules::Recal::Action::Ask ? (r.jump ? "a big jump: asks, \"Update your paces?\""
                                                                                            : "slower by more than 2%: asks, \"Keep your paces, or update?\"")
                                                                                 : "no change";
                notes += fmt("- %s: %s in %s = a 5K of %s (was %s): %s.\n", when.c_str(), PlanText::name(s.asRun.kind),
                             clock(trialSec).c_str(), clock(r.fiveKSec).c_str(), clock(sc.fiveKSec).c_str(), verdict);
                if (r.action == PlanRules::Recal::Action::Apply) {
                    const PaceResult was = pr;
                    pr = PaceModel::compute({st.setup.fiveKSec, 0});
                    notes += fmt("  Threshold pace %s /km (was %s), easy %s-%s /km.\n", clock(pr.paces.threshold).c_str(),
                                 clock(was.paces.threshold).c_str(), clock(pr.paces.easyFast).c_str(), clock(pr.paces.easySlow).c_str());
                }
            }
        }
        recs.push_back(rec);
    }

    o += "Events:\n\n" + (notes.empty() ? std::string("- (nothing to settle)\n") : notes) + "\n";
    o += "| Week | Mon | Tue | Wed | Thu | Fri | Sat | Sun |\n|---|---|---|---|---|---|---|---|\n";
    for (size_t i = 0; i < recs.size();) {
        const uint8_t rw = recs[i].runWeek;
        std::string cells[7];
        for (auto& c : cells) {
            c = "";
        }
        uint8_t pw = recs[i].planWeek;
        size_t j = i;
        for (; j < recs.size() && recs[j].runWeek == rw && recs[j].date != 0; ++j) {
            const uint8_t wd = static_cast<uint8_t>(Date::weekday(recs[j].date));
            cells[wd] = recs[j].cell;
            pw = recs[j].planWeek;
        }
        o += fmt("| %u (plan %u) |", static_cast<unsigned>(rw + 1), static_cast<unsigned>(pw + 1));
        for (const auto& c : cells) {
            o += " " + c + " |";
        }
        o += "\n";
        i = j;
    }
    o += "\n`*` the week's key session. A week row shows what the plan had for each day when it came.\n\n";
    return o;
}

std::string allScenarios()
{
    std::string o = "# plan_sim: the plan's memory, scenario by scenario (TP5)\n\n"
                    "Generated by `plan_sim` with the app's own code (`PlanRules`). Rules: `PLANS.md` 2.6 and 4.3.\n\n";
    int n = 1;
    for (const SimScenario& s : scenarios()) {
        o += fmt("## %d. ", n++);
        o += runScenario(s).substr(4);
    }
    return o;
}

std::vector<SimScenario> scenarios()
{
    using T = SimEvent::Type;
    std::vector<SimScenario> v;
    const Ymd start = 20270104u;   // Monday 4 January 2027
    auto at = [start](int week, int day) { return Date::addDays(start, week * 7 + day); };   // 0-based week, 0 = Monday
    auto ev = [](Ymd d, T t, uint32_t value = 0) { return SimEvent{d, t, value}; };
    auto base = [&](const char* title, const char* what, int untilWeek, int untilDay) {
        SimScenario s;
        s.title = title;
        s.what = what;
        s.start = start;
        s.until = at(untilWeek, untilDay);
        return s;
    };

    {   // 1
        SimScenario s = base("A missed easy run is dropped",
                             "Week 2's Wednesday easy run is not run. Nothing replaces it and nothing is repeated: "
                             "an easy run is not worth chasing (PLANS.md 4.3).", 1, 6);
        s.events = {ev(at(1, 2), T::Miss)};
        v.push_back(s);
    }
    {   // 2
        SimScenario s = base("A missed key session takes a later quality's place",
                             "Week 6's key session is Tuesday's intervals; Friday's cruise intervals is the week's other "
                             "quality. Tuesday is missed, so the intervals take Friday's place that week.", 5, 6);
        s.events = {ev(at(5, 1), T::Miss)};
        v.push_back(s);
    }
    {   // 3
        SimScenario s = base("A missed long run is offered the next day",
                             "Sunday's long run is missed. On Monday, a rest day, it is offered once; the runner accepts, "
                             "and Monday is the long run.", 2, 1);
        s.events = {ev(at(1, 6), T::Miss), ev(at(2, 0), T::AcceptOffer)};
        v.push_back(s);
    }
    {   // 4
        SimScenario s = base("A missed long run, offered and declined",
                             "The same, and the runner declines. It is not offered again, and the long run is dropped.", 2, 3);
        s.events = {ev(at(1, 6), T::Miss), ev(at(2, 0), T::DeclineOffer)};
        v.push_back(s);
    }
    {   // 5
        SimScenario s = base("A missed key session, with no other quality left this week",
                             "Week 7's key session is Friday's 10K-pace session, and Tuesday's intervals are already run. "
                             "Friday is missed and nothing hard is left to take its place, so on Sunday, an easier session, "
                             "it is offered in place of the long run.", 6, 6);
        s.events = {ev(at(6, 4), T::Miss), ev(at(6, 6), T::DeclineOffer)};
        v.push_back(s);
    }
    {   // 6
        SimScenario s = base("A week less than half done is repeated (no race date)",
                             "Week 3: only Tuesday's session is run (1 of 4, less than half). The next week repeats week 3, "
                             "and the plan grows a week, to 11.", 4, 6);
        s.events = {ev(at(2, 2), T::Miss), ev(at(2, 4), T::Miss), ev(at(2, 6), T::Miss)};
        v.push_back(s);
    }
    {   // 7
        SimScenario s = base("A week less than half done is repeated, with a race date",
                             "The same in week 5, with the race on Sunday 14 March: the plan cannot grow, so the repeat takes "
                             "the place of week 6, which is dropped.", 6, 6);
        s.raceDate = 20270314u;
        s.events = {ev(at(4, 1), T::Miss), ev(at(4, 2), T::Miss), ev(at(4, 4), T::Miss)};
        v.push_back(s);
    }
    {   // 8
        SimScenario s = base("A repeat never touches the last two weeks before a 10K",
                             "Week 8 is mostly missed with the race on Sunday 14 March. Repeating it would drop week 9, "
                             "one of the last two weeks, so it is not repeated.", 8, 6);
        s.raceDate = 20270314u;
        s.events = {ev(at(7, 1), T::Miss), ev(at(7, 2), T::Miss), ev(at(7, 4), T::Miss)};
        v.push_back(s);
    }
    {   // 9
        SimScenario s = base("A week with nothing done: the last cut-back week again",
                             "Nothing is run in week 5. The next week is the last cut-back week (week 4) again, and the "
                             "plan resumes at week 6; it grows a week.", 6, 6);
        s.events = {ev(at(4, 1), T::Miss), ev(at(4, 2), T::Miss), ev(at(4, 4), T::Miss), ev(at(4, 6), T::Miss)};
        v.push_back(s);
    }
    {   // 10
        SimScenario s = base("With no race date the plan grows twice at most",
                             "Weeks 2, its repeat and the repeat of that are each less than half done. The first two are "
                             "repeated, the plan growing to 12; the third is not.", 5, 6);
        for (int w = 1; w <= 3; ++w) {
            s.events.push_back(ev(at(w, 1), T::Miss));
            s.events.push_back(ev(at(w, 2), T::Miss));
            s.events.push_back(ev(at(w, 4), T::Miss));
        }
        v.push_back(s);
    }
    {   // 11
        SimScenario s = base("Do tomorrow's session today",
                             "Week 3's Tuesday has repetitions and Wednesday easy running with strides. On Tuesday the runner "
                             "does Wednesday's session today: the two swap.", 2, 3);
        s.events = {ev(at(2, 1), T::DoTomorrow)};
        v.push_back(s);
    }
    {   // 12
        SimScenario s = base("Move today's session to tomorrow",
                             "Week 3's Friday tempo run is moved to Saturday, a rest day: Friday rests and Saturday runs "
                             "the tempo, the day before the long run.", 2, 6);
        s.events = {ev(at(2, 4), T::MoveTomorrow)};
        v.push_back(s);
    }
    {   // 13
        SimScenario s = base("Skipping a key session",
                             "Week 6's Tuesday intervals are skipped. Skipping counts as missing for the key-session rule: "
                             "the intervals take Friday's place.", 5, 6);
        s.events = {ev(at(5, 1), T::Skip)};
        v.push_back(s);
    }
    {   // 14
        SimScenario s = base("A pause of 10 days repeats the week before",
                             "Paused on Tuesday 12 January (week 2), resumed on Friday 22 January (week 3), 10 days later: "
                             "nothing in the pause is missed, and the last completed week (week 1) is repeated; the plan "
                             "then goes on from week 2, two weeks later than it was.", 4, 6);
        s.events = {ev(at(1, 1), T::Pause), ev(at(2, 4), T::Resume)};
        v.push_back(s);
    }
    {   // 15
        SimScenario s = base("A pause of over 14 days: easy running until a time trial",
                             "Paused on 12 January, resumed on Monday 1 February (20 days). The plan runs easy sessions, "
                             "with a 5K time trial on the long-run day. The trial is run in 22:10, and the plan resumes "
                             "at week 2, the week it was paused in.", 6, 6);
        s.events = {ev(at(1, 1), T::Pause), ev(at(4, 0), T::Resume), ev(at(4, 6), T::Trial, 22 * 60 + 10)};
        v.push_back(s);
    }
    {   // 16
        SimScenario s = base("A faster time trial applies from the next run",
                             "Week 4's 5K time trial is run in 21:20, 3% faster than the 22:00 the paces came from. The new "
                             "paces apply from the next session.", 4, 1);
        s.events = {ev(at(3, 6), T::Trial, 21 * 60 + 20)};
        v.push_back(s);
    }
    {   // 17
        SimScenario s = base("A slower time trial asks",
                             "The trial is run in 22:50, 3.8% slower. One bad day should not drag the paces down: the app asks "
                             "\"Keep your paces, or update?\" and the runner keeps them.", 4, 1);
        s.events = {ev(at(3, 6), T::Trial, 22 * 60 + 50), ev(at(4, 1), T::AnswerKeep)};
        v.push_back(s);
    }
    {   // 18
        SimScenario s = base("A slower time trial, and the runner updates",
                             "The same trial, and the runner chooses to update: the paces follow the 22:50.", 4, 1);
        s.events = {ev(at(3, 6), T::Trial, 22 * 60 + 50), ev(at(4, 1), T::AnswerUpdate)};
        v.push_back(s);
    }
    {   // 19
        SimScenario s = base("A big jump asks",
                             "A 5K time trial of 20:30 is 6.8% faster than 22:00: more than about 6% in one step is asked "
                             "about, not applied.", 4, 1);
        s.events = {ev(at(3, 6), T::Trial, 20 * 60 + 30), ev(at(4, 1), T::AnswerUpdate)};
        v.push_back(s);
    }
    return v;
}

}  // namespace XTraining
