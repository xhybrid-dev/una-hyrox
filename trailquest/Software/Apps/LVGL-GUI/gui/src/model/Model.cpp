/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side link of Trail Quest to its service.
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
    LOG_INFO("Keys: 1 = L1, 2 = L2, 3 = R1, 4 = R2; Esc = close\n");
#endif
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::sendBeatPress(uint32_t seq, uint32_t startMs, uint32_t pressMs)
{
    SDK::send_msg<CustomMessage::BeatPress>(mKernel, seq, startMs, pressMs);
}

void Model::playFx(TrailQuest::FxId id)
{
    SDK::send_msg<CustomMessage::FxPlay>(mKernel, id);
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
}

void Model::onStop()
{
    LOG_INFO("called\n");
}

// Events from the service
bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::BEAT_RESULT: {
            auto* m = static_cast<CustomMessage::BeatResult*>(msg);
            if (modelListener) {
                modelListener->onBeatResult(*m);
            }
        } break;

        default:
            break;
    }
    return true;
}
