/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of X Sauna and its link to the service.
 *
 * The service owns the session and tells the screens about it (Commands.hpp).
 * The model keeps the latest of each message so a screen built later can draw
 * at once, forwards each to the screen on display, runs the idle timer, and
 * remembers the menu choices (mode, rest, the selected rows). What a button
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
    /// Ask for the next phase. False, with nothing sent, inside the lap lockout
    /// (a double tap with a wet finger).
    bool lap();
    void pause();
    void resume();
    void stop(bool save);
    void requestLaps(uint8_t first);

    // What the service last said
    const Sauna::SessionSnapshot& snapshot() const { return mSnapshot; }
    bool sessionActive() const;
    bool paused() const;
    const Sauna::SavedSession& saved() const { return mSaved; }
    bool hasSaved() const { return mHasSaved; }
    /// True when the last session was thrown away on purpose.
    bool discarded() const { return mDiscarded; }
    const Sauna::LapEventData& lastLap() const { return mLastLap; }
    const Sauna::LapsPageData& lapsPage() const { return mLapsPage; }

    // Menu choices, kept while the app is open
    uint8_t mode() const { return mMode; }      ///< Sauna::Mode
    bool    rest() const { return mRest; }
    void    setMode(uint8_t m) { mMode = m; }
    void    setRest(bool r) { mRest = r; }
    uint16_t mainPos() const { return mMainPos; }
    void     setMainPos(uint16_t p) { mMainPos = p; }
    HoldMode holdMode() const { return mHoldMode; }
    void     setHoldMode(HoldMode m) { mHoldMode = m; }
    uint8_t  liveFace() const { return mLiveFace; }
    void     setLiveFace(uint8_t f) { mLiveFace = f; }

#if defined(SIMULATOR)
    /// The text-fit audit hands a screen worst-case data (see Audit.hpp).
    void injectSnapshot(const Sauna::SessionSnapshot& s);
    void injectLapEvent(const Sauna::LapEventData& e);
    void injectSaved(const Sauna::SavedSession& s);
    void injectLapsPage(const Sauna::LapsPageData& p);
#endif

protected:
    ModelListener*     modelListener;   ///< The screen on display
    const SDK::Kernel& mKernel;         ///< The GUI process's kernel interface

    Sauna::SessionSnapshot mSnapshot {};
    Sauna::SavedSession    mSaved {};
    Sauna::LapEventData    mLastLap {};
    Sauna::LapsPageData    mLapsPage {};
    bool     mHasSaved = false;
    bool     mDiscarded = false;

    uint8_t  mMode = 0;
    bool     mRest = false;
    uint16_t mMainPos = 0;
    HoldMode mHoldMode = HoldMode::End;
    uint8_t  mLiveFace = 0;       ///< The face the live screen was on

    Sauna::LapLockout mLapLockout;

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
