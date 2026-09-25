/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   HybridX Streak's glance: a service-only app for the glances screen.
 *
 * Built on the SDK's GlanceSteps pattern (Docs/service-lifecycle.md 3.3):
 * configure and draw on EVENT_GLANCE_START, push the form on the tick when it
 * has changed, and exit on EVENT_GLANCE_STOP or COMMAND_APP_STOP.
 *
 * On start it reads the public ../SharedData/HybridX/streak.json the app
 * saves, runs the same scanner and model over at most kMaxNew activities newer
 * than it, and projects the week to now -- WITHOUT saving: the app is the
 * single writer (PLAN 4, 8). GlanceLayout decides what to draw for the area
 * and control budget the watch reports; the service only turns its specs
 * into SDK glance controls. It logs the area it was given, which the PC
 * simulator cannot tell us (NOTES S0).
 ******************************************************************************
 */

#ifndef STREAK_GLANCE_SERVICE_HPP
#define STREAK_GLANCE_SERVICE_HPP

#include <cstdint>

#include "SDK/Glance/GlanceControl.hpp"
#include "SDK/Kernel/Kernel.hpp"

class Service
{
public:
    explicit Service(SDK::Kernel& kernel);

    /// Activities the glance reads per look: the app does the full catch-up.
    static constexpr size_t kMaxNew = 4;

    void run();

private:
    bool configure();
    void build();
    void pushIfChanged();


    const SDK::Kernel& mKernel;
    SDK::Glance::Form  mForm;
    bool               mBuilt       = false;
    int16_t            mWidth       = 0;
    int16_t            mHeight      = 0;
    uint32_t           mMaxControls = 0;
};

#endif // STREAK_GLANCE_SERVICE_HPP
