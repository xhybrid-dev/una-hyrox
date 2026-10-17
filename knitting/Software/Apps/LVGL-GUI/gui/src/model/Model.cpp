/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of Knitting and its link to the service.
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
    LOG_INFO("Keys: 1 = L1 (next set), 2 = L2 (reset), 3 = R1 (count row), 4 = R2 (exit), Esc = close\n");
#endif
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::countRow()
{
    SDK::send_msg<CustomMessage::CountRow>(mKernel);
}

void Model::nextSet()
{
    SDK::send_msg<CustomMessage::NextSet>(mKernel);
}

void Model::undoRow()
{
    SDK::send_msg<CustomMessage::UndoRow>(mKernel);
}

void Model::resetAll()
{
    SDK::send_msg<CustomMessage::ResetAll>(mKernel);
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
        case CustomMessage::COUNTER_UPDATE: {
            auto* m = static_cast<CustomMessage::CounterUpdate*>(msg);
            mCounter.rows  = m->rows;
            mCounter.set   = m->set;
            mCounter.total = m->total;
            if (modelListener) {
                modelListener->onCounterUpdate(mCounter);
            }
        } break;

        default:
            break;
    }
    return true;
}
