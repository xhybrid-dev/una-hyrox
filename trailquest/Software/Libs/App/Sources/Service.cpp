/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   Trail Quest background logic (see Service.hpp).
 *
 * The lifecycle is X Weigh-in's, from the SDK's Waypoint tutorial: run until
 * the app is stopped or its GUI closes. A service the GUI no longer needs
 * would otherwise run for ever (una-sdk Docs/service-lifecycle.md). Nothing
 * is lost by leaving: the game saves after every step that ends on the map.
 ******************************************************************************
 */

#include "Service.hpp"

#include "SDK/Messages/MessageGuard.hpp"

#include <cstdlib>

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace {

/// How long the screen is asked to stay on after the last sign of life, ms.
constexpr uint32_t kBacklightHoldMs = 30000;

// Feedback.hpp is pure, so it copies the kernel's limits and effect numbers.
// Tie the copies to the SDK here, where both are visible: if the SDK changes,
// the build fails instead of a pattern being cut short on the watch.
using SDK::Message::RequestBuzzerPlay;
using SDK::Message::RequestVibroPlay;
namespace FxDetail = TrailQuest::detail;
static_assert(TrailQuest::kMaxVibroNotes == RequestVibroPlay::skMaxNotes, "vibro note limit changed");
static_assert(TrailQuest::kMaxBuzzerNotes == RequestBuzzerPlay::skMaxNotes, "buzzer note limit changed");
static_assert(FxDetail::kStrongClick == RequestVibroPlay::STRONG_CLICK_100, "effect renumbered");
static_assert(FxDetail::kSharpClick == RequestVibroPlay::SHARP_CLICK_100, "effect renumbered");
static_assert(FxDetail::kSoftBump == RequestVibroPlay::SOFT_BUMP_100, "effect renumbered");
static_assert(FxDetail::kDoubleClick == RequestVibroPlay::DOUBLE_CLICK_100, "effect renumbered");
static_assert(FxDetail::kAlert750 == RequestVibroPlay::ALERT_750MS_100, "effect renumbered");
static_assert(FxDetail::kShortDoubleStrong == RequestVibroPlay::SHORT_DOUBLE_CLICK_STRONG_1_100, "effect renumbered");
static_assert(FxDetail::kLongDoubleSharpStrong == RequestVibroPlay::LONG_DOUBLE_SHARP_CLICK_STRONG_1_100, "effect renumbered");

} // namespace

Service::Service(SDK::Kernel &kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
{
    (void)kernel;
}

uint32_t Service::entropy() const
{
#ifdef SIMULATOR
    if (const char *fixed = std::getenv("TRAILQUEST_SEED")) {
        return static_cast<uint32_t>(std::strtoul(fixed, nullptr, 10));
    }
#endif
    return mKernel.sys.getTimeMs();
}

void Service::startUp()
{
    // Done here, not in the constructor: the file system's first use may log,
    // and in the simulator the service is constructed before the logger exists.
    mSettings = TrailQuest::SaveStore::loadSettings(mKernel.fs);
    const TrailQuest::SaveStore::Loaded loaded = TrailQuest::SaveStore::load(mKernel.fs);
    switch (loaded.source) {
    case TrailQuest::SaveStore::Source::File:
        LOG_INFO("resumed: stage %u, place %u, level %u\n", static_cast<unsigned>(loaded.state.stage),
                 static_cast<unsigned>(loaded.state.node), static_cast<unsigned>(loaded.state.level));
        break;
    case TrailQuest::SaveStore::Source::Backup:
        LOG_WARNING("save unusable, resumed from the backup\n");
        break;
    case TrailQuest::SaveStore::Source::None:
        LOG_INFO("no saved game\n");
        break;
    }
    // Stirred, so coming back after quitting mid-duel meets a fresh spirit.
    mSession.begin(loaded.source == TrailQuest::SaveStore::Source::None ? nullptr : &loaded.state, entropy() | 1u);
}

void Service::run()
{
    LOG_INFO("thread started\n");
    startUp();

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
                sendViews();
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped, exiting service\n");
                // The kernel does not stop a service when its GUI closes, and
                // nothing else will ever reclaim the thread.
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;

            case CustomMessage::FX_PLAY: {
                const uint8_t fx = static_cast<CustomMessage::FxPlay *>(msg)->fx;
                if (fx < static_cast<uint8_t>(TrailQuest::FxId::Count)) {
                    playFx(static_cast<TrailQuest::FxId>(fx));
                }
                holdBacklight(kBacklightHoldMs);
            } break;

            default:
                if (onCommand(*msg)) {
                    holdBacklight(kBacklightHoldMs);
                    sendViews();
                    saveIfAsked();
                }
                break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }
    }

    LOG_INFO("thread stopped\n");
}

bool Service::onCommand(const SDK::MessageBase &msg)
{
    using namespace CustomMessage;
    const SDK::MessageType::Type type = msg.getType();
    if (type < HELLO || type > SETTINGS) {
        return false;
    }
    const uint16_t seq = static_cast<const Command &>(msg).seq;
    if (type == HELLO) {
        mHello = seq;   // the game is untouched: the World says where it stands
        return true;
    }
    mAck = seq;
    switch (type) {
    case NEW_GAME:
        mSession.newGame(entropy());
        LOG_INFO("new game\n");
        break;
    case TRAVEL:
        mSession.travel(static_cast<const Travel &>(msg).exit);
        break;
    case OBSTACLE_TRY:
        mSession.obstacle(static_cast<TrailQuest::BeatResult>(static_cast<const ObstacleTry &>(msg).beat));
        break;
    case DUEL_TURN: {
        const auto &t = static_cast<const DuelTurn &>(msg);
        TrailQuest::PlayerTurn turn;
        turn.action = static_cast<TrailQuest::Action>(t.action);
        turn.item   = t.item;
        turn.beat   = static_cast<TrailQuest::BeatResult>(t.beat);
        turn.skill  = t.skill;
        mSession.duelTurn(turn);
    } break;
    case JOIN: {
        const auto &j = static_cast<const Join &>(msg);
        mSession.join(j.accept != 0, j.slot);
    } break;
    case ADVANCE:
        mSession.advance();
        break;
    case SETTINGS:
        mSettings.beat = static_cast<const Settings &>(msg).beat != 0;
        if (!TrailQuest::SaveStore::saveSettings(mKernel.fs, mSettings)) {
            LOG_WARNING("settings not saved\n");
        }
        break;
    default:
        return false;
    }
    const TrailQuest::WorldView w = mSession.world();
    if (w.cause == TrailQuest::ViewCause::Refused) {
        LOG_WARNING("command 0x%lx refused in phase %u\n", static_cast<unsigned long>(type),
                    static_cast<unsigned>(w.phase));
    } else {
        // One line a step, for the simulator scripts (docs/experiments).
        LOG_INFO("step: cause %u phase %u stage %u place %u level %u stamina %u won %u lost %u\n",
                 static_cast<unsigned>(w.cause), static_cast<unsigned>(w.phase), static_cast<unsigned>(w.state.stage),
                 static_cast<unsigned>(w.state.node), static_cast<unsigned>(w.state.level),
                 static_cast<unsigned>(w.state.stamina), static_cast<unsigned>(w.state.duelsWon),
                 static_cast<unsigned>(w.state.duelsLost));
    }
    return true;
}

void Service::sendViews()
{
    if (mSession.showsDuel()) {
        if (!SDK::send_msg<CustomMessage::Duel>(mKernel, mSession.duel())) {
            LOG_WARNING("duel view not sent\n");
        }
    }
    if (!SDK::send_msg<CustomMessage::World>(mKernel, mSession.world(), mAck, mHello, mSettings.beat)) {
        LOG_WARNING("world view not sent\n");
    }
}

void Service::saveIfAsked()
{
    if (!mSession.wantsSave()) {
        return;
    }
    if (TrailQuest::SaveStore::save(mKernel.fs, mSession.state())) {
        mSession.saved();
        LOG_DEBUG("saved: stage %u, place %u\n", static_cast<unsigned>(mSession.state().stage),
                  static_cast<unsigned>(mSession.state().node));
    } else {
        // Left asking: the next step tries again. The previous save stands.
        LOG_WARNING("save failed\n");
    }
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
