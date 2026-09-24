/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   HybridX Streak's service process (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include "SDK/Messages/MessageGuard.hpp"

#define LOG_MODULE_PRX   "Service"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

using Effect = SDK::Message::RequestVibroPlay::Effect;

Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel)
{
}

void Service::run()
{
    LOG_INFO("Started\n");

    const uint32_t startMs = mKernel.sys.getTimeMs();

    while (true) {
        SDK::MessageBase* msg = nullptr;
        if (mKernel.comm.getMessage(msg, skWaitMs)) {
            switch (msg->getType()) {
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Stop requested\n");
                    mKernel.comm.releaseMessage(msg);
                    return;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                    mGuiStarted = true;
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                    mGuiStarted = false;
                    break;

                case CustomMessage::CELEBRATE:
                    celebrate(static_cast<CustomMessage::Celebrate*>(msg)->moment);
                    break;

                default:
                    // Unknown types are released and ignored, never treated
                    // as a clock (service-lifecycle.md 4.1).
                    break;
            }
            mKernel.comm.releaseMessage(msg);
        }

        // The GUI is the only reason to be here. Unsigned subtraction keeps the
        // check right across the millisecond clock's wrap.
        if (!mGuiStarted && mKernel.sys.getTimeMs() - startMs >= skStartupGraceMs) {
            LOG_INFO("No GUI, nothing to do: exiting\n");
            return;
        }
    }
}

void Service::celebrate(CustomMessage::Moment moment)
{
    // Effects from the DRV2605-style library the kernel exposes
    // (CommandMessages.hpp RequestVibroPlay::Effect). A tick for the everyday,
    // a double click for a week done, a double pulse for a summit.
    switch (moment) {
        case CustomMessage::Moment::SessionFound: {
            static constexpr Effect kFx[] = { Effect::SHARP_TICK_1_100 };
            vibrate(kFx, 1, 0);
            break;
        }
        case CustomMessage::Moment::StepUp: {
            static constexpr Effect kFx[] = { Effect::SHORT_DOUBLE_CLICK_STRONG_1_100 };
            vibrate(kFx, 1, 0);
            backlightOn(skBacklightMs);
            break;
        }
        case CustomMessage::Moment::Summit: {
            static constexpr Effect kFx[] = { Effect::PULSING_STRONG_1_100, Effect::PULSING_STRONG_1_100 };
            vibrate(kFx, 2, 250);
            backlightOn(skBacklightMs);
            break;
        }
        case CustomMessage::Moment::Shield: {
            static constexpr Effect kFx[] = { Effect::SOFT_BUMP_100 };
            vibrate(kFx, 1, 0);
            break;
        }
    }
}

void Service::vibrate(const Effect* effects, uint8_t count, uint16_t gapMs)
{
    // N effects need 2N-1 notes (effects and pauses share the note array).
    const uint8_t maxCount = (SDK::Message::RequestVibroPlay::skMaxNotes + 1u) / 2u;
    if (count > maxCount) {
        count = maxCount;
    }
    if (count == 0u) {
        return;
    }

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestVibroPlay>();
    if (!msg) {
        return;
    }
    uint8_t n = 0u;
    for (uint8_t i = 0u; i < count; ++i) {
        msg->notes[n].effect = static_cast<uint8_t>(effects[i]);
        msg->notes[n].pause  = 0;
        ++n;
        if (i + 1u < count) {
            msg->notes[n].effect = static_cast<uint8_t>(Effect::NO_EFFECT);
            msg->notes[n].pause  = gapMs;
            ++n;
        }
    }
    msg->notesCount = n;
    mKernel.comm.sendMessage(msg);
    mKernel.comm.releaseMessage(msg);
}

void Service::backlightOn(uint32_t timeoutMs)
{
    if (auto bl = SDK::make_msg<SDK::Message::RequestBacklightSet>(mKernel)) {
        bl->brightness       = 100;
        bl->autoOffTimeoutMs = timeoutMs;
        bl.send();
    }
}
