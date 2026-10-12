/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   X Weigh-in's glance: a service-only app for the glances screen.
 *
 * Built on HybridX Streak's glance and the SDK's GlanceSteps pattern
 * (Docs/service-lifecycle.md 3.3): configure and draw on EVENT_GLANCE_START,
 * push the form on the tick when it has changed, and exit on
 * EVENT_GLANCE_STOP or COMMAND_APP_STOP, so every look reads afresh.
 *
 * It reads the log's SharedData copy (../SharedData/HybridX/weighins.csv, the
 * one file outside its own folder a glance can open) into the same Trend the
 * app uses, and NEVER writes: the app is the single writer. GlanceView picks
 * what to show and GlanceLayout how, for the area and control budget the
 * watch reports; the service only turns the specs into SDK glance controls.
 * It logs the area it was given, which the PC simulator cannot tell us.
 ******************************************************************************
 */

#ifndef XWEIGHIN_GLANCE_SERVICE_HPP
#define XWEIGHIN_GLANCE_SERVICE_HPP

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

#endif // XWEIGHIN_GLANCE_SERVICE_HPP
