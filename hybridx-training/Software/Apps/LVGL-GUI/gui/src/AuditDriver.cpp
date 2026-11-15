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
