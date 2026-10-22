/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of X Sauna and its link to the service.
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    // The port owns the kernel's callback slots and forwards to these.
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(this);

#if defined(SIMULATOR)
    LOG_INFO("Application is running through simulator!\n");
    LOG_INFO("Keys: 1 = L1, 2 = L2, 3 = R1, 4 = R2 (hold to stop a session), Esc = close\n");
#endif
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::startSession(uint8_t mode, uint8_t rest)
{
    SDK::send_msg<CustomMessage::SessionStart>(mKernel, mode, rest);
}

void Model::lap()
{
    SDK::send_msg<CustomMessage::SessionLap>(mKernel);
}

void Model::pause()
{
    SDK::send_msg<CustomMessage::SessionPause>(mKernel);
}

void Model::resume()
{
    SDK::send_msg<CustomMessage::SessionResume>(mKernel);
}

void Model::stop(bool save)
{
    SDK::send_msg<CustomMessage::SessionStop>(mKernel, static_cast<uint8_t>(save ? 1u : 0u));
}

void Model::onStart()
{
    LOG_INFO("called\n");
}

void Model::onResume()
{
    LOG_INFO("called\n");
}

void Model::onSuspend()
{
    LOG_INFO("called\n");
    // A hold cannot complete while the GUI is away: the release would never
    // reach the screen.
    if (modelListener) {
        modelListener->onSuspend();
    }
}

void Model::onStop()
{
    LOG_INFO("called\n");
}

// Events from the service
bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::SESSION_UPDATE: {
            auto* m = static_cast<CustomMessage::SessionUpdate*>(msg);
            mSnapshot = m->snapshot;
            if (modelListener) {
                modelListener->onSessionUpdate(mSnapshot);
            }
        } break;

        case CustomMessage::SESSION_SAVED: {
            auto* m = static_cast<CustomMessage::SessionSaved*>(msg);
            if (modelListener) {
                modelListener->onSessionSaved(m->saved);
            }
        } break;

        default:
            break;
    }
    return true;
}
