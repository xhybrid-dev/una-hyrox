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

Sauna::SessionSnapshot worstSnapshot()
{
    Sauna::SessionSnapshot s;
    s.state           = static_cast<uint8_t>(Sauna::RunState::Running);
    s.phase           = static_cast<uint8_t>(Sauna::Phase::Plunge);   // the widest name
    s.round           = 255;
    s.laps            = 64;
    s.mode            = static_cast<uint8_t>(Sauna::Mode::SaunaPlunge);
    s.rest            = 1;
    s.hr              = 65535;
    s.recoveryState   = static_cast<uint8_t>(Sauna::Recovery::State::Pending);
    s.recoverySecLeft = 60;
    s.hour            = 23;
    s.minute          = 59;
    s.battery         = 100;
    s.lapMs           = 3599000u;          // the last 60 px time, 59:59
    s.activeMs        = kMax32;            // 99:59:59
    s.elapsedMs       = kMax32;
    s.phaseMs[0] = s.phaseMs[1] = s.phaseMs[2] = kMax32;
    return s;
}

Sauna::SessionSnapshot worstPausedSnapshot()
{
    Sauna::SessionSnapshot s = worstSnapshot();
    s.state         = static_cast<uint8_t>(Sauna::RunState::Paused);
    s.hr            = 0;
    s.logOk         = 0;
    s.lapMs         = kMax32;              // the 40 px face: 99:59:59
    s.recoveryState = static_cast<uint8_t>(Sauna::Recovery::State::Done);
    s.recoveryDrop  = INT16_MIN;
    return s;
}

Sauna::SavedSession worstSaved(bool saved)
{
    Sauna::SavedSession s;
    s.saved                = saved ? 1u : 0u;
    s.activeMs             = kMax32;
    s.summary.laps         = Sauna::kMaxLaps;
    s.summary.rounds       = 255;
    s.summary.saunaMs      = kMax32;
    s.summary.plungeMs     = kMax32;
    s.summary.restMs       = kMax32;
    s.summary.hrAvg        = 65535;
    s.summary.hrMax        = 65535;
    s.summary.hasRecovery  = true;
    s.summary.bestDrop     = INT16_MIN;
    return s;
}

Sauna::LapsPageData worstLaps()
{
    Sauna::LapsPageData p;
    p.first = 0;
    p.total = Sauna::kMaxLaps;
    p.count = Sauna::kLapsPerPage;
    for (uint8_t i = 0; i < Sauna::kLapsPerPage; ++i) {
        p.rows[i].phase         = static_cast<uint8_t>(Sauna::Phase::Plunge);
        p.rows[i].round         = 255;
        p.rows[i].recoveryState = static_cast<uint8_t>(Sauna::Recovery::State::Done);
        p.rows[i].recoveryDrop  = INT16_MIN;
        p.rows[i].hrAvg         = 65535;
        p.rows[i].durationMs    = kMax32;
    }
    return p;
}

Sauna::LapEventData worstLapEvent()
{
    Sauna::LapEventData e;
    e.finishedPhase = static_cast<uint8_t>(Sauna::Phase::Plunge);
    e.finishedRound = 255;
    e.nextPhase     = static_cast<uint8_t>(Sauna::Phase::Plunge);
    e.nextRound     = 255;
    e.hrAvg         = 65535;
    e.durationMs    = kMax32;
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
    // The live screen, main face
    { nullptr, [] { sModel->injectSnapshot(worstSnapshot()); } },       // the menu goes to Live
    { nullptr, [] { sModel->injectSnapshot(worstSnapshot()); } },
    { "live main, worst case", nullptr },
    { nullptr, [] { sModel->injectSnapshot(worstPausedSnapshot()); } },
    { "live main, paused, 10 h lap, file error, +32768", nullptr },
    { nullptr, [] { sModel->injectLapEvent(worstLapEvent()); } },
    { "lap toast", nullptr },
    // Totals
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(worstSnapshot()); } },
    { "live totals, four rows", nullptr },
    { nullptr, [] {
          Sauna::SessionSnapshot s = worstSnapshot();
          s.mode = static_cast<uint8_t>(Sauna::Mode::SaunaOnly);
          sModel->injectSnapshot(s);
      } },
    { "live totals, just sauna", nullptr },
    // Clock
    { nullptr, [] { key(Btn::L2); sModel->injectSnapshot(worstSnapshot()); } },
    { "live clock", nullptr },
    { nullptr, [] {
          Sauna::SessionSnapshot s = worstSnapshot();
          s.battery = Sauna::kBatteryUnknown;
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
    // Summary
    { nullptr, [] { sModel->injectSaved(worstSaved(true)); ScreenManager::instance().goTo(ScreenId::Summary); } },
    { nullptr, [] {} },
    { "summary, overview", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "summary, time", nullptr },
    { nullptr, [] { key(Btn::L2); } },
    { "summary, heart rate", nullptr },
    { nullptr, [] { key(Btn::L2); sModel->injectLapsPage(worstLaps()); } },
    { "summary, laps", nullptr },
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
    if (std::getenv("XSAUNA_AUDIT") == nullptr) {
        return;
    }
    sModel = &model;
    // Give the service its first update and the first screen time to settle.
    lv_timer_create(&tick, kTickMs, nullptr);
}

} // namespace AuditDriver

#endif // SIMULATOR
