/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of HybridX Ride and its link to the service
 *          (HybridX Workout's, plus the ride type, units and GPS state).
 *
 * The service owns the session and tells the screens about it (Commands.hpp).
 * The model keeps the latest of each message so a screen built later can draw
 * at once, forwards each to the screen on display, runs the idle timer, and
 * remembers the menu position and the live face. The settings (the auto lap)
 * are the service's, which remembers them between rides. What a button
 * does to the session is a request to the service; a screen only ever shows
 * what the service last said.
 ******************************************************************************
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include <cstdint>

#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include "Commands.hpp"
#include "LapLockout.hpp"

class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    /// The kernel ticks the GUI at 10 Hz.
    static constexpr uint32_t kFrameRate = 10;
    /// A menu screen exits the app after this many frames without a key (30 s).
    static constexpr uint32_t kScreenTimeoutSteps = 30u * kFrameRate;
    /// The action menu closes itself after this many (10 s).
    static constexpr uint32_t kActionTimeoutSteps = 10u * kFrameRate;

    /// What a held R1 on the action menu is confirming.
    enum class HoldMode : uint8_t { End, Discard };

    Model();

    /// The screen that receives model events. Exactly one is bound at a time.
    void bind(ModelListener* listener) { modelListener = listener; }

    // Keys and the idle timer. A click restarts the timer at the screen's own
    // timeout (setIdleTimeout), 30 s unless the screen says otherwise.
    void handleKeyEvent(uint8_t key);
    void setIdleTimeout(uint32_t steps = kScreenTimeoutSteps);

    /// Exit the application: the kernel stops both processes.
    void exitApp();

    // The session: requests to the service
    void startSession();
    /// Ask for the next lap. False, with nothing sent, inside the lap lockout
    /// (a double tap).
    bool lap();
    void pause();
    void resume();
    void stop(bool save);
    void requestLaps(uint8_t first);
    /// Step the auto lap to its next choice (the service saves it and answers).
    void nextAutoLap();
    /// Choose Outdoor or Indoor (the service saves it and answers). Also tells the
    /// service the main menu is up, so an outdoor ride's GPS starts looking.
    void setOutdoor(bool outdoor);

    // What the service last said
    const Ride::SessionSnapshot& snapshot() const { return mSnapshot; }
    bool sessionActive() const;
    bool paused() const;
    const Ride::SavedSession& saved() const { return mSaved; }
    bool hasSaved() const { return mHasSaved; }
    /// True when the last session was thrown away on purpose.
    bool discarded() const { return mDiscarded; }
    const Ride::LapEventData& lastLap() const { return mLastLap; }
    const Ride::LapsPageData& lapsPage() const { return mLapsPage; }
    /// The remembered auto lap, minutes (0: off).
    uint8_t autoLapMin() const { return mAutoLapMin; }
    /// The remembered ride type, and the watch profile's units.
    bool outdoor() const { return mOutdoor; }
    bool imperial() const { return mImperial; }
    bool settingsKnown() const { return mSettingsKnown; }
    /// The GPS (Ride::GpsState), as the service last said.
    uint8_t gps() const { return mGps; }
    /// The strap's state (SDK::Accessory::State), kStrapUnknown before the kernel says.
    static constexpr uint8_t kStrapUnknown = 0xFF;
    uint8_t strapState() const { return mStrapState; }
    const char* strapName() const { return mStrapName; }
    /// The Streak line after the last save (empty strings when there is none).
    bool streakShow() const { return mStreakShow; }
    const char* streakLine1() const { return mStreakLine1; }
    const char* streakLine2() const { return mStreakLine2; }

    // Menu state, kept while the app is open
    uint16_t mainPos() const { return mMainPos; }
    void     setMainPos(uint16_t p) { mMainPos = p; }
    HoldMode holdMode() const { return mHoldMode; }
    void     setHoldMode(HoldMode m) { mHoldMode = m; }
    uint8_t  liveFace() const { return mLiveFace; }
    void     setLiveFace(uint8_t f) { mLiveFace = f; }

#if defined(SIMULATOR)
    /// The text-fit audit hands a screen worst-case data (see Audit.hpp).
    void injectSnapshot(const Ride::SessionSnapshot& s);
    void injectLapEvent(const Ride::LapEventData& e);
    void injectSaved(const Ride::SavedSession& s);
    void injectLapsPage(const Ride::LapsPageData& p);
    void injectStreakLine(bool show, const char* line1, const char* line2);
    void injectStrap(uint8_t state, const char* name);
    void injectGps(uint8_t state);
    void injectSettings(bool outdoor, bool imperial);
#endif

protected:
    ModelListener*     modelListener;   ///< The screen on display
    const SDK::Kernel& mKernel;         ///< The GUI process's kernel interface

    Ride::SessionSnapshot mSnapshot {};
    Ride::SavedSession    mSaved {};
    Ride::LapEventData    mLastLap {};
    Ride::LapsPageData    mLapsPage {};
    bool     mHasSaved = false;
    bool     mDiscarded = false;

    uint8_t  mAutoLapMin = 0;
    bool     mOutdoor = true;
    bool     mImperial = false;
    bool     mSettingsKnown = false;   ///< The service has sent the settings
    uint8_t  mGps = 0;
    uint8_t  mStrapState = kStrapUnknown;
    char     mStrapName[24] = {};
    bool     mStreakShow = false;
    char     mStreakLine1[24] = {};
    char     mStreakLine2[20] = {};
    uint16_t mMainPos = 0;
    HoldMode mHoldMode = HoldMode::End;
    uint8_t  mLiveFace = 0;       ///< The face the live screen was on

    Ride::LapLockout mLapLockout;

    bool     mIsRunning = true;
    uint32_t mIdleSteps = kScreenTimeoutSteps;   ///< What a click restarts the timer at
    uint32_t mIdleTimer = kScreenTimeoutSteps;

    // IGuiLifeCycleCallback
    void onStart() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;
    void onFrame() override;

    // ICustomMessageHandler: messages from the service
    bool customMessageHandler(SDK::MessageBase* msg) override;
};

#endif // MODEL_HPP
