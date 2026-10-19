/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   HybridX Run's VO2max glance: a service-only app for the glances
 *          screen.
 *
 * Built as X Weigh-in's and HybridX Streak's glances, on the SDK's
 * GlanceSteps pattern (Docs/service-lifecycle.md 3.3): configure and draw on
 * EVENT_GLANCE_START, push the form on the tick when it has changed, and exit
 * on EVENT_GLANCE_STOP or COMMAND_APP_STOP, so every look reads afresh.
 *
 * It reads the Run app's shared file (../SharedData/HybridX/vo2max.json, the
 * one folder outside its own a glance can open) and NEVER writes: the app is
 * the single writer. GlanceView picks what to show and GlanceLayout how, for
 * the area and control budget the watch reports; the service only turns the
 * specs into SDK glance controls. It logs the area it was given, which the
 * PC simulator cannot tell us.
 ******************************************************************************
 */

#ifndef RUN_GLANCE_SERVICE_HPP
#define RUN_GLANCE_SERVICE_HPP

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

#endif // RUN_GLANCE_SERVICE_HPP
