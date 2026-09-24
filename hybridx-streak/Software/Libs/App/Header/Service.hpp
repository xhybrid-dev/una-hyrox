/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   HybridX Streak's service process.
 *
 * Phase S0: the lifecycle and the haptics. The service answers the GUI's
 * requests to mark a moment, and exits once its GUI has gone. It holds no
 * sensor and no thread while the watch is idle: streak state is a function of
 * stored dates and the current time, so it is recomputed on open
 * (Docs/service-lifecycle.md section 9) rather than kept resident.
 *
 * Scanning activities and the rules engine arrive in S1/S2.
 ******************************************************************************
 */

#ifndef STREAK_SERVICE_HPP
#define STREAK_SERVICE_HPP

#include <cstdint>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/CommandMessages.hpp"

#include "Commands.hpp"

class Service
{
public:
    explicit Service(SDK::Kernel& kernel);

    /// The service's main loop; returning ends the whole app.
    void run();

private:
    void celebrate(CustomMessage::Moment moment);
    void vibrate(const SDK::Message::RequestVibroPlay::Effect* effects, uint8_t count, uint16_t gapMs);
    void backlightOn(uint32_t timeoutMs);

    SDK::Kernel& mKernel;
    bool         mGuiStarted = false;

    /// A normal launch starts the service a moment before its GUI; this long
    /// is allowed for the GUI to arrive before "no GUI" means "exit".
    static constexpr uint32_t skStartupGraceMs = 5000u;
    /// The loop's bounded wait. Nothing here is time-driven yet, so this only
    /// paces the exit check.
    static constexpr uint32_t skWaitMs = 1000u;
    static constexpr uint32_t skBacklightMs = 5000u;
};

#endif // STREAK_SERVICE_HPP
