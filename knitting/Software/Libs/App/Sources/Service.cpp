/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   Knitting background logic: keep the counter, answer the GUI.
 ******************************************************************************
 */

#include "Service.hpp"

#include "StateStore.hpp"

#include "SDK/Messages/CommandMessages.hpp"
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

    // Pick up where the last run left off. Done here, not in the constructor:
    // the file system's first use may log, and in the simulator the service
    // is constructed before the logger's HAL exists.
    const Knitting::StateStore::Loaded loaded = Knitting::StateStore::load(mKernel.fs);
    mCounter = Knitting::RowCounter(loaded.state);
    switch (loaded.source) {
    case Knitting::StateStore::Source::File:
        LOG_INFO("resumed: rows %lu, set %lu, total %lu\n",
                 static_cast<unsigned long>(loaded.state.rows),
                 static_cast<unsigned long>(loaded.state.set),
                 static_cast<unsigned long>(loaded.state.total));
        break;
    case Knitting::StateStore::Source::Backup:
        LOG_WARNING("state file unusable, resumed from the backup\n");
        break;
    case Knitting::StateStore::Source::Fresh:
        LOG_INFO("no saved state, starting fresh\n");
        break;
    }

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
                // nothing else will reclaim it. Nothing is lost by leaving: every
                // change was saved when it was made, and the next start reads it.
                mKernel.comm.releaseMessage(msg);
                return;

            case CustomMessage::COUNT_ROW:
                if (mCounter.countRow()) {
                    sendUpdate();
                    saveState();
                }
                break;

            case CustomMessage::NEXT_SET:
                if (mCounter.nextSet()) {
                    sendUpdate();
                    saveState();
                }
                break;

            case CustomMessage::UNDO_ROW:
                if (mCounter.undoRow()) {
                    sendUpdate();
                    saveState();
                    buzz(SDK::Message::RequestVibroPlay::STRONG_CLICK_100);
                }
                break;

            case CustomMessage::RESET_ALL:
                // Sent only when the hold on L2 completes (the screen's work).
                if (mCounter.reset()) {
                    sendUpdate();
                    saveState();
                    buzz(SDK::Message::RequestVibroPlay::SHORT_DOUBLE_CLICK_STRONG_1_100);
                }
                break;

            default:
                break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }
    }
}

void Service::saveState()
{
    if (!Knitting::StateStore::save(mKernel.fs, mCounter.state())) {
        // The screen still shows the live count; the next change tries again.
        LOG_WARNING("could not save the count\n");
    }
}

void Service::buzz(uint8_t effect)
{
    auto vibro = SDK::make_msg<SDK::Message::RequestVibroPlay>(mKernel);
    if (!vibro) {
        return;
    }
    vibro->notes[0].effect = effect;
    vibro->notesCount = 1;
    vibro.send();
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
