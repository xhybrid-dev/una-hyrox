/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of HybridX Streak and its link to the service.
 *
 * The HybridX Race Model's shape (itself from RunLVGL): registered with the
 * SDK's LVGL port for lifecycle and custom messages; screens are plain
 * ModelListeners. It holds the home view the service sends, and asks the
 * service to mark moments with a buzz (only a service may).
 *
 * In HYBRIDXSTREAK_DEMO builds the home view comes from Demo::kScenarios
 * instead, and screens can play a scenario's moment forward locally.
 ******************************************************************************
 */

#ifndef STREAK_MODEL_HPP
#define STREAK_MODEL_HPP

#include <cstdint>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Config.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Utils/Utils.hpp"

#include "Commands.hpp"
#include "StreakView.hpp"
#include "gui/screens/ScreenManager.hpp"

class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    static constexpr uint32_t kFrameRate          = SDK::GUI::Config::kFrameRate;
    static constexpr uint32_t kScreenTimeoutSteps = SDK::Utils::secToTicks(30, kFrameRate);

    void bind(ModelListener* listener) { mListener = listener; }

    void handleKeyEvent(uint8_t key);
    void resetIdleTimer();
    void exitApp();

    const Streak::HomeView& home() const { return mHome; }

    /// Change the view locally (the demo, and a screen playing a moment
    /// forward) and tell the bound screen.
    void setHome(const Streak::HomeView& v);

    /// Ask the service to mark a moment (vibration, backlight).
    void celebrate(CustomMessage::Moment moment);

    /// The climb just summited, for the summit screen.
    void setSummited(uint8_t climb) { mSummited = climb; }
    uint8_t summited() const { return mSummited; }

    /// The screen the current state belongs on.
    ScreenId entryScreen() const;

#if HYBRIDXSTREAK_DEMO
    /// Step to another demo scenario; returns the screen it belongs on.
    ScreenId demoGo(int delta);
    uint8_t demoIndex() const { return mDemoIndex; }
#endif

private:
    // IGuiLifeCycleCallback
    void onStart() override;
    void onFrame() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;

    // ICustomMessageHandler
    bool customMessageHandler(SDK::MessageBase* message) override;

    static bool isClick(uint8_t key);

    ModelListener*     mListener = nullptr;
    const SDK::Kernel& mKernel;

    bool     mIsRunning = false;
    uint32_t mIdleTimer = 0;

    Streak::HomeView mHome {};
    uint8_t          mSummited = 0;

#if HYBRIDXSTREAK_DEMO
    uint8_t mDemoIndex = 0;
#endif
};

#endif // STREAK_MODEL_HPP
