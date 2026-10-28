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
