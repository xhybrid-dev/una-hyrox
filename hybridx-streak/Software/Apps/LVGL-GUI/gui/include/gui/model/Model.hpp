/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of HybridX Streak and its link to the service.
 *
 * The HybridX Race Model's shape (itself from RunLVGL): registered with the
 * SDK's LVGL port for lifecycle and custom messages; screens are plain
 * ModelListeners. It keeps the latest of every view the service sends (home,
 * this week, app names, trophies, goal) and a queue of moments for the home
 * screen to play, and it carries the athlete's commands back to the service.
 * The service owns the rules; the GUI only shows them.
 *
 * In HYBRIDXSTREAK_DEMO builds the home view comes from Demo::kScenarios
 * instead, and screens can play a scenario's moment forward locally.
 ******************************************************************************
 */

#ifndef STREAK_GUI_MODEL_HPP
#define STREAK_GUI_MODEL_HPP

#include <cstdint>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Config.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Utils/Utils.hpp"

#include "Commands.hpp"
#include "Goal.hpp"
#include "StreakEvents.hpp"
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

    // -- What the service sent ----------------------------------------------------
    const Streak::HomeView&         home() const { return mHome; }
    const CustomMessage::WeekData&     week() const { return mWeek; }
    const CustomMessage::AppNamesData& apps() const { return mApps; }
    const CustomMessage::TrophyData&   trophies() const { return mTrophies; }
    const CustomMessage::GoalData&     goal() const { return mGoal; }
    bool clockUnset() const { return (mHome.flags & Streak::HomeView::kClockUnset) != 0; }

    /// Change the view locally (the demo, and a screen playing a moment
    /// forward) and tell the bound screen.
    void setHome(const Streak::HomeView& v);

    // -- Moments to play (DESIGN 6) -------------------------------------------------
    bool hasMoment() const { return mMomentAt < mMoments.count; }
    const Streak::Event& peekMoment() const { return mMoments.items[mMomentAt]; }
    Streak::Event popMoment() { return mMoments.items[mMomentAt++]; }
    /// The moments still to play, from the next one on (for working out the
    /// view as it was before them).
    const Streak::Events& moments() const { return mMoments; }
    uint8_t momentAt() const { return mMomentAt; }

    /// The shield offer being played, and the streak a reset ended.
    void setOffer(const Streak::Event& e) { mOffer = e; }
    const Streak::Event& offer() const { return mOffer; }
    void setLostStreak(uint16_t weeks) { mLostStreak = weeks; }
    uint16_t lostStreak() const { return mLostStreak; }

    // -- Commands to the service ------------------------------------------------------
    /// Ask the service to mark a moment (vibration, backlight).
    void celebrate(CustomMessage::Moment moment);
    void logManual(Streak::Kind kind, bool yesterday);
    void weekAction(uint8_t action, uint8_t index);
    void decideShields(bool use);
    void setGoal(const Streak::Goal& goal);

    // -- Navigation memory ----------------------------------------------------------------
    /// The climb just summited, for the summit screen.
    void setSummited(uint8_t climb) { mSummited = climb; }
    uint8_t summited() const { return mSummited; }

    /// Menu positions to come back to.
    uint16_t menuAt     = 0;
    uint16_t weekAt     = 0;
    uint16_t settingsAt = 0;
    uint16_t trophyAt   = 0;

    /// The week item a confirm screen acts on, and the setting a value
    /// screen edits.
    uint8_t confirmIndex = 0;
    uint8_t editField    = 0;

    /// Log a session: the kind chosen, and whether "today or yesterday?" is
    /// the question now (the screen is rebuilt for each step).
    uint8_t logKind = 0;
    bool    logWhen = false;

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

    Streak::HomeView         mHome {};
    CustomMessage::WeekData     mWeek {};
    CustomMessage::AppNamesData mApps {};
    CustomMessage::TrophyData   mTrophies {};
    CustomMessage::GoalData     mGoal {};
    Streak::Events           mMoments {};
    uint8_t                  mMomentAt   = 0;
    Streak::Event            mOffer {};
    uint16_t                 mLostStreak = 0;
    bool                     mDeclined   = false;   ///< a reset the athlete chose is not replayed
    uint8_t                  mSummited   = 0;

#if HYBRIDXSTREAK_DEMO
    uint8_t mDemoIndex = 0;
#endif
};

#endif // STREAK_GUI_MODEL_HPP
