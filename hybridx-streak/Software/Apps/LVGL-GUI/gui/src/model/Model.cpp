/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of HybridX Streak (see the header).
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#define LOG_MODULE_PRX   "Model"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

#if HYBRIDXSTREAK_DEMO
#include "gui/demo/Demo.hpp"
#endif

Model::Model()
    : mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(this);
#if HYBRIDXSTREAK_DEMO
    mHome = Demo::kScenarios[0].view;
#endif
}

void Model::handleKeyEvent(uint8_t key)
{
    if (isClick(key)) {
        resetIdleTimer();
    }
}

void Model::resetIdleTimer()
{
    mIdleTimer = kScreenTimeoutSteps;
}

void Model::exitApp()
{
    LOG_INFO("Exiting\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::setHome(const Streak::HomeView& v)
{
    mHome = v;
    if (mListener) {
        mListener->onHomeView();
    }
}

void Model::celebrate(CustomMessage::Moment moment)
{
    SDK::send_msg<CustomMessage::Celebrate>(mKernel, moment);
}

ScreenId Model::entryScreen() const
{
#if HYBRIDXSTREAK_DEMO
    return Demo::kScenarios[mDemoIndex].screen;
#else
    return ScreenId::Home;
#endif
}

#if HYBRIDXSTREAK_DEMO
ScreenId Model::demoGo(int delta)
{
    const int n = Demo::kScenarioCount;
    mDemoIndex  = static_cast<uint8_t>(((mDemoIndex + delta) % n + n) % n);
    LOG_INFO("Demo scenario %u: %s\n", static_cast<unsigned>(mDemoIndex), Demo::kScenarios[mDemoIndex].name);
    mHome = Demo::kScenarios[mDemoIndex].view;
    return Demo::kScenarios[mDemoIndex].screen;
}
#endif

// -- Lifecycle ----------------------------------------------------------------

void Model::onStart()
{
    mIsRunning = true;
    resetIdleTimer();
}

void Model::onFrame()
{
    if (mIsRunning && mIdleTimer > 0u) {
        if (--mIdleTimer == 0u && mListener) {
            mListener->onIdleTimeout();
        }
    }
}

void Model::onResume()
{
    mIsRunning = true;
    resetIdleTimer();
}

void Model::onSuspend()
{
    mIsRunning = false;
    if (mListener) {
        mListener->onSuspend();
    }
}

void Model::onStop()
{
    mIsRunning = false;
}

bool Model::isClick(uint8_t key)
{
    namespace Btn = SDK::GUI::Button;
    return key == Btn::L1 || key == Btn::L2 || key == Btn::R1 || key == Btn::R2;
}

// -- Messages from the service --------------------------------------------------

bool Model::customMessageHandler(SDK::MessageBase* message)
{
    switch (message->getType()) {
        case CustomMessage::HOME_VIEW:
#if !HYBRIDXSTREAK_DEMO
            setHome(static_cast<CustomMessage::HomeView*>(message)->view);
#endif
            break;
        default:
            break;
    }
    // The port responds to and releases every custom message itself.
    return true;
}
