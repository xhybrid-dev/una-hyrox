/**
 ******************************************************************************
 * @file    AuditDriver.cpp
 * @brief   The simulator's text-fit audit run (see the header).
 ******************************************************************************
 */

#include "gui/AuditDriver.hpp"

#if defined(SIMULATOR)

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include "lvgl.h"

#include "SDK/GUI/Button.hpp"
#include "SDK/Messages/AccessoryMessages.hpp"

#include "gui/Audit.hpp"
#include "gui/model/Model.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/screens/ScreenManager.hpp"

namespace AuditDriver
{

namespace
{
constexpr uint32_t kTickMs = 250;
constexpr uint32_t kMax32  = 0xFFFFFFFFu;

Model* sModel    = nullptr;
int    sStep     = 0;
int    sProblems = 0;

namespace Btn = SDK::GUI::Button;

Workout::SessionSnapshot worstSnapshot()
{
    Workout::SessionSnapshot s;
    s.state      = static_cast<uint8_t>(Workout::RunState::Running);
    s.laps       = Workout::kMaxLaps;
    s.zone       = 5;
    s.autoLapMin = 30;
    s.hr         = 65535;
    s.hour       = 23;
    s.minute     = 59;
    s.battery    = 100;
    s.lapMs      = 3599000u;          // the last 60 px time, 59:59
    s.activeMs   = kMax32;            // 99:59:59
    s.elapsedMs  = kMax32;
    s.kcalX10    = kMax32;
    for (uint8_t z = 0; z < Workout::kZoneCount; ++z) {
        s.zoneMs[z] = kMax32;
    }
    return s;
}

Workout::SessionSnapshot worstPausedSnapshot()
{
    Workout::SessionSnapshot s = worstSnapshot();
    s.state = static_cast<uint8_t>(Workout::RunState::Paused);
    s.hr    = 0;
    s.zone  = 0;
    s.logOk = 0;
    s.lapMs = kMax32;                 // the 40 px face: 99:59:59
    return s;
}

Workout::SavedSession worstSaved(bool saved)
{
    Workout::SavedSession s;
    s.saved            = saved ? 1u : 0u;
    s.activeMs         = kMax32;
    s.summary.laps     = Workout::kMaxLaps;
    s.summary.hrAvg    = 65535;
    s.summary.hrMax    = 65535;
    s.summary.kcalX10  = kMax32;
    for (uint8_t z = 0; z < Workout::kZoneCount; ++z) {
        s.summary.zoneMs[z] = kMax32;
    }
    return s;
}

Workout::LapsPageData worstLaps(uint8_t first)
{
    Workout::LapsPageData p;
    p.first = first;
    p.total = Workout::kMaxLaps;
    p.count = Workout::kLapsPerPage;
    for (uint8_t i = 0; i < Workout::kLapsPerPage; ++i) {
        p.rows[i].autoEnded  = 1;
        p.rows[i].hrAvg      = 65535;
        p.rows[i].hrMax      = 65535;
        p.rows[i].kcal       = 65535;
        p.rows[i].durationMs = kMax32;
    }
    return p;
}

Workout::LapEventData worstLapEvent()
{
    Workout::LapEventData e;
    e.finishedLap = Workout::kMaxLaps - 1;
    e.autoEnded   = 1;
    e.hrAvg       = 65535;
    e.durationMs  = kMax32;
    return e;
}

using Workout::SessionType;
using Workout::StepKind;

/// The worst snapshot, on a plan's step.
Workout::SessionSnapshot planSnapshot(SessionType t, StepKind k, uint8_t round, uint8_t rounds, uint8_t station,
                                      uint32_t stepMs, uint32_t remainingMs, bool done = false)
{
    Workout::SessionSnapshot s = worstSnapshot();
    Workout::PlanView& v = s.plan;
    v.type        = static_cast<uint8_t>(t);
    v.kind        = static_cast<uint8_t>(k);
    v.step        = 1;
    v.steps       = Workout::kMaxLaps;
    v.round       = round;
    v.rounds      = rounds;
    v.station     = station;
    v.done        = done ? 1u : 0u;
    v.amrapRounds = 65535;
    v.distanceM   = station > 0 ? Workout::circuitStation(station).distanceM : 1000;
    v.stepMs      = stepMs;
    v.remainingMs = remainingMs;
    if (done) {
        s.state = static_cast<uint8_t>(Workout::RunState::Paused);
    }
    return s;
}

Workout::LapEventData planLapEvent(Workout::Cue cue, StepKind finished, uint8_t round, uint8_t station)
{
    Workout::LapEventData e = worstLapEvent();
    e.autoEnded = 0;
    e.cue       = static_cast<uint8_t>(cue);
    e.kind      = static_cast<uint8_t>(finished);
    e.round     = round;
    e.station   = station;
    return e;
}

Workout::SavedSession planSaved(SessionType t)
{
    Workout::SavedSession s = worstSaved(true);
    s.type        = static_cast<uint8_t>(t);
    s.planDone    = 1;
    s.amrapRounds = 65535;
    return s;
}

/// Laps 61 to 63 of a plan: the widest names its kind can give.
Workout::LapsPageData planLaps(SessionType t)
{
    Workout::LapsPageData p = worstLaps(60);
    for (uint8_t i = 0; i < Workout::kLapsPerPage; ++i) {
        Workout::LapRow& r = p.rows[i];
        r.autoEnded = 0;
        r.round     = t == SessionType::Amrap ? 255 : 60;
        switch (t) {
            case SessionType::Circuit:
                r.kind    = static_cast<uint8_t>(i == 1 ? StepKind::Run : StepKind::Station);
                r.station = i == 0 ? 8 : 2;   // "Wall balls", then a run, then "Sled push"
                r.round   = 8;
                break;
            case SessionType::Strength:
            case SessionType::Tabata:
            case SessionType::Custom:
                r.kind = static_cast<uint8_t>(i == 1 ? StepKind::Rest : StepKind::Work);
                break;
            default:
                r.kind = static_cast<uint8_t>(StepKind::Work);
                break;
        }
    }
    return p;
}

void key(uint8_t code)
{
    Screen* s = ScreenManager::instance().current();
    if (s) {
        s->onKey(code);
    }
}

// One step: something to do, or a check of what is on show.
struct Step {
    const char* name;      ///< Non-null: check, and log under this name
    void (*run)();         ///< Non-null: do this
};

const Step kSteps[] = {
    // The main menu: the longest strap hint, then the longest auto lap
    { nullptr, [] { sModel->injectStrap(static_cast<uint8_t>(SDK::Accessory::State::CONNECTED), "Polar H10 ABCDEFGH"); } },
    { "main menu, strap connected", nullptr },
    { nullptr, [] { sModel->injectStrap(0xFF, ""); } },
    { "main menu, wrist", nullptr },
    // The live screen, main face
    { nullptr, [] { sModel->injectSnapshot(worstSnapshot()); } },       // the menu goes to Live
    { nullptr, [] { sModel->injectSnapshot(worstSnapshot()); } },
    { "live main, worst case", nullptr },
    { nullptr, [] { sModel->injectSnapshot(worstPausedSnapshot()); } },
    { "live main, paused, 10 h lap, file error", nullptr },
    { nullptr, [] { sModel->injectLapEvent(worstLapEvent()); } },
    { "lap toast, auto", nullptr },
    // Totals
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(worstSnapshot()); } },
    { "live totals", nullptr },
    // Zones
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(worstSnapshot()); } },
    { "live zones", nullptr },
    // Clock
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(worstSnapshot()); } },
    { "live clock", nullptr },
    { nullptr, [] {
          Workout::SessionSnapshot s = worstSnapshot();
          s.battery = Workout::kBatteryUnknown;
          sModel->injectSnapshot(s);
      } },
    { "live clock, battery unknown", nullptr },
    // Hold to confirm
    { nullptr, [] { sModel->setHoldMode(Model::HoldMode::Discard); ScreenManager::instance().goTo(ScreenId::HoldConfirm); } },
    { nullptr, [] {} },
    { "hold to discard", nullptr },
    // Result: not saved
    { nullptr, [] { sModel->stop(true); ScreenManager::instance().goTo(ScreenId::Result); } },
    { nullptr, [] { sModel->injectSaved(worstSaved(false)); } },
    { "result, not saved", nullptr },
    // Result: saved, with the longest Streak line
    { nullptr, [] { sModel->stop(true); ScreenManager::instance().goTo(ScreenId::Result); } },
    { nullptr, [] { sModel->injectStreakLine(true, "255 of 7 this week", "65535-week streak"); sModel->injectSaved(worstSaved(true)); } },
    { "result, saved, Streak line", nullptr },
    // Summary
    { nullptr, [] { sModel->injectSaved(worstSaved(true)); ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] {} },
    { "summary, overview", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "summary, zones", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "summary, heart rate", nullptr },
    // The laps, at the widest lap numbers: from the overview, L1 twice is laps
    // 61 to 63. (The first laps page would be overwritten by the simulator's
    // own service, which answers a request for laps 1 to 3 with none.)
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] { key(Btn::L1); key(Btn::L1); sModel->injectLapsPage(worstLaps(60)); } },
    { "summary, laps 61 to 63", nullptr },

    // -- The kinds of workout (W3) --------------------------------------------
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Live); } },
    { nullptr, [] { key(Btn::L2); key(Btn::L2); key(Btn::L2); /* round to the main face */
                    sModel->injectSnapshot(planSnapshot(SessionType::Strength, StepKind::Work, 20, 20, 0, 3599000u, 0)); } },
    { "plan: strength set 20/20, 59:59", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Strength, StepKind::Rest, 19, 20, 0, 1000u, 299000u)); } },
    { "plan: strength rest", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Emom, StepKind::Work, 60, 60, 0, 1000u, 59000u)); } },
    { "plan: EMOM min 60/60", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Amrap, StepKind::Work, 255, 1, 0, 1000u, 3600000u)); } },
    { "plan: AMRAP round 255, 1:00:00 left", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Tabata, StepKind::Rest, 8, 8, 0, 8000u, 2000u)); } },
    { "plan: Tabata rest 8/8, last seconds", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Custom, StepKind::Work, 30, 30, 0, 1000u, 299000u)); } },
    { "plan: intervals work 30/30", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Circuit, StepKind::Station, 8, 8, 8, kMax32, 0)); } },
    { "plan: circuit Wall balls, 100 reps", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Circuit, StepKind::Station, 4, 8, 4, 1000u, 0)); } },
    { "plan: circuit Burpees", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Circuit, StepKind::Run, 8, 8, 0, kMax32, 0)); } },
    { "plan: circuit run 8/8, 1 km", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Custom, StepKind::Work, 30, 30, 0, 1000u, 0, true)); } },
    { "plan: done, 99:59:59", nullptr },
    { nullptr, [] {
          Workout::SessionSnapshot s = planSnapshot(SessionType::Emom, StepKind::Work, 60, 60, 0, 1000u, 59000u);
          s.state = static_cast<uint8_t>(Workout::RunState::Paused);
          sModel->injectSnapshot(s);
      } },
    { "plan: EMOM paused", nullptr },
    // The toasts
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Circuit, StepKind::Run, 8, 8, 0, 1000u, 0)); } },
    { nullptr, [] { sModel->injectLapEvent(planLapEvent(Workout::Cue::Station, StepKind::Run, 8, 0)); } },
    { "toast: circuit, Wall balls begins", nullptr },
    { nullptr, [] { sModel->injectLapEvent(planLapEvent(Workout::Cue::Run, StepKind::Station, 7, 8)); } },
    { "toast: circuit, Run 8 after Wall balls", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Emom, StepKind::Work, 60, 60, 0, 1000u, 59000u)); } },
    { nullptr, [] { sModel->injectLapEvent(planLapEvent(Workout::Cue::Work, StepKind::Work, 59, 0)); } },
    { "toast: EMOM, Minute 60", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Amrap, StepKind::Work, 255, 1, 0, 1000u, 1000u)); } },
    { nullptr, [] { sModel->injectLapEvent(planLapEvent(Workout::Cue::Round, StepKind::Work, 254, 0)); } },
    { "toast: AMRAP, Round 255", nullptr },
    { nullptr, [] { sModel->injectSnapshot(planSnapshot(SessionType::Strength, StepKind::Work, 20, 20, 0, 1000u, 0)); } },
    { nullptr, [] { sModel->injectLapEvent(planLapEvent(Workout::Cue::Rest, StepKind::Work, 20, 0)); } },
    { "toast: strength, Rest after Set 20", nullptr },
    { nullptr, [] { sModel->injectLapEvent(planLapEvent(Workout::Cue::Done, StepKind::Work, 20, 0)); } },
    { "toast: Done", nullptr },
    // Totals on a plan: the type in place of the auto lap
    { nullptr, [] { key(Btn::R1); /* dismiss the toast */ } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Live); } },
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(planSnapshot(SessionType::Custom, StepKind::Work, 30, 30, 0, 1000u, 1000u)); } },
    { "plan: totals, Type Intervals", nullptr },
    // The action menu once a plan is done (the SDK's wheel: built, not measured)
    { nullptr, [] { key(Btn::L1); sModel->injectSnapshot(planSnapshot(SessionType::Custom, StepKind::Work, 30, 30, 0, 1000u, 0, true)); } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Action); } },
    { nullptr, [] {} },
    { "action menu, plan done", nullptr },
    // The summary of each kind: its line, then laps 61 to 63 of its widest names
    { nullptr, [] { sModel->injectSaved(planSaved(SessionType::Amrap)); ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] {} },
    { "summary: AMRAP, 65535 rounds", nullptr },
    { nullptr, [] { sModel->injectSaved(planSaved(SessionType::Tabata)); ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] {} },
    { "summary: Tabata, of 8 rounds", nullptr },
    { nullptr, [] { sModel->injectSaved(planSaved(SessionType::Circuit)); ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] { key(Btn::L1); key(Btn::L1); sModel->injectLapsPage(planLaps(SessionType::Circuit)); } },
    { "summary: circuit laps, Wall balls", nullptr },
    { nullptr, [] { sModel->injectSaved(planSaved(SessionType::Amrap)); ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] { key(Btn::L1); key(Btn::L1); sModel->injectLapsPage(planLaps(SessionType::Amrap)); } },
    { "summary: AMRAP laps, Round 255", nullptr },
    { nullptr, [] { sModel->injectSaved(planSaved(SessionType::Custom)); ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] { key(Btn::L1); key(Btn::L1); sModel->injectLapsPage(planLaps(SessionType::Custom)); } },
    { "summary: intervals laps, Work 60 / Rest 60", nullptr },
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
        std::printf("AUDIT: %-52s %s\n", st.name, n == 0 ? "ok" : "PROBLEMS");
        sProblems += n;
    }
}
} // namespace

void startIfRequested(Model& model)
{
    if (std::getenv("HXWORKOUT_AUDIT") == nullptr) {
        return;
    }
    sModel = &model;
    // Give the service its first update and the first screen time to settle.
    lv_timer_create(&tick, kTickMs, nullptr);
}

} // namespace AuditDriver

#endif // SIMULATOR
