/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   HybridX Streak's service process.
 *
 * On start (PLAN S2):
 *   1. the clock: local day and time, or "clock unset" (NOTES E.5);
 *   2. load state.json (falling back to its .bak);
 *   3. read the goal from AppConfig (the phone may have changed it);
 *   4. scan every app's Activity folder, credit, close finished weeks
 *      (StreakModel::update);
 *   5. save state.json, and the public ../SharedData/HybridX/streak.json the
 *      glance reads;
 *   6. when the GUI starts, send it the views and the moments to play.
 * Then it answers the GUI (log, undo, exclude, shields, goal, buzz), saving
 * after every change, and exits once its GUI has gone. Nothing runs while the
 * watch is idle (Docs/service-lifecycle.md 9: recompute on open).
 *
 * The model, the scanner and the work buffers are statics placed at start-up:
 * together about 15 KB, far more than the service's 10 KB stack.
 ******************************************************************************
 */

#ifndef STREAK_SERVICE_HPP
#define STREAK_SERVICE_HPP

#include <cstdint>
#include <memory>

#include "SDK/AppConfig/AppConfig.hpp"
#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/CommandMessages.hpp"

#include "Commands.hpp"
#include "Goal.hpp"
#include "StreakEvents.hpp"

class Service
{
public:
    explicit Service(SDK::Kernel& kernel);

    /// The service's main loop; returning ends the whole app.
    void run();

private:
    struct Now {
        bool     clockOk     = false;
        int32_t  day         = 0;   ///< local day number
        uint32_t secondOfDay = 0;
    };

    Now          now() const;
    void         open();
    void         save();
    Streak::Goal goalFromConfig() const;
    void         goalToConfig(const Streak::Goal& goal);
    void         sendViews();
    void         sendMoments();
    void         handle(SDK::MessageBase* msg);

    void celebrate(CustomMessage::Moment moment);
    void vibrate(const SDK::Message::RequestVibroPlay::Effect* effects, uint8_t count, uint16_t gapMs);
    void backlightOn(uint32_t timeoutMs);

    SDK::Kernel&                     mKernel;
    std::unique_ptr<SDK::AppConfig>  mConfig;
    Streak::Events                   mMoments {};   ///< waiting for the GUI
    bool                             mGuiStarted = false;
    bool                             mClockOk    = false;

    static constexpr uint32_t skStartupGraceMs = 5000u;
    static constexpr uint32_t skWaitMs         = 1000u;
    static constexpr uint32_t skBacklightMs    = 5000u;
    /// 2026-01-01 00:00 UTC: earlier means the watch has lost the time (PLAN 6.1).
    static constexpr int64_t  skClockFloorUtc  = 1767225600;
};

#endif // STREAK_SERVICE_HPP
