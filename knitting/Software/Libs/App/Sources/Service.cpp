/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   Knitting background logic: keep the counter, answer the GUI.
 ******************************************************************************
 */

#include "Service.hpp"

#include "SDK/Messages/MessageGuard.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

Service::Service(SDK::Kernel &kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mCounter()
{
    (void)kernel;
}

void Service::run()
{
    LOG_INFO("thread started\n");

    while (true) {
        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, 1000)) {
            switch (msg->getType()) {
            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
                sendUpdate();
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped, exiting service\n");
                // The kernel does not stop a service when its GUI closes, and
                // nothing else will reclaim it. K0 keeps nothing worth staying
                // alive for: the count is lost on exit until K2 saves it.
                mKernel.comm.releaseMessage(msg);
                return;

            case CustomMessage::COUNT_ROW:
                mCounter.countRow();
                sendUpdate();
                break;

            case CustomMessage::NEXT_SET:
                mCounter.nextSet();
                sendUpdate();
                break;

            case CustomMessage::RESET_ALL:
                mCounter.reset();
                sendUpdate();
                break;

            default:
                break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }
    }
}

void Service::sendUpdate()
{
    auto update = SDK::make_msg<CustomMessage::CounterUpdate>(mKernel);
    if (!update) {
        return;
    }

    const Knitting::CounterState &s = mCounter.state();
    update->rows = s.rows;
    update->set = s.set;
    update->total = s.total;
    update.send();
}
