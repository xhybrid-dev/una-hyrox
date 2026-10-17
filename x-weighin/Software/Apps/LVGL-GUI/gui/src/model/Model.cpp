/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of X Weigh-in and its link to the service.
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
    LOG_INFO("Keys: 1 = UP, 2 = DOWN, 3 = SELECT, 4 = BACK; Esc = close\n");
#endif
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::saveWeighIn(int32_t hundredths)
{
    LOG_INFO("asking the service to save %ld\n", static_cast<long>(hundredths));
    SDK::send_msg<CustomMessage::SaveWeighIn>(mKernel, hundredths);
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
        case CustomMessage::HOME_STATE: {
            auto* m  = static_cast<CustomMessage::HomeState*>(msg);
            mHome    = m->home;
            mHasHome = true;
            if (modelListener) {
                modelListener->onHome();
            }
        } break;

        case CustomMessage::GRAPH_DATA: {
            auto* m = static_cast<CustomMessage::GraphData*>(msg);
            mWindow = m->window;
            if (modelListener) {
                modelListener->onGraph();
            }
        } break;

        case CustomMessage::SAVE_RESULT: {
            auto* m      = static_cast<CustomMessage::SaveResult*>(msg);
            mLastOutcome = m->outcome;
            mLastSaved   = m->hundredths;
            LOG_INFO("save outcome: %d\n", static_cast<int>(m->outcome));
            if (modelListener) {
                modelListener->onSaveResult();
            }
        } break;

        default:
            break;
    }
    return true;
}
