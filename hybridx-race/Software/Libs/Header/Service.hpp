/**
 ******************************************************************************
 * @file    Service.hpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   Background process: race logic, sensors, FIT writing, persistence.
 ******************************************************************************
 *
 * Adapted from the SDK's Workout example, which is the closest fit: non-GPS,
 * distance-free, manual laps. Removed: pressure and altitude, distance and
 * speed, calories and body weight, the auto-lap divider. Added: the race model,
 * segment-aware haptics, AppConfig, and the lifecycle rule below.
 *
 * **Lifecycle.** The kernel does not stop a service when its GUI closes
 * (Docs/service-lifecycle.md 1.1). The SDK's activity apps exit ~500 ms later
 * regardless of whether an activity is running -- safe only because their GUI
 * cannot exit mid-activity, and called out as a trap in 5.4. Ours keeps timing
 * and saves after a grace window instead; see skGuiGoneGraceMs.
 *
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <array>
#include <memory>

#include "SDK/AppConfig/AppConfig.hpp"
#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Metrics/MonotonicTime.hpp"
#include "SDK/Metrics/ThrottledSample.hpp"
#include "SDK/Metrics/VariableCounter.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/Timer/Timer.hpp"

#include "ActivitySummary.hpp"
#include "ActivitySummarySerializer.hpp"
#include "ActivityWriter.hpp"
#include "Commands.hpp"
#include "RaceModel.hpp"
#include "Settings.hpp"
#include "SettingsSerializer.hpp"
#include "Track.hpp"
#include "WristTiltDetector.hpp"

class Service : public WristTiltDetector::IListener
{
public:
    explicit Service(SDK::Kernel &kernel);
    virtual ~Service();

    void run();

private:
    // -- Constants ------------------------------------------------------------

    static constexpr uint32_t skBacklightTimeout = 5000u;
    static constexpr uint32_t skSamplePeriod = 1000u;
    static constexpr uint32_t skSampleLatency = 1000u;
    static constexpr uint32_t skBatteryLogPeriodMs = 5u * 60u * 1000u;
    static constexpr float    skFusionSampleRateHz = 100.0f;

    /**
     * @brief How long a race keeps timing after its GUI closes.
     *
     * Jon's call, recorded in NOTES.md 1.2. R2 is Back almost everywhere else
     * in the UI, so the person most likely to leave mid-race is the one
     * fumbling for the split button; saving immediately would end their race on
     * one bad press. After this window with no GUI, the race is saved and the
     * service exits, so nothing leaks and nothing is lost.
     */
    static constexpr uint32_t skGuiGoneGraceMs = 5u * 60u * 1000u;

    /// Heart rate below this is not a heart rate (brief 9.2).
    static constexpr float skHrMinValid = 20.0f;
    static constexpr float skHrMaxValid = 300.0f;

    // -- Infrastructure -------------------------------------------------------

    SDK::Kernel &mKernel;
    bool         mGuiStarted = false;

    /// Monotonic tick when the GUI went away; 0 while it is present.
    uint32_t mGuiGoneAtMs = 0u;

    // -- Settings and persistence ---------------------------------------------

    Settings                  mSettings {};
    bool                      mIsImperial = false;
    bool                      mTimeFormat12h = false;
    SettingsSerializer        mSettingsSerializer;
    ActivitySummary           mSummary {};
    ActivitySummarySerializer mActivitySummarySerializer;
    ActivityWriter            mActivityWriter;

    /// Read in run(), never in the constructor: in the simulator the logger
    /// does not exist yet at construction time (brief 9.4).
    std::unique_ptr<SDK::AppConfig> mConfig;

    // -- Sensors ---------------------------------------------------------------

    SDK::Sensor::Connection mSensorHr;
    SDK::Sensor::Connection mSensorBatteryLevel;
    SDK::Sensor::Connection mSensorBatteryMetrics;
    SDK::Sensor::Connection mSensorWristMotion;
    SDK::Sensor::Connection mSensorFusion;
    bool                    mIsSensorsConnected = false;

    // -- Metrics ----------------------------------------------------------------

    SDK::Metric::MonotonicTime<SDK::Interface::ISystem> mTimeTracker;
    SDK::Metric::VariableCounter                        mHrCounter;
    SDK::Metric::ThrottledSample<float, SDK::Interface::ISystem> mBatterySoc;
    SDK::Metric::ThrottledSample<float, SDK::Interface::ISystem> mBatteryVoltage;

    std::array<uint8_t, CustomMessage::kHrThresholdsCount> mHrThresholds = {};
    uint8_t mHrThresholdCount = 0u;
    uint8_t mHrSource = 0u;       ///< HeartRateEx::Source, for the icon and FIT
    uint8_t mHrOpticalBpm = 0u;   ///< Raw optical bpm, for the FIT series
    uint8_t mHrExternalBpm = 0u;  ///< Raw external bpm, for the FIT series
    uint8_t mHrTrust = 0u;        ///< Latest arbitrated trust level

    // -- The race ----------------------------------------------------------------

    Race::RaceModel mRace;
    Track::State    mTrackState = Track::State::INACTIVE;
    Track::Data     mRaceData {};
    std::time_t     mRaceStartUtc = 0;  ///< Wall time of the start, for FIT
    bool            mFitOpen = false;   ///< True between start() and stop()

    // -- Wrist tilt ---------------------------------------------------------------

    WristTiltDetector mWristTiltDetector;

    // -- Lifecycle -----------------------------------------------------------------

    void connectSensors();
    void disconnect();
    void onStartGUI();
    void onStopGUI();
    bool hasWorkOutstanding() const;

    // -- Sensor data ----------------------------------------------------------------

    void handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch &data);

    // -- Event handlers --------------------------------------------------------------

    void handleEvent(const CustomMessage::SettingsSave &event);
    void handleEvent(const CustomMessage::RaceStart &event);
    void handleEvent(const CustomMessage::RaceSplit &event);
    void handleUndoSplit();
    void handlePause();
    void handleResume();
    void handleFinishEarly();
    void handleUndoFinish();
    void handleSave();
    void handleDiscard();

    // -- Race control ------------------------------------------------------------------

    void loadConfiguration();
    void sendInitialInfoToGui();
    void sendSettings();
    void startRace(Race::Format format);
    void processRace();
    void publishRaceData();
    void onSegmentOpened(bool raceStarting);
    void finishRace(bool completed);
    /// Emit the workout and workout_step messages for the race just started.
    void emitRaceWorkout();

    void saveRace(bool discard);
    void buildSummary();
    void sendSummary();
    ActivityWriter::RecordData prepareRecordData();
    uint32_t nowMs() const;

    // -- Notifications -------------------------------------------------------------------

    void setCapabilities();
    void requestAccessoryPrepare();
    void requestAccessoryRelease();
    void notifySegment(Race::SegmentType type);
    void notifyNewActivity();
    void backlightOn(uint32_t timeoutMs = skBacklightTimeout);
    void playBuzzerPattern(uint16_t beepMs, uint8_t count = 1u, uint16_t silenceMs = 100u);
    void playVibroPattern(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count = 1u,
                          uint16_t silenceMs = 100u);

    // -- WristTilt callback ------------------------------------------------------------------

    void onWristTilt(uint32_t timestampMs) override;
};

#endif  // SERVICE_HPP
