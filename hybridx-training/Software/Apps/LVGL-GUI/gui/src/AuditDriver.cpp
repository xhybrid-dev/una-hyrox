/**
 ******************************************************************************
 * @file    AuditDriver.cpp
 * @brief   The text-fit audit run (see the header).
 ******************************************************************************
 */

#include "gui/AuditDriver.hpp"

#if defined(SIMULATOR)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "lvgl.h"

#include "SDK/GUI/Button.hpp"

#include "gui/Audit.hpp"
#include "gui/WorkoutUi.hpp"
#include "gui/model/Model.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

// The simulator links the core (the service's), so the audit can ask it for the plans' words.
#include "PlanBuilder.hpp"
#include "PlanText.hpp"
#include "ReviewInsight.hpp"
#include "ReviewText.hpp"
#include "data/PlanData.hpp"

namespace AuditDriver
{

namespace
{
using namespace XTraining;
using Info = CustomMessage::PlanTodayInfo;
namespace Btn = SDK::GUI::Button;

constexpr uint32_t kTickMs = 250;

Model* sModel    = nullptr;
int    sStep     = 0;
int    sProblems = 0;

/// The widest "what" each kind of session gets, in Today's face, over the three plans at the
/// three levels (with three, four and five run days), their trial and recent weeks included.
char sWhat[kKindCount][Info::kText];

void copy(char* dst, size_t cap, const char* src)
{
    std::snprintf(dst, cap, "%s", src);
}

void findWidest()
{
    const lv_font_t* font = Theme::font(Theme::Font::Medium25);
    int32_t widest[kKindCount] = {};
    const Paces paces = PaceModel::compute({25 * 60, 0}).paces;
    for (const PlanDef& def : PlanData::kPlans) {
        for (uint8_t level = 1; level <= 3; ++level) {
            for (uint8_t days = 3; days <= 5; ++days) {
                const Scaling sc = PlanBuilder::scaling(def, level, paces, days);
                auto take = [&](const Week& w) {
                    for (const Session& s : w.slot) {
                        const Session run = PlanBuilder::forLevel(s, sc);
                        char text[Info::kText];
                        PlanText::what(run, def.raceCm, text, sizeof(text));
                        const int32_t px = WorkoutUi::textWidth(text, font);
                        const uint8_t k = static_cast<uint8_t>(run.kind);
                        if (k < kKindCount && px > widest[k]) {
                            widest[k] = px;
                            copy(sWhat[k], sizeof(sWhat[k]), text);
                        }
                    }
                };
                for (uint8_t w = 0; w < def.weekCount; ++w) {
                    take(def.weeks[w]);
                }
                if (def.trialWeek) {
                    take(*def.trialWeek);
                }
                if (def.recentWeek) {
                    take(*def.recentWeek);
                }
            }
        }
    }
}

Info session(Kind k, uint8_t status = Info::Session)
{
    Info t;
    t.status     = status;
    t.kind       = static_cast<uint8_t>(k);
    t.runWeek    = 23;
    t.weeks      = 24;
    t.date       = 20260930;   // a Wednesday
    t.plannedSec = 180 * 60;
    t.plannedM   = 32000;
    copy(t.name, sizeof(t.name), PlanText::name(k));
    copy(t.what, sizeof(t.what), sWhat[static_cast<uint8_t>(k)]);
    copy(t.target, sizeof(t.target), k == Kind::Easy || k == Kind::Long ? "easy 23:59-26:48 /mi" : "19:59-20:59 /mi");
    return t;
}

Info notToday(uint8_t status, const char* name = "", const char* what = "")
{
    Info t;
    t.status   = status;
    t.runWeek  = 23;
    t.weeks    = 24;
    t.date     = 20260930;
    t.nextDate = 20260930;
    t.nextKind = static_cast<uint8_t>(Kind::LongFinish);
    copy(t.nextWhat, sizeof(t.nextWhat), sWhat[static_cast<uint8_t>(Kind::LongFinish)]);
    copy(t.name, sizeof(t.name), name);
    copy(t.what, sizeof(t.what), what);
    return t;
}

CustomMessage::PlanWeekInfo worstWeek(Kind k)
{
    CustomMessage::PlanWeekInfo w;
    w.ok      = true;
    w.runWeek = 23;
    w.weeks   = 24;
    for (uint8_t d = 0; d < 7; ++d) {
        w.day[d].date = 20260928 + d;
        w.day[d].kind = static_cast<uint8_t>(k);
        w.day[d].mark = static_cast<uint8_t>(DayMark::NotLogged);
        w.day[d].key  = true;
    }
    return w;
}

CustomMessage::PlanOverviewInfo worstOverview(uint8_t status)
{
    CustomMessage::PlanOverviewInfo o;
    o.status       = status;
    o.plan         = 3;
    o.level        = 3;
    o.runDays      = 6;
    o.runWeek      = 23;
    o.weeks        = 24;
    o.weekSessions = 6;
    o.weekDone     = 6;
    o.total        = 144;
    o.soFar        = 144;
    o.done         = 144;
    o.start        = 20260930;
    o.end          = 20260930;
    o.raceDate     = 20260930;
    for (uint8_t w = 0; w < CustomMessage::PlanOverviewInfo::kMaxWeeks; ++w) {
        o.phase[w] = static_cast<uint8_t>(Phase::Specific);
    }
    copy(o.problem, sizeof(o.problem), "The race is too close or past");
    if (status == Info::NoPlan || status == Info::Problem) {
        o.plan  = 0;
        o.weeks = 0;
    }
    return o;
}

CustomMessage::PlanPacesInfo worstPaces(bool gap, bool oldHalf)
{
    CustomMessage::PlanPacesInfo p;
    p.ok         = true;
    // As wide as a 59:59 5K's paces per mile (the simulator is metric).
    p.easyFast   = 1439;
    p.easySlow   = 1608;
    p.marathon   = 1259;
    p.half       = 1259;
    p.threshold  = 1259;
    p.tenK       = 1259;   // threshold meets 10K pace: the note
    p.interval   = 1259;
    p.repetition = 1259;
    p.fiveK      = 1259;
    p.fiveKSec   = 59 * 60 + 59;
    p.fiveKDate  = 20260930;
    p.halfSec    = 3 * 3600 + 59 * 60 + 59;
    p.halfDate   = 20260930;
    p.halfUsed   = !oldHalf;
    p.gap        = gap;
    p.halfAs5kSec          = 59 * 60 + 59;
    p.predicted10kSec      = 2 * 3600 + 5 * 60;
    p.predictedHalfSec     = 4 * 3600 + 59 * 60;
    p.predictedMarathonSec = 9 * 3600 + 59 * 60 + 59;
    return p;
}

void key(uint8_t code)
{
    Screen* s = ScreenManager::instance().current();
    if (s) {
        s->onKey(code);
    }
}

void go(ScreenId id) { ScreenManager::instance().goTo(id); }

struct Step {
    const char* name;   ///< Non-null: check, and log under this name
    void (*run)();      ///< Non-null: do this
};

#define TODAY_KIND(K)                                                                         \
    { nullptr, [] { sModel->injectPlanToday(session(Kind::K)); } },                           \
    { "today: " #K, nullptr }

/// TP5: an offer for a missed session of kind K (a long run, or a key session)
Info offerOf(Kind k, uint8_t detail)
{
    Info t = notToday(Info::Offer, XTraining::PlanText::name(k), "Wed 30 Sep");
    t.kind   = static_cast<uint8_t>(k);
    t.detail = detail;
    return t;
}

Info paceAsk(uint32_t newSec, uint32_t oldSec, uint8_t detail)
{
    Info t = notToday(Info::PaceAsk, "Your 5K: 59:59", "your paces: 59:59");
    t.plannedSec = newSec;
    t.plannedM   = oldSec;
    t.detail     = detail;
    return t;
}

/// TP8: the offer to hold the plan (detail EaseTired, EaseFresh or EaseMixed).
Info easeOf(uint8_t detail)
{
    Info t = notToday(Info::Offer, "Hold the plan?", "12 sessions too slow");
    t.detail = detail;
    return t;
}

/// TP8: Form's data, @p n sessions and the one line @p line speaks of @p lineN of them.
CustomMessage::PlanFormInfo formOf(uint8_t n, XTraining::Review::Line line, uint8_t lineN)
{
    CustomMessage::PlanFormInfo f;
    f.count = n;
    f.line  = static_cast<uint8_t>(line);
    f.lineN = lineN;
    for (uint8_t i = 0; i < n && i < CustomMessage::PlanFormInfo::kMax; ++i) {
        f.row[i].date   = 20260930 + i;
        f.row[i].judged = i == 2 ? 0 : 1;
        f.row[i].inPct  = static_cast<uint8_t>(100 - i * 12);
        f.row[i].dir    = i % 2 == 0 ? 1 : 2;
        f.row[i].feel   = static_cast<uint8_t>(i % 6);
    }
    return f;
}

#define OFFER_KIND(K, D)                                                                      \
    { nullptr, [] { sModel->injectPlanToday(offerOf(Kind::K, Info::D)); } },                  \
    { "today: offer, " #K, nullptr }

const Step kSteps[] = {
    { nullptr, [] { findWidest(); sModel->injectPlanToday(session(Kind::Easy)); go(ScreenId::PlanToday); } },
    { nullptr, [] {} },
    TODAY_KIND(Easy), TODAY_KIND(EasyStrides), TODAY_KIND(Long), TODAY_KIND(LongFinish),
    TODAY_KIND(Tempo), TODAY_KIND(Cruise), TODAY_KIND(Intervals), TODAY_KIND(ThirtyThirty),
    TODAY_KIND(Hills), TODAY_KIND(Reps), TODAY_KIND(Pace10k), TODAY_KIND(PaceHalf),
    TODAY_KIND(TimeTrial5k), TODAY_KIND(TimeTrial10k), TODAY_KIND(Shakeout), TODAY_KIND(Race),
    { nullptr, [] { sModel->injectPlanToday(session(Kind::LongFinish, Info::Done)); } },
    { "today: done", nullptr },
    { nullptr, [] { sModel->injectPlanToday(notToday(Info::RestDay, "Rest day")); } },
    { "today: rest day, next a long run with a fast finish", nullptr },
    { nullptr, [] { sModel->injectPlanToday(notToday(Info::BeforeStart, "Plan starts", "Wed 30 Sep")); } },
    { "today: before the start", nullptr },
    { nullptr, [] { sModel->injectPlanToday(notToday(Info::Finished, "Plan complete")); } },
    { "today: plan complete", nullptr },
    { nullptr, [] { sModel->injectPlanToday(notToday(Info::NoPlan)); } },
    { "today: no plan", nullptr },
    { nullptr, [] { sModel->injectPlanToday(notToday(Info::PhoneAsks, "New plan from your phone", "Half marathon, level 2")); } },
    { "today: the phone asks", nullptr },
    { nullptr, [] { sModel->injectPlanToday(notToday(Info::Paused, "Plan paused", "since Wed 30 Sep")); } },
    { "today: paused", nullptr },
    { nullptr, [] { sModel->injectPlanToday(session(Kind::LongFinish, Info::Skipped)); } },
    { "today: skipped, next a long run with a fast finish", nullptr },
    OFFER_KIND(Long, LongRun), OFFER_KIND(LongFinish, LongRun), OFFER_KIND(Tempo, KeySession),
    OFFER_KIND(Cruise, KeySession), OFFER_KIND(Intervals, KeySession), OFFER_KIND(ThirtyThirty, KeySession),
    OFFER_KIND(Hills, KeySession), OFFER_KIND(Reps, KeySession), OFFER_KIND(Pace10k, KeySession),
    OFFER_KIND(PaceHalf, KeySession), OFFER_KIND(TimeTrial5k, KeySession), OFFER_KIND(TimeTrial10k, KeySession),
    { nullptr, [] { sModel->injectPlanToday(paceAsk(59 * 60 + 59, 59 * 60 + 58, Info::Slower)); } },
    { "today: the trial's 5K, slower", nullptr },
    { nullptr, [] { sModel->injectPlanToday(paceAsk(12 * 60, 12 * 60 + 40, Info::Jump)); } },
    { "today: the trial's 5K, a big jump", nullptr },
    { nullptr, [] {
          Info t = session(Kind::LongFinish);
          t.notice = Info::NotAllowed;   // the title says how the last change went
          sModel->injectPlanToday(t);
      } },
    { "today: a change not allowed", nullptr },
    { nullptr, [] { sModel->injectPlanToday(notToday(Info::Problem, "The race is too close or past")); } },
    { "today: problem, race", nullptr },
    { nullptr, [] { sModel->injectPlanToday(notToday(Info::Problem, "Choose a plan in the UNA app")); } },
    { "today: problem, plan", nullptr },
    { nullptr, [] { sModel->injectPlanToday(notToday(Info::Problem, "Set your 5K in the UNA app")); } },
    { "today: problem, 5K", nullptr },
    { nullptr, [] { sModel->injectPlanToday(notToday(Info::Problem, "Check the plan's settings")); } },
    { "today: problem, settings", nullptr },

    // A day of the week (its date in the title)
    { nullptr, [] { sModel->setChosenDay(20260930); go(ScreenId::PlanDay); } },
    { nullptr, [] {} },
    { nullptr, [] { sModel->injectPlanDay(session(Kind::LongFinish)); } },
    { "day: Wed 30 Sep, a long run with a fast finish", nullptr },

    // This week: every row the widest short name
    { nullptr, [] { go(ScreenId::PlanWeek); } },
    { nullptr, [] {} },
    { nullptr, [] { sModel->injectPlanWeek(worstWeek(Kind::LongFinish)); } },
    { "week: Long+fast, not logged, key", nullptr },
    { nullptr, [] { sModel->injectPlanWeek(worstWeek(Kind::TimeTrial10k)); } },
    { "week: 10K trial", nullptr },
    { nullptr, [] { sModel->injectPlanWeek(worstWeek(Kind::Intervals)); } },
    { "week: Intervals", nullptr },

    // Plan
    { nullptr, [] { go(ScreenId::PlanOverview); } },
    { nullptr, [] {} },
    { nullptr, [] { sModel->injectPlanOverview(worstOverview(Info::Session)); } },
    { "plan: week 24 of 24, race-specific, 144 of 144", nullptr },
    { nullptr, [] {
          auto o = worstOverview(Info::Session);
          o.vo2X10 = 950;        // X Run's widest figure (TP4), with the plan's start beside it
          o.vo2StartX10 = 888;
          sModel->injectPlanOverview(o);
      } },
    { "plan: X Run's VO2max, was", nullptr },
    { nullptr, [] {
          auto o = worstOverview(Info::Session);
          o.vo2X10 = 888;        // no start figure yet: named as X Run's
          sModel->injectPlanOverview(o);
      } },
    { "plan: X Run's VO2max", nullptr },
    { nullptr, [] {
          auto o = worstOverview(Info::BeforeStart);
          o.vo2X10 = 950;
          sModel->injectPlanOverview(o);
      } },
    { "plan: before the start, X Run's VO2max", nullptr },
    { nullptr, [] { sModel->injectPlanOverview(worstOverview(Info::BeforeStart)); } },
    { "plan: before the start", nullptr },
    { nullptr, [] { sModel->injectPlanOverview(worstOverview(Info::Finished)); } },
    { "plan: complete", nullptr },
    { nullptr, [] { sModel->injectPlanOverview(worstOverview(Info::Problem)); } },
    { "plan: problem", nullptr },
    { nullptr, [] { sModel->injectPlanOverview(worstOverview(Info::NoPlan)); } },
    { "plan: no plan", nullptr },

    // Paces, in imperial for the widest numbers
    { nullptr, [] { go(ScreenId::PlanPaces); } },
    { nullptr, [] {} },
    { nullptr, [] { sModel->injectPlanPaces(worstPaces(true, false)); } },
    { "paces: the paces", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "paces: your times, 5K and half", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "paces: predicted", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "paces: note, threshold and the half's gap", nullptr },
    { nullptr, [] { sModel->injectPlanPaces(worstPaces(false, true)); } },
    { "paces: note, threshold and an old half", nullptr },

    // New plan: the summary, and its problem
    { nullptr, [] {
          CustomMessage::PlanSetupInfo& d = sModel->draft();
          d.plan = 2; d.level = 3; d.days = 0x3F; d.longDay = 5; d.fiveKRecent = false; d.fiveKSec = 59 * 60 + 59;
          sModel->setDraftReady(true);
          go(ScreenId::NewPlanConfirm);
      } },
    { nullptr, [] {} },
    { nullptr, [] {
          CustomMessage::PlanPreviewInfo p;
          p.ok = true; p.weeks = 8; p.planWeeks = 12; p.start = 20260930;
          sModel->injectPlanPreview(p);
      } },
    { "new plan: the half, six days, 8 of its 12 weeks", nullptr },
    { nullptr, [] {
          CustomMessage::PlanPreviewInfo p;
          copy(p.problem, sizeof(p.problem), "The race is too close or past");
          sModel->injectPlanPreview(p);
      } },
    { "new plan: a problem", nullptr },
    { nullptr, [] { sModel->setDraftReady(false); } },

    // The step card after a short rep
    { nullptr, [] {
          Model::WorkoutStepInfo s;
          s.none = false;
          s.intensity = 1;   // rest
          copy(s.duration, sizeof(s.duration), "400 m");
          copy(s.result, sizeof(s.result), "Last rep 99:59 /mi");
          copy(s.aim, sizeof(s.aim), "aim 99:59-99:59");
          s.verdict = 2;
          s.rep = 20;
          s.reps = 20;
          sModel->injectWorkoutStep(s);
          go(ScreenId::TrackWorkoutStep);
      } },
    { nullptr, [] {} },
    { "step card: the last rep's result", nullptr },

    // Saved: the plan's lines
    { nullptr, [] { go(ScreenId::Main); } },
    { nullptr, [] { go(ScreenId::TrackSaved); } },
    { nullptr, [] {} },
    { nullptr, [] {
          CustomMessage::PlanSavedInfo s;
          s.outcome = CustomMessage::PlanSavedInfo::Done;
          s.runWeek = 23; s.weekDone = 6; s.weekSessions = 6;
          s.nextDate = 20260930; s.nextKind = static_cast<uint8_t>(Kind::LongFinish);
          sModel->injectPlanSaved(s);
      } },
    { "saved: done, week 24, next a long run with a fast finish", nullptr },
    { nullptr, [] {
          CustomMessage::PlanSavedInfo s;
          s.outcome = CustomMessage::PlanSavedInfo::TooShort;
          s.nextDate = 20260930; s.nextKind = static_cast<uint8_t>(Kind::TimeTrial10k);
          sModel->injectPlanSaved(s);
      } },
    { "saved: not counted, under half the session", nullptr },
    { nullptr, [] {
          CustomMessage::PlanSavedInfo s;
          s.outcome = CustomMessage::PlanSavedInfo::Done;
          s.trial = CustomMessage::PlanSavedInfo::Applied; s.trialSec = 3599;
          s.nextDate = 20260930; s.nextKind = static_cast<uint8_t>(Kind::LongFinish);
          sModel->injectPlanSaved(s);
      } },
    { "saved: a trial, new paces", nullptr },
    { nullptr, [] {
          CustomMessage::PlanSavedInfo s;
          s.outcome = CustomMessage::PlanSavedInfo::PartDone;
          s.trial = CustomMessage::PlanSavedInfo::Kept; s.trialSec = 3599;
          s.nextDate = 20260930; s.nextKind = static_cast<uint8_t>(Kind::TimeTrial10k);
          sModel->injectPlanSaved(s);
      } },
    { "saved: a trial, paces kept", nullptr },
    { nullptr, [] {
          CustomMessage::PlanSavedInfo s;
          s.outcome = CustomMessage::PlanSavedInfo::Done;
          s.trial = CustomMessage::PlanSavedInfo::AsksJump; s.trialSec = 3599;
          s.nextDate = 20260930; s.nextKind = static_cast<uint8_t>(Kind::LongFinish);
          sModel->injectPlanSaved(s);
      } },
    { "saved: a trial, a big jump", nullptr },
    // TP8: how it went on the Saved screen (three lines), the widest of each
    { nullptr, [] {
          CustomMessage::PlanSavedInfo s;
          s.outcome = CustomMessage::PlanSavedInfo::Done;
          s.runWeek = 23; s.weekDone = 6; s.weekSessions = 6;
          s.reviewJudged = 1; s.inPct = 40; s.fastPct = 60; s.gapSecKm = -9; s.units = 6; s.unitsIn = 2;
          s.nextDate = 20260930; s.nextKind = static_cast<uint8_t>(Kind::LongFinish);
          sModel->injectPlanSaved(s);
      } },
    { "saved: too fast, 40% on pace, next a long fast finish", nullptr },
    { nullptr, [] {
          CustomMessage::PlanSavedInfo s;
          s.outcome = CustomMessage::PlanSavedInfo::PartDone;
          s.runWeek = 23; s.weekDone = 6; s.weekSessions = 6;
          s.reviewJudged = 1; s.inPct = 5; s.fastPct = 0; s.gapSecKm = 40;
          s.offRun = 3; s.asks = 1;
          s.nextDate = 20260930; s.nextKind = static_cast<uint8_t>(Kind::LongFinish);
          sModel->injectPlanSaved(s);
      } },
    { "saved: too slow, 5% on pace, Today has a question", nullptr },
    { nullptr, [] {
          CustomMessage::PlanSavedInfo s;
          s.outcome = CustomMessage::PlanSavedInfo::Done;
          s.trial = CustomMessage::PlanSavedInfo::AsksJump; s.trialSec = 3599;
          s.runWeek = 23; s.weekDone = 6; s.weekSessions = 6;
          s.reviewJudged = 1; s.inPct = 100;
          s.nextDate = 20260930; s.nextKind = static_cast<uint8_t>(Kind::TimeTrial10k);
          sModel->injectPlanSaved(s);
      } },
    { "saved: a trial, 100% on pace", nullptr },

    // TP8: the Feel screen, each choice selected
    { nullptr, [] { go(ScreenId::Feel); } },
    { nullptr, [] {} },
    { "feel: Steady", nullptr },
    { nullptr, [] { key(Btn::L1); } },
    { "feel: Good", nullptr },
    { nullptr, [] { key(Btn::L1); } },
    { "feel: Fresh", nullptr },
    { nullptr, [] { key(Btn::L2); key(Btn::L2); key(Btn::L2); } },
    { "feel: Tired", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "feel: Spent", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "feel: Skip", nullptr },

    // TP8: the summary's How it went face, worst numbers, in miles
    { nullptr, [] {
          CustomMessage::PlanSavedInfo s;
          s.outcome = CustomMessage::PlanSavedInfo::Done;
          s.reviewJudged = 1; s.inPct = 100; s.fastPct = 0; s.gapSecKm = -600; s.fadeSecKm = -100; s.units = 32; s.unitsIn = 32;
          sModel->injectPlanSaved(s);
          go(ScreenId::TrackSummary);
      } },
    { nullptr, [] { for (int i = 0; i < 7; ++i) { key(Btn::L2); } } },
    { "summary: how it went, worst numbers", nullptr },
    { nullptr, [] {
          CustomMessage::PlanSavedInfo s;
          s.outcome = CustomMessage::PlanSavedInfo::Done;
          s.reviewJudged = 1; s.inPct = 0; s.fastPct = 0; s.gapSecKm = 600; s.fadeSecKm = 99; s.units = 2; s.unitsIn = 0;
          sModel->injectPlanSaved(s);
          go(ScreenId::TrackSummary);
      } },
    { nullptr, [] { for (int i = 0; i < 7; ++i) { key(Btn::L2); } } },
    { "summary: how it went, slow, 0%", nullptr },

    // TP8: Form, with every line the pattern can say
    { nullptr, [] { go(ScreenId::PlanForm); } },
    { nullptr, [] {} },
    { nullptr, [] { sModel->injectPlanForm(CustomMessage::PlanFormInfo{}); } },
    { "form: no sessions yet", nullptr },
    { nullptr, [] { sModel->injectPlanForm(formOf(8, XTraining::Review::Line::None, 0)); } },
    { "form: eight sessions, nothing to flag", nullptr },
    { nullptr, [] { sModel->injectPlanForm(formOf(8, XTraining::Review::Line::SlowFresh, 12)); } },
    { "form: slow, yet fresh", nullptr },
    { nullptr, [] { sModel->injectPlanForm(formOf(8, XTraining::Review::Line::SlowTired, 12)); } },
    { "form: slow, and tired", nullptr },
    { nullptr, [] { sModel->injectPlanForm(formOf(8, XTraining::Review::Line::SlowMixed, 12)); } },
    { "form: slow, mixed", nullptr },
    { nullptr, [] { sModel->injectPlanForm(formOf(8, XTraining::Review::Line::Fast, 12)); } },
    { "form: too fast", nullptr },
    { nullptr, [] { sModel->injectPlanForm(formOf(3, XTraining::Review::Line::OnTired, 0)); } },
    { "form: three sessions, held the pace while tired", nullptr },
    { nullptr, [] { sModel->injectPlanForm(formOf(1, XTraining::Review::Line::OnTrack, 0)); } },
    { "form: one session, on pace", nullptr },

    // TP8: the offer to hold the plan, on Today and its two choices
    // TP8: the titles (the SDK Title keeps a fixed width): none wider than the widest already shipped (a plan has 12 weeks at most)
    { nullptr, [] {
          const lv_font_t* font = Theme::font(Theme::Font::Italic18);
          const int32_t limit = WorkoutUi::textWidth("DO IT INSTEAD?", font);   // 1.0.0's widest offer title
          const char* const titles[] = { "HOW IT FELT", "HOW IT WENT", "FORM", "HOLD PLAN?", "WEEK 12 AGAIN" };
          int bad = 0;
          for (const char* t : titles) {
              const int32_t w = WorkoutUi::textWidth(t, font);
              if (w > limit) {
                  std::printf("AUDIT: title '%s' is %d px, wider than the %d px of DO IT INSTEAD?\n", t,
                              static_cast<int>(w), static_cast<int>(limit));
                  ++bad;
              }
          }
          std::printf("AUDIT: %-58s %s\n", "titles: none wider than DO IT INSTEAD?", bad == 0 ? "ok" : "PROBLEMS");
          sProblems += bad;
      } },
    { nullptr, [] { sModel->injectPlanToday(easeOf(Info::EaseTired)); go(ScreenId::PlanToday); } },
    { nullptr, [] {} },
    { "today: hold the plan, tired", nullptr },
    { nullptr, [] { sModel->injectPlanToday(easeOf(Info::EaseFresh)); } },
    { "today: hold the plan, fresh", nullptr },
    { nullptr, [] { sModel->injectPlanToday(easeOf(Info::EaseMixed)); } },
    { "today: hold the plan, mixed", nullptr },
    { nullptr, [] { go(ScreenId::PlanOffer); } },
    { nullptr, [] {} },
    { "offer: hold the plan", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "offer: hold the plan, not now", nullptr },

    // TP8: why holding the plan was not allowed, in Today's italic line
    { nullptr, [] {
          Info t = session(Kind::LongFinish);
          t.notice = Info::NotAllowed; t.why = 1;
          go(ScreenId::PlanToday);
          sModel->injectPlanToday(t);
      } },
    { nullptr, [] {} },
    { "today: not allowed, too near the race", nullptr },
    { nullptr, [] {
          Info t = session(Kind::LongFinish);
          t.notice = Info::NotAllowed; t.why = 2;
          sModel->injectPlanToday(t);
      } },
    { "today: not allowed, plan is long enough", nullptr },
    { nullptr, [] {
          Info t = session(Kind::LongFinish);
          t.notice = Info::NotAllowed; t.why = 3;
          sModel->injectPlanToday(t);
      } },
    { "today: not allowed, last week of plan", nullptr },
    { nullptr, [] {
          Info t = session(Kind::LongFinish);
          t.notice = Info::NotAllowed; t.why = 4;
          sModel->injectPlanToday(t);
      } },
    { "today: not allowed, already repeated", nullptr },
};
constexpr int kStepCount = static_cast<int>(sizeof(kSteps) / sizeof(kSteps[0]));

void tick(lv_timer_t*)
{
    if (sStep >= kStepCount) {
        std::printf("AUDIT: %d problem%s\n", sProblems, sProblems == 1 ? "" : "s");
        std::fflush(stdout);
        _exit(sProblems == 0 ? 0 : 1);
    }
    const Step& st = kSteps[sStep++];
    if (st.run) {
        st.run();
    }
    if (st.name) {
        const int n = Audit::check(st.name);
        std::printf("AUDIT: %-58s %s\n", st.name, n == 0 ? "ok" : "PROBLEMS");
        sProblems += n;
    }
}
} // namespace

void startIfRequested(Model& model)
{
    if (std::getenv("XTRAINING_AUDIT") == nullptr) {
        return;
    }
    sModel = &model;
    lv_timer_create(&tick, kTickMs, nullptr);
}

} // namespace AuditDriver

#endif // SIMULATOR
