/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   HybridX Workout background logic: runs a workout and records it
 *          (X Sauna's service, with the SDK Workout example's calories, zones
 *          and auto lap, the strap, and the HybridX activity link).
 *
 * The workout lives here, not in the screen, so it survives the screen going
 * away: if the GUI closes mid-workout the service keeps recording for
 * kGuiGoneGraceMs, then saves and exits; if the kernel stops the app it saves
 * what it has. A boot that finds a recording interrupted finishes the file
 * first (ActivityWriter::recoverInterrupted).
 *
 * Once a second (each new UTC second, not "a second since the last": see
 * SecondGate.hpp) while a workout runs: one FIT record (heart rate), one
 * sample into the WorkoutModel, which may end the lap by the auto lap, and a
 * tick of the PlanRunner, which ends a plan's timed steps. Each step the runner
 * begins or ends comes back as a Cue, which the service buzzes (onCue). Laps,
 * strength sets and the session are written when it ends, with the sport of
 * the kind of workout (WorkoutFit.hpp, fitSportOf).
 *
 * The plan runs here, not in the screen, so its timers keep going if the
 * screen closes. When a plan ends, the runner pauses the workout; Resume is
 * refused from then on, so only End or Discard remain.
 *
 * After a save: the workout goes into ../SharedData/HybridX/act_workout.json
 * for HybridX Streak (writeActivityLink), and the Streak line is worked out and
 * sent to the Saved screen (sendStreakLine), as HybridX Race does.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <cstdint>
#include <ctime>

#include "ActivityWriter.hpp"
#include "Commands.hpp"
#include "HrZones.hpp"
#include "PlanRunner.hpp"
#include "SecondGate.hpp"
#include "WorkoutFit.hpp"
#include "WorkoutModel.hpp"
#include "WorkoutSettings.hpp"
#include "WorkoutSnapshot.hpp"

#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Metrics/MonotonicTime.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

class Service
{
public:
    explicit Service(SDK::Kernel &kernel);

    virtual ~Service() = default;

    void run();

    /// If the screen is gone this long with a workout running, save and exit.
    static constexpr uint32_t kGuiGoneGraceMs = 5u * 60u * 1000u;

private:
    static constexpr uint32_t skWaitMs = 200u;           ///< Longest wait for a message
    static constexpr uint32_t skNoGuiGraceMs = 10000u;   ///< Exit if no screen ever opens
    /// A heart rate older than this is not "now".
    static constexpr uint32_t skHrMaxAgeMs = 5000u;
    /// Heart-rate gate, as HybridX Race applies it to what goes in the file.
    static constexpr float    skHrMinValid = 20.0f;
    /// How long a wrist raise or a lap lights the screen.
    static constexpr uint32_t skBacklightMs = 5000u;
    static constexpr uint32_t skSamplePeriod = 1000u;
    static constexpr uint32_t skSampleLatency = 1000u;

    bool sessionActive() const;

    void startSession();
    void lap();
    void pause();
    void resume();
    /// End the workout. @p bank: the app is being stopped, not the person ending it.
    void finish(bool save, bool bank);

    void tick();
    void connectSensors();
    void disconnectSensors();
    void onSensorData(uint16_t handle, SDK::Sensor::DataBatch &batch);
    /// The lap toast and buzz for a plain workout's lap that has just ended (R2 or the auto lap).
    void lapEnded(uint32_t nowMs, bool autoEnded);
    /// Buzz and show what the plan runner says has happened.
    void onCue(Workout::Cue cue, uint32_t nowMs);
    /// The lap toast for lap @p index (from 0), with what begins now.
    void sendLapEvent(uint8_t index, Workout::Cue cue);
    /// The vibration pattern for each cue (a countdown of three ticks for Warning).
    void buzzCue(Workout::Cue cue);

    uint16_t currentHr(uint32_t nowMs) const;

    /// The watch's weight and zone thresholds (RequestSystemSettings), as the example reads them.
    void readSystemSettings();
    void loadSettings();
    void saveSettings();
    void setAutoLap(uint8_t minutes);
    void setPlan(const Workout::PlanConfig &plan);

    void requestAccessoryPrepare();
    void requestAccessoryRelease();

    /// The saved workout, for HybridX Streak (hybridx-link/docs/ACTIVITY_LINK.md).
    void writeActivityLink(const Workout::FitSession &fs);
    /// The Streak line on the Saved screen (hybridx-link StreakNow).
    void sendStreakLine();

    void sendUpdate();
    void sendSettings();
    void sendSaved(bool saved);
    void sendLaps(uint8_t first);
    void notifyNewActivity();
    void buzz(uint8_t effect);
    void backlightOn(uint32_t timeoutMs = skBacklightMs);

    SDK::Kernel &mKernel;
    ActivityWriter mWriter;
    SDK::Metric::MonotonicTime<SDK::Interface::ISystem> mTimeTracker;

    SDK::Sensor::Connection mHrSensor;
    SDK::Sensor::Connection mBatterySensor;
    SDK::Sensor::Connection mWristSensor;   ///< While the screen is up: a wrist raise lights it

    bool        mGuiStarted = false;
    uint32_t    mGuiGoneAtMs = 0;
    bool        mFitOpen = false;
    bool        mLogOk = true;
    std::time_t mStartUtc = 0;
    Workout::SecondGate mSecond;      ///< One record and one sample per UTC second

    Workout::WorkoutSettings mSettings;
    Workout::HrZones mZones;
    uint32_t    mWeightG = Workout::kDefaultWeightG;

    float    mHrBpm = 0.0f;
    uint8_t  mHrTrust = 0;
    uint32_t mHrAtMs = 0;
    bool     mHaveHr = false;
    uint8_t  mBattery = Workout::kBatteryUnknown;
};

#endif // SERVICE_HPP
