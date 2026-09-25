/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of HybridX Streak (see the header).
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#define LOG_MODULE_PRX   "Model"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

#if HYBRIDXSTREAK_DEMO
#include "gui/demo/Demo.hpp"
#endif

Model::Model()
    : mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(this);
#if HYBRIDXSTREAK_DEMO
    mHome = Demo::kScenarios[0].view;
#endif
}

void Model::handleKeyEvent(uint8_t key)
{
    if (isClick(key)) {
        resetIdleTimer();
    }
}

void Model::resetIdleTimer()
{
    mIdleTimer = kScreenTimeoutSteps;
}

void Model::exitApp()
{
    LOG_INFO("Exiting\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::setHome(const Streak::HomeView& v)
{
    mHome = v;
    if (mListener) {
        mListener->onHomeView();
    }
}

void Model::celebrate(CustomMessage::Moment moment)
{
    SDK::send_msg<CustomMessage::Celebrate>(mKernel, moment);
}

void Model::logManual(Streak::Kind kind, bool yesterday)
{
    SDK::send_msg<CustomMessage::LogManual>(mKernel, static_cast<uint8_t>(kind), yesterday);
}

void Model::weekAction(uint8_t action, uint8_t index)
{
    SDK::send_msg<CustomMessage::WeekAction>(mKernel, action, index);
}

void Model::decideShields(bool use)
{
    // Letting the streak go shows Fresh start at once; the service's own
    // StreakReset moment for it is then not played a second time.
    mDeclined = !use;
    SDK::send_msg<CustomMessage::ShieldDecision>(mKernel, use);
}

void Model::setGoal(const Streak::Goal& goal)
{
    SDK::send_msg<CustomMessage::SetGoal>(mKernel, goal);
}

ScreenId Model::entryScreen() const
{
#if HYBRIDXSTREAK_DEMO
    return Demo::kScenarios[mDemoIndex].screen;
#else
    return ScreenId::Home;
#endif
}

#if HYBRIDXSTREAK_DEMO
ScreenId Model::demoGo(int delta)
{
    const int n = Demo::kScenarioCount;
    mDemoIndex  = static_cast<uint8_t>(((mDemoIndex + delta) % n + n) % n);
    LOG_INFO("Demo scenario %u: %s\n", static_cast<unsigned>(mDemoIndex), Demo::kScenarios[mDemoIndex].name);
    mHome = Demo::kScenarios[mDemoIndex].view;
    return Demo::kScenarios[mDemoIndex].screen;
}
#endif

// -- Lifecycle ----------------------------------------------------------------

void Model::onStart()
{
    mIsRunning = true;
    resetIdleTimer();
}

void Model::onFrame()
{
    if (mIsRunning && mIdleTimer > 0u) {
        if (--mIdleTimer == 0u && mListener) {
            mListener->onIdleTimeout();
        }
    }
}

void Model::onResume()
{
    mIsRunning = true;
    resetIdleTimer();
}

void Model::onSuspend()
{
    mIsRunning = false;
    if (mListener) {
        mListener->onSuspend();
    }
}

void Model::onStop()
{
    mIsRunning = false;
}

bool Model::isClick(uint8_t key)
{
    namespace Btn = SDK::GUI::Button;
    return key == Btn::L1 || key == Btn::L2 || key == Btn::R1 || key == Btn::R2;
}

// -- Messages from the service --------------------------------------------------

bool Model::customMessageHandler(SDK::MessageBase* message)
{
#if HYBRIDXSTREAK_DEMO
    // The demo shows canned scenarios; the service's real views are ignored.
    (void)message;
#else
    switch (message->getType()) {
        case CustomMessage::HOME_VIEW:
            setHome(static_cast<CustomMessage::HomeView*>(message)->view);
            break;

        case CustomMessage::MOMENTS: {
            // Append to what is still unplayed; drop what has been played.
            const Streak::Events& in = static_cast<CustomMessage::Moments*>(message)->events;
            Streak::Events        q;
            for (uint8_t i = mMomentAt; i < mMoments.count; ++i) {
                q.add(mMoments.items[i].kind, mMoments.items[i].a, mMoments.items[i].b);
            }
            for (uint8_t i = 0; i < in.count; ++i) {
                if (in.items[i].kind == Streak::EventKind::StreakReset && mDeclined) {
                    mDeclined = false;   // already shown (decideShields)
                    continue;
                }
                q.add(in.items[i].kind, in.items[i].a, in.items[i].b);
            }
            mMoments  = q;
            mMomentAt = 0;
            if (mListener) {
                mListener->onMoments();
            }
            break;
        }

        case CustomMessage::WEEK_LIST:
            mWeek = static_cast<CustomMessage::WeekList*>(message)->data;
            if (mListener) {
                mListener->onWeek();
            }
            break;

        case CustomMessage::APP_NAMES:
            mApps = static_cast<CustomMessage::AppNames*>(message)->data;
            break;

        case CustomMessage::TROPHIES:
            mTrophies = static_cast<CustomMessage::Trophies*>(message)->data;
            if (mListener) {
                mListener->onTrophies();
            }
            break;

        case CustomMessage::GOAL_VIEW:
            mGoal = static_cast<CustomMessage::GoalView*>(message)->data;
            if (mListener) {
                mListener->onGoal();
            }
            break;

        default:
            break;
    }
#endif
    // The port responds to and releases every custom message itself.
    return true;
}
