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

Ride::SessionSnapshot worstSnapshot()
{
    Ride::SessionSnapshot s;
    s.state      = static_cast<uint8_t>(Ride::RunState::Running);
    s.laps       = Ride::kMaxLaps;
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
    for (uint8_t z = 0; z < Ride::kZoneCount; ++z) {
        s.zoneMs[z] = kMax32;
    }
    return s;
}

/// An outdoor ride at the extremes: every GPS figure at its cap.
Ride::SessionSnapshot worstOutdoorSnapshot()
{
    Ride::SessionSnapshot s = worstSnapshot();
    s.outdoor        = 1;
    s.gps            = static_cast<uint8_t>(Ride::GpsState::Fix);
    s.speedMmS       = kMax32;
    s.distCm         = kMax32;
    s.ascentCm       = kMax32;
    s.lapDistCm      = kMax32;
    s.avgSpeedMmS    = kMax32;
    s.lapAvgSpeedMmS = kMax32;
    return s;
}

/// An indoor plan at its longest: Intervals of 30 rounds of 10 minutes.
Ride::SessionSnapshot worstPlanSnapshot(Ride::StepKind kind, uint32_t remainingMs, bool done = false)
{
    Ride::SessionSnapshot s = worstSnapshot();
    s.plan.type        = static_cast<uint8_t>(Ride::SessionType::Custom);
    s.plan.kind        = static_cast<uint8_t>(kind);
    s.plan.steps       = 59;
    s.plan.step        = 59;
    s.plan.round       = 30;
    s.plan.rounds      = 30;
    s.plan.done        = done ? 1u : 0u;
    s.plan.stepMs      = 600000u - remainingMs;
    s.plan.remainingMs = done ? 0u : remainingMs;
    if (done) {
        s.state = static_cast<uint8_t>(Ride::RunState::Paused);
    }
    return s;
}

Ride::PlanConfig longestPlan()
{
    Ride::PlanConfig p;
    p.type    = Ride::SessionType::Custom;
    p.workSec = Ride::kWorkSecMax;
    p.restSec = Ride::kRestSecMax;
    p.rounds  = Ride::kRoundsMax;
    return p;
}

Ride::SessionSnapshot worstPausedSnapshot()
{
    Ride::SessionSnapshot s = worstSnapshot();
    s.state = static_cast<uint8_t>(Ride::RunState::Paused);
    s.hr    = 0;
    s.zone  = 0;
    s.logOk = 0;
    s.lapMs = kMax32;                 // the 40 px face: 99:59:59
    return s;
}

Ride::SavedSession worstSavedOutdoor()
{
    Ride::SavedSession s;
    s.saved              = 1;
    s.outdoor            = 1;
    s.activeMs           = kMax32;
    s.summary.laps       = Ride::kMaxLaps;
    s.summary.hrAvg      = 65535;
    s.summary.hrMax      = 65535;
    s.summary.kcalX10    = kMax32;
    s.summary.distCm     = kMax32;
    s.summary.ascentCm   = kMax32;
    s.summary.descentCm  = kMax32;
    s.summary.maxSpeedMmS = kMax32;
    s.summary.avgSpeedMmS = kMax32;
    for (uint8_t z = 0; z < Ride::kZoneCount; ++z) {
        s.summary.zoneMs[z] = kMax32;
    }
    return s;
}

Ride::SavedSession worstSaved(bool saved)
{
    Ride::SavedSession s;
    s.saved            = saved ? 1u : 0u;
    s.activeMs         = kMax32;
    s.summary.laps     = Ride::kMaxLaps;
    s.summary.hrAvg    = 65535;
    s.summary.hrMax    = 65535;
    s.summary.kcalX10  = kMax32;
    for (uint8_t z = 0; z < Ride::kZoneCount; ++z) {
        s.summary.zoneMs[z] = kMax32;
    }
    return s;
}

Ride::LapsPageData worstPlanLaps(uint8_t first)
{
    Ride::LapsPageData p;
    p.first = first;
    p.total = 59;
    p.count = Ride::kLapsPerPage;
    for (uint8_t i = 0; i < Ride::kLapsPerPage; ++i) {
        p.rows[i].autoEnded  = 1;
        p.rows[i].kind       = static_cast<uint8_t>(i % 2 == 0 ? Ride::StepKind::Rest : Ride::StepKind::Work);
        p.rows[i].round      = 30;
        p.rows[i].hrAvg      = 65535;
        p.rows[i].hrMax      = 65535;
        p.rows[i].kcal       = 65535;
        p.rows[i].durationMs = kMax32;
    }
    return p;
}

Ride::LapsPageData worstLaps(uint8_t first)
{
    Ride::LapsPageData p;
    p.first = first;
    p.total = Ride::kMaxLaps;
    p.count = Ride::kLapsPerPage;
    for (uint8_t i = 0; i < Ride::kLapsPerPage; ++i) {
        p.rows[i].autoEnded  = 1;
        p.rows[i].hrAvg      = 65535;
        p.rows[i].hrMax      = 65535;
        p.rows[i].kcal       = 65535;
        p.rows[i].durationMs = kMax32;
        p.rows[i].distCm     = kMax32;
    }
    return p;
}

Ride::LapEventData worstLapEvent()
{
    Ride::LapEventData e;
    e.finishedLap = Ride::kMaxLaps - 1;
    e.autoEnded   = 1;
    e.hrAvg       = 65535;
    e.durationMs  = kMax32;
    e.distCm      = kMax32;
    e.avgSpeedMmS = kMax32;
    return e;
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
    // The main menu: each row, with the longest hints. (The wheel is the SDK's widget, so it is
    // checked by eye in the captures as well.)
    { nullptr, [] { sModel->injectStrap(static_cast<uint8_t>(SDK::Accessory::State::CONNECTED), "Polar H10 ABCDEFGH"); } },
    { "main menu, start (finding GPS)", nullptr },
    { nullptr, [] { key(Btn::R1); } },
    { "main menu, start (no GPS warning)", nullptr },
    { nullptr, [] { sModel->injectGps(static_cast<uint8_t>(Ride::GpsState::Fix)); } },
    { "main menu, start (GPS ready)", nullptr },
    { nullptr, [] { sModel->injectSettings(false, false); } },
    { "main menu, start (indoor)", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "main menu, ride row", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "main menu, heart rate row (strap)", nullptr },
    { nullptr, [] { sModel->injectStrap(0xFF, ""); } },
    { "main menu, heart rate row (wrist)", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "main menu, type row (Free)", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "main menu, auto lap row (indoor)", nullptr },
    { nullptr, [] { sModel->injectPlanSettings(longestPlan(), 0, 0, true); } },
    { "main menu, settings row (longest Intervals)", nullptr },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::PlanSettings); } },
    { nullptr, [] { sModel->injectPlanSettings(longestPlan(), 0, 0, true); } },
    { "plan settings, longest", nullptr },
    { nullptr, [] { key(Btn::L2); key(Btn::L2); } },
    { "plan settings, rounds", nullptr },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] {
          Ride::PlanConfig p;
          p.type = Ride::SessionType::Tabata;
          sModel->injectPlanSettings(p, 0, 0, true);
      } },
    { "main menu, settings row (Tabata)", nullptr },
    { nullptr, [] { key(Btn::L1); } },
    { "main menu, type row (Tabata)", nullptr },
    { nullptr, [] { sModel->injectSettings(true, false); sModel->injectPlanSettings(Ride::PlanConfig(), 0, 25, false); } },
    { "main menu, auto lap row (25 km)", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "main menu, auto-pause row (Off)", nullptr },
    { nullptr, [] { sModel->injectSettings(false, false); } },
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
          Ride::SessionSnapshot s = worstSnapshot();
          s.battery = Ride::kBatteryUnknown;
          sModel->injectSnapshot(s);
      } },
    { "live clock, battery unknown", nullptr },
    // An indoor plan: the step face, its countdown and its end, the toasts, the totals and the clock
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(worstPlanSnapshot(Ride::StepKind::Work, 600000u)); } },
    { "live main, plan work, 10 minutes", nullptr },
    { nullptr, [] { sModel->injectSnapshot(worstPlanSnapshot(Ride::StepKind::Rest, 2500u)); } },
    { "live main, plan rest, countdown", nullptr },
    { nullptr, [] { sModel->injectSnapshot(worstPlanSnapshot(Ride::StepKind::Work, 0u, true)); } },
    { "live main, plan done", nullptr },
    { nullptr, [] {
          Ride::LapEventData e = worstLapEvent();
          e.cue   = static_cast<uint8_t>(Ride::Cue::Work);
          e.kind  = static_cast<uint8_t>(Ride::StepKind::Rest);
          e.round = 29;
          e.distCm = 0;
          sModel->injectLapEvent(e);
      } },
    { "lap toast, plan work", nullptr },
    { nullptr, [] {
          Ride::LapEventData e = worstLapEvent();
          e.cue   = static_cast<uint8_t>(Ride::Cue::Done);
          e.kind  = static_cast<uint8_t>(Ride::StepKind::Work);
          e.round = 30;
          sModel->injectLapEvent(e);
      } },
    { "lap toast, plan done", nullptr },
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(worstPlanSnapshot(Ride::StepKind::Rest, 600000u)); } },
    { "live totals, plan", nullptr },
    { nullptr, [] { key(Btn::L2); key(Btn::L2); sModel->injectSnapshot(worstPlanSnapshot(Ride::StepKind::Rest, 600000u)); } },
    { "live clock, plan", nullptr },
    // Outdoors: the main face leads with speed, then the Lap face, then the totals, the toast
    { nullptr, [] { sModel->injectSettings(true, false); sModel->injectSnapshot(worstOutdoorSnapshot()); key(Btn::L2); } },
    { "live main, outdoor, worst case", nullptr },
    { nullptr, [] {
          Ride::SessionSnapshot s = worstOutdoorSnapshot();
          s.state = static_cast<uint8_t>(Ride::RunState::Paused);
          s.logOk = 0;
          sModel->injectSnapshot(s);
      } },
    { "live main, outdoor, paused, file error", nullptr },
    { nullptr, [] {
          Ride::SessionSnapshot s = worstOutdoorSnapshot();
          s.state      = static_cast<uint8_t>(Ride::RunState::Paused);
          s.autoPaused = 1;
          sModel->injectSnapshot(s);
      } },
    { "live main, outdoor, auto-paused", nullptr },
    { nullptr, [] { sModel->injectSnapshot(worstOutdoorSnapshot()); key(Btn::L2); } },
    { "live lap, outdoor", nullptr },
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(worstOutdoorSnapshot()); } },
    { "live totals, outdoor", nullptr },
    { nullptr, [] { sModel->injectLapEvent(worstLapEvent()); } },
    { "lap toast, outdoor", nullptr },
    { nullptr, [] { key(Btn::L1); key(Btn::L1); sModel->injectSettings(true, true); sModel->injectSnapshot(worstOutdoorSnapshot()); } },
    { "live main, outdoor, imperial", nullptr },
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(worstOutdoorSnapshot()); } },
    { "live lap, outdoor, imperial", nullptr },
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(worstOutdoorSnapshot()); } },
    { "live totals, outdoor, imperial", nullptr },
    // The action menu, and a finished plan's
    { nullptr, [] { sModel->injectSnapshot(worstSnapshot()); ScreenManager::instance().goTo(ScreenId::Action); } },
    { nullptr, [] {} },
    { "action menu", nullptr },
    { nullptr, [] { sModel->injectSnapshot(worstPlanSnapshot(Ride::StepKind::Work, 0u, true)); ScreenManager::instance().goTo(ScreenId::Live); } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Action); } },
    { nullptr, [] {} },
    { "action menu, plan done", nullptr },
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
    { nullptr, [] { sModel->injectStreakLine(true, "255 of 7 this week", "65535-week streak", 65535); sModel->injectSaved(worstSaved(true)); } },
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
    // An indoor plan's summary: titled with its kind, rounds, and its steps as laps
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] {
          Ride::SavedSession s = worstSaved(true);
          s.type = static_cast<uint8_t>(Ride::SessionType::Custom);
          s.summary.laps = 59;
          sModel->injectSaved(s);
          ScreenManager::instance().goTo(ScreenId::Summary);
      } },
    { nullptr, [] {} },
    { "summary, plan overview", nullptr },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] { key(Btn::L1); key(Btn::L1); sModel->injectLapsPage(worstPlanLaps(54)); } },
    { "summary, plan laps 55 to 57", nullptr },
    // The outdoor summary: overview, the Ride page, and the laps with distances
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] { sModel->injectSaved(worstSavedOutdoor()); ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] {} },
    { "summary, outdoor overview", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "summary, outdoor ride page", nullptr },
    { nullptr, [] { sModel->injectSettings(true, false); ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] { key(Btn::L1); key(Btn::L1); sModel->injectLapsPage(worstLaps(60)); } },
    { "summary, outdoor laps 61 to 63", nullptr },
    { nullptr, [] { sModel->injectSettings(true, true); ScreenManager::instance().goTo(ScreenId::Main); } },
    { nullptr, [] { ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] { key(Btn::L2); } },
    { "summary, outdoor ride page, imperial", nullptr },
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
    if (std::getenv("HXRIDE_AUDIT") == nullptr) {
        return;
    }
    sModel = &model;
    // Give the service its first update and the first screen time to settle.
    lv_timer_create(&tick, kTickMs, nullptr);
}

} // namespace AuditDriver

#endif // SIMULATOR
