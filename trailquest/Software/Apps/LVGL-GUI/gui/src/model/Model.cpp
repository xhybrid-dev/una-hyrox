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

#include "lvgl.h"

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

uint16_t Model::next()
{
    mSentAt  = lv_tick_get();
    mCommand = ++mSeq;
    mHello   = 0;
    mLost    = false;
    return mCommand;
}

void Model::keepAlive(uint32_t nowMs)
{
    if (waiting() && nowMs - mSentAt >= kAnswerMs) {
        LOG_WARNING("no answer to command %u: asking again\n", static_cast<unsigned>(mCommand));
        mSentAt = nowMs;   // once every kAnswerMs, not every frame
        hello();
    }
}

void Model::hello()
{
    mHello = ++mSeq;
    SDK::send_msg<CustomMessage::Hello>(mKernel, mHello);
}

void Model::newGame()
{
    SDK::send_msg<CustomMessage::NewGame>(mKernel, next());
}

void Model::travel(uint8_t exitIndex)
{
    SDK::send_msg<CustomMessage::Travel>(mKernel, next(), exitIndex);
}

void Model::obstacleTry(TrailQuest::BeatResult beat)
{
    SDK::send_msg<CustomMessage::ObstacleTry>(mKernel, next(), beat);
}

void Model::duelTurn(TrailQuest::Action action, TrailQuest::ItemId item, TrailQuest::BeatResult beat)
{
    SDK::send_msg<CustomMessage::DuelTurn>(mKernel, next(), action, item, beat);
}

void Model::join(bool accept, uint8_t slot)
{
    SDK::send_msg<CustomMessage::Join>(mKernel, next(), accept, slot);
}

void Model::advance()
{
    SDK::send_msg<CustomMessage::Advance>(mKernel, next());
}

void Model::setBeat(bool on)
{
    mBeat = on;   // shown at once; the service saves it and echoes it back
    SDK::send_msg<CustomMessage::Settings>(mKernel, next(), on);
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
    if (modelListener) {
        modelListener->onSuspend();
    }
}

void Model::onStop()
{
    LOG_INFO("called\n");
}

// Snapshots from the service: copied into the Model before the screen hears
// of them (the message is released when this returns).
bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::WORLD: {
            const auto* m = static_cast<const CustomMessage::World*>(msg);
            mWorld    = m->view;
            mAck      = m->ack;
            mBeat     = m->beat != 0;
            mHasWorld = true;
            // The service answers in order: an answer to our Hello that does
            // not carry the command's number means the command never ran.
            if (mAck != mCommand && mHello != 0 && m->hello == mHello) {
                LOG_WARNING("command %u was lost\n", static_cast<unsigned>(mCommand));
                mLost = true;
            }
            if (modelListener) {
                modelListener->onWorld();
            }
        } break;

        case CustomMessage::DUEL: {
            mDuel = static_cast<const CustomMessage::Duel*>(msg)->view;
            if (modelListener) {
                modelListener->onDuel();
            }
        } break;

        default:
            break;
    }
    return true;
}
