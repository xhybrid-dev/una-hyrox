/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of X Sauna and its link to the service.
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#include "SDK/GUI/Button.hpp"
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
    LOG_INFO("Keys: 1 = L1, 2 = L2, 3 = R1, 4 = R2 (hold R1 to confirm), Esc = close\n");
#endif
}

void Model::handleKeyEvent(uint8_t key)
{
    namespace Btn = SDK::GUI::Button;
    if (key == Btn::L1 || key == Btn::L2 || key == Btn::R1 || key == Btn::R2) {
        mIdleTimer = mIdleSteps;
    }
}

void Model::setIdleTimeout(uint32_t steps)
{
    mIdleSteps = steps;
    mIdleTimer = steps;
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

bool Model::sessionActive() const
{
    return mSnapshot.state == static_cast<uint8_t>(Sauna::RunState::Running) ||
           mSnapshot.state == static_cast<uint8_t>(Sauna::RunState::Paused);
}

bool Model::paused() const
{
    return mSnapshot.state == static_cast<uint8_t>(Sauna::RunState::Paused);
}

void Model::startSession()
{
    mLapLockout.reset();
    mHasSaved   = false;
    mDiscarded  = false;
    mSaved      = Sauna::SavedSession();
    SDK::send_msg<CustomMessage::SessionStart>(mKernel, mMode, static_cast<uint8_t>(mRest ? 1u : 0u));
}

bool Model::lap()
{
    if (!mLapLockout.allow(mKernel.sys.getTimeMs())) {
        return false;
    }
    SDK::send_msg<CustomMessage::SessionLap>(mKernel);
    return true;
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
    mHasSaved  = false;
    mSaved     = Sauna::SavedSession();
    mDiscarded = !save;
    SDK::send_msg<CustomMessage::SessionStop>(mKernel, static_cast<uint8_t>(save ? 1u : 0u));
}

void Model::requestLaps(uint8_t first)
{
    SDK::send_msg<CustomMessage::SessionLapsRequest>(mKernel, first);
}

void Model::onStart()
{
    LOG_INFO("called\n");
}

void Model::onResume()
{
    mIsRunning = true;
    mIdleTimer = mIdleSteps;
}

void Model::onSuspend()
{
    mIsRunning = false;
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

void Model::onFrame()
{
    if (mIsRunning && mIdleTimer > 0u) {
        if (--mIdleTimer == 0u && modelListener) {
            modelListener->onIdleTimeout();
        }
    }
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

        case CustomMessage::SESSION_LAP_EVENT: {
            auto* m = static_cast<CustomMessage::SessionLapEvent*>(msg);
            mLastLap = m->event;
            if (modelListener) {
                modelListener->onLapEvent(mLastLap);
            }
        } break;

        case CustomMessage::SESSION_SAVED: {
            auto* m = static_cast<CustomMessage::SessionSaved*>(msg);
            mSaved    = m->saved;
            mHasSaved = true;
            if (modelListener) {
                modelListener->onSessionSaved(mSaved);
            }
        } break;

        case CustomMessage::SESSION_LAPS_PAGE: {
            auto* m = static_cast<CustomMessage::SessionLapsPage*>(msg);
            mLapsPage = m->page;
            if (modelListener) {
                modelListener->onLapsPage(mLapsPage);
            }
        } break;

        default:
            break;
    }
    return true;
}

#if defined(SIMULATOR)
void Model::injectSnapshot(const Sauna::SessionSnapshot& s)
{
    mSnapshot = s;
    if (modelListener) {
        modelListener->onSessionUpdate(mSnapshot);
    }
}

void Model::injectLapEvent(const Sauna::LapEventData& e)
{
    mLastLap = e;
    if (modelListener) {
        modelListener->onLapEvent(mLastLap);
    }
}

void Model::injectSaved(const Sauna::SavedSession& s)
{
    mSaved    = s;
    mHasSaved = true;
    if (modelListener) {
        modelListener->onSessionSaved(mSaved);
    }
}

void Model::injectLapsPage(const Sauna::LapsPageData& p)
{
    mLapsPage = p;
    if (modelListener) {
        modelListener->onLapsPage(mLapsPage);
    }
}
#endif
