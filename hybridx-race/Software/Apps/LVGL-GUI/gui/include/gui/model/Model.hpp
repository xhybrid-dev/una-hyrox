/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of HybridX Race and its link to the service process.
 *
 * Ported from the SDK's RunLVGL Model and repointed at the race message set
 * (Commands.hpp). It registers with SDK::LVGL::Port for lifecycle and custom
 * messages; screens are plain ModelListeners that own an LVGL screen.
 *
 * The model holds state and forwards intent. It does no timing of its own: the
 * service owns the race, and the one thing the GUI contributes is the instant
 * the split button went down (brief 7.4).
 ******************************************************************************
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include <cstdint>
#include <ctime>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Config.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Utils/Utils.hpp"

#include "ActivitySummary.hpp"
#include "AppMenu.hpp"
#include "Commands.hpp"
#include "RaceData.hpp"
#include "Settings.hpp"
#include "Track.hpp"

// ---------------------------------------------------------------------------
// App::Config -- application-level constants (timing, frame rate).
// ---------------------------------------------------------------------------
namespace App::Config
{
constexpr uint32_t kFrameRate = SDK::GUI::Config::kFrameRate;

constexpr uint32_t kMenuAnimationMs = 400;  // wheel slide
constexpr uint32_t kScreenTimeoutSteps = SDK::Utils::secToTicks(30, kFrameRate);  // 30 s

/// How long the split toast stays up (brief 8.2 item 4).
constexpr uint32_t kSplitToastSteps = SDK::Utils::secToTicks(2, kFrameRate);

/// Auto-save after this long with no input on the finished screen (D4).
constexpr uint32_t kAutoSaveSteps = SDK::Utils::secToTicks(60, kFrameRate);

/// The action menu closes itself back to the race after this (brief 8.2 item 5).
constexpr uint32_t kActionMenuTimeoutSteps = SDK::Utils::secToTicks(10, kFrameRate);

constexpr uint8_t kHrThresholdsCount = CustomMessage::kHrThresholdsCount;
}  // namespace App::Config

// ---------------------------------------------------------------------------
// App::Display -- minimum valid values for on-screen display.
// ---------------------------------------------------------------------------
namespace App::Display
{
constexpr uint8_t kMinHR = 20;  ///< bpm -- below physiological minimum
}  // namespace App::Display

class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    /// Which action a hold-to-confirm is about to commit.
    enum class HoldConfirmMode : uint8_t
    {
        Finish = 0,
        Discard,
    };

    /// The screen that receives model events. Exactly one is bound at a time.
    void bind(ModelListener *listener) { modelListener = listener; }

    /// Menu navigation state, so a screen can restore where the wheel was.
    App::MenuNav::Nav &menu() { return mMenu; }

    void handleKeyEvent(uint8_t key);
    void resetIdleTimer();
    void exitApp();

    // -- System context ------------------------------------------------------
    void getDate(uint8_t &month, uint8_t &day, uint8_t &weekday) const;
    void getTime(uint8_t &h, uint8_t &m, uint8_t &s) const;
    uint8_t getBatteryLevel() const { return mBatteryLevel; }
    bool is12HourFormat() const { return mTimeFormat12h; }
    const uint8_t *getHrThresholds() const { return mHrThresholds; }
    uint8_t getHrThresholdsCount() const { return mHrThresholdsCount; }
    uint8_t getAccessoryState() const { return mAccessoryState; }

    // -- Settings -------------------------------------------------------------
    const Settings &getSettings() const { return mSettings; }
    void saveSettings(const Settings &settings);

    // -- Race format selection (pre-race) --------------------------------------
    Race::Format getFormat() const { return mSettings.format; }
    void setFormat(Race::Format format);

    // -- Race control ----------------------------------------------------------
    void raceStart();
    void raceSplit();
    void raceUndoSplit();
    void racePause();
    void raceResume();
    void raceFinishEarly();
    void raceUndoFinish();
    void raceSave();
    void raceDiscard();

    // -- Race state ------------------------------------------------------------
    bool isRaceActive() const { return mTrackState == Track::State::ACTIVE; }
    bool isRacePaused() const { return mTrackState == Track::State::PAUSED; }
    bool isRaceRunning() const { return isRaceActive() || isRacePaused(); }
    const Track::Data &getRaceData() const { return mRaceData; }
    const Track::SplitEvent &getLastSplit() const { return mLastSplit; }
    bool raceCompleted() const { return mRaceCompleted; }

    // -- Live heart rate outside a race -----------------------------------------
    uint8_t getIdleHr() const { return mIdleHr; }

    // -- Summary -----------------------------------------------------------------
    bool isSummaryAvailable() const { return mSummary.valid; }
    const ActivitySummary &getSummary() const { return mSummary; }
    void requestSummary();

    // -- Hold-to-confirm ----------------------------------------------------------
    void setHoldConfirmMode(HoldConfirmMode mode) { mHoldConfirmMode = mode; }
    HoldConfirmMode getHoldConfirmMode() const { return mHoldConfirmMode; }

private:
    ModelListener     *modelListener;
    const SDK::Kernel &mKernel;

    // IGuiLifeCycleCallback
    void onStart() override;
    void onFrame() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;

    // ICustomMessageHandler
    bool customMessageHandler(SDK::MessageBase *message) override;

    void decIdleTimer();
    static bool isClick(uint8_t key);

    // State
    bool     mIsRunning = false;
    uint32_t mIdleTimer = 0;

    App::MenuNav::Nav mMenu {};
    std::tm           mTime {};

    // Mirrored from the service
    bool     mTimeFormat12h = false;
    uint8_t  mHrThresholds[App::Config::kHrThresholdsCount] = {};
    uint8_t  mHrThresholdsCount = App::Config::kHrThresholdsCount;
    Settings mSettings {};

    uint8_t mBatteryLevel = 0;
    uint8_t mAccessoryState = 0;  // SDK::Accessory::State (0 = UNAVAILABLE)
    uint8_t mIdleHr = 0;

    // Race
    HoldConfirmMode   mHoldConfirmMode = HoldConfirmMode::Discard;
    Track::State      mTrackState = Track::State::INACTIVE;
    Track::Data       mRaceData {};
    Track::SplitEvent mLastSplit {};
    bool              mRaceCompleted = false;

    /// Assembled from SUMMARY_META plus the SUMMARY_PAGE messages that follow.
    /// Owned here: unlike RunLVGL, the GUI never holds a pointer into the
    /// service's memory.
    ActivitySummary mSummary {};
};

#endif  // MODEL_HPP
