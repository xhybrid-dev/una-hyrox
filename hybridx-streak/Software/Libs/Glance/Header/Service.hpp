/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   HybridX Streak's glance: a service-only app for the glances screen.
 *
 * Built on the SDK's GlanceSteps pattern (Docs/service-lifecycle.md 3.3):
 * configure and draw on EVENT_GLANCE_START, push the form on the tick when it
 * has changed, and exit on EVENT_GLANCE_STOP or COMMAND_APP_STOP.
 *
 * Phase S0 shows demonstration content (the S0 first look) plus one small
 * line reporting the glance area the watch actually gives an app: the PC
 * simulator cannot run glances, so this is how the real width, height and
 * control budget are measured (PLAN S0). Phase S4 replaces the content with
 * the real summary from ../SharedData/HybridX/streak.json.
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
