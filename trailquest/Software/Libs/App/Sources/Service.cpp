/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   Trail Quest background logic (see Service.hpp).
 *
 * The lifecycle is X Weigh-in's, from the SDK's Waypoint tutorial: run until
 * the app is stopped or its GUI closes. A service the GUI no longer needs
 * would otherwise run for ever (una-sdk Docs/service-lifecycle.md).
 ******************************************************************************
 */

#include "Service.hpp"

#include "SDK/Messages/MessageGuard.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace {

/// How long the screen is asked to stay on after the last sign of life, ms.
constexpr uint32_t kBacklightHoldMs = 30000;

} // namespace

Service::Service(SDK::Kernel &kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mBeat(TrailQuest::clampBeat(TrailQuest::BeatConfig{}))
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
                holdBacklight(kBacklightHoldMs);
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped, exiting service\n");
                // The kernel does not stop a service when its GUI closes, and
                // nothing else will ever reclaim the thread.
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;

            case CustomMessage::BEAT_PRESS:
                onBeatPress(*static_cast<CustomMessage::BeatPress *>(msg));
                break;

            case CustomMessage::FX_PLAY: {
                const uint8_t fx = static_cast<CustomMessage::FxPlay *>(msg)->fx;
                if (fx < static_cast<uint8_t>(TrailQuest::FxId::Count)) {
                    playFx(static_cast<TrailQuest::FxId>(fx));
                }
            } break;

            default:
                break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }
    }

    LOG_INFO("thread stopped\n");
}

void Service::holdBacklight(uint32_t timeoutMs)
{
    auto bl = SDK::make_msg<SDK::Message::RequestBacklightSet>(mKernel);
    if (bl) {
        bl->brightness       = 100;
        bl->autoOffTimeoutMs = timeoutMs;
        bl.send();
    }
}

void Service::playFx(TrailQuest::FxId id)
{
    const TrailQuest::FxPattern &p = TrailQuest::fxPattern(id);

    auto vibro = SDK::make_msg<SDK::Message::RequestVibroPlay>(mKernel);
    if (vibro) {
        uint32_t n = 0;
        for (uint8_t i = 0; i < p.vibroCount && n < SDK::Message::RequestVibroPlay::skMaxNotes; ++i, ++n) {
            vibro->notes[n].effect = p.vibro[i].effect;
            vibro->notes[n].pause  = p.vibro[i].pauseMs;
        }
        vibro->notesCount = n;
        vibro.send();
    }

    auto buzzer = SDK::make_msg<SDK::Message::RequestBuzzerPlay>(mKernel);
    if (buzzer) {
        uint32_t n = 0;
        for (uint8_t i = 0; i < p.buzzerCount && n < SDK::Message::RequestBuzzerPlay::skMaxNotes; ++i, ++n) {
            buzzer->notes[n].time   = p.buzzer[i].ms;
            buzzer->notes[n].volume = p.buzzer[i].volume;
        }
        buzzer->notesCount = n;
        buzzer.send();
    }
}

void Service::onBeatPress(const CustomMessage::BeatPress &press)
{
    const TrailQuest::BeatJudgement j = TrailQuest::judgeBeat(mBeat, press.startMs, press.pressMs);

    LOG_INFO("beat %lu: %s, %ld ms\n", static_cast<unsigned long>(press.seq),
             j.result == TrailQuest::BeatResult::Perfect ? "perfect"
             : j.result == TrailQuest::BeatResult::Good  ? "good"
                                                          : "miss",
             static_cast<long>(j.offsetMs));

    SDK::send_msg<CustomMessage::BeatResult>(mKernel, press.seq, j.offsetMs, j.result);

    playFx(j.result == TrailQuest::BeatResult::Perfect ? TrailQuest::FxId::Perfect
           : j.result == TrailQuest::BeatResult::Good  ? TrailQuest::FxId::Hit
                                                        : TrailQuest::FxId::Miss);
    holdBacklight(kBacklightHoldMs);
}
