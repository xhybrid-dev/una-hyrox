/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   HybridX Ride background logic: runs a ride and records it
 *          (HybridX Workout's service, with the SDK Workout example's calories,
 *          zones and auto lap, the strap, the HybridX activity link, and for an
 *          outdoor ride the SDK Cycling example's GPS distance, speed and
 *          altitude).
 *
 * The ride lives here, not in the screen, so it survives the screen going
 * away: if the GUI closes mid-ride the service keeps recording for
 * kGuiGoneGraceMs, then saves and exits; if the kernel stops the app it saves
 * what it has. A boot that finds a recording interrupted finishes the file
 * first (ActivityWriter::recoverInterrupted).
 *
 * Outdoor or indoor is the main menu's choice (RideSettings::outdoor). Outdoors
 * the GPS location is wanted from the moment the screen opens, so the fix is
 * there by Start; speed, distance and the barometer join at Start. Indoors
 * there is no GPS: heart rate and time only. The GPS is released when the ride
 * ends or the screen goes.
 *
 * Once a second (each new UTC second, not "a second since the last": see
 * SecondGate.hpp) while a ride runs: one sample into the RideModel (heart rate
 * and, outdoors, the motion since the last sample), which may end the lap by
 * the auto lap, then one FIT record. Laps and the session are written when it
 * ends.
 *
 * After a save: the ride goes into ../SharedData/HybridX/act_ride.json
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
#include "SecondGate.hpp"
#include "RideFit.hpp"
#include "RideModel.hpp"
#include "RideSettings.hpp"
#include "RideSnapshot.hpp"

#include "SDK/Filters/SimpleLPF.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Metrics/DeltaCounter.hpp"
#include "SDK/Metrics/MonotonicCounter.hpp"
#include "SDK/Metrics/MonotonicTime.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

class Service
{
public:
    explicit Service(SDK::Kernel &kernel);

    virtual ~Service() = default;

    void run();

    /// If the screen is gone this long with a ride running, save and exit.
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
    /// A GPS speed or position older than this is not "now".
    static constexpr uint32_t skGpsMaxAgeMs = 3000u;
    /// Speeds the example's counters accept, m/s: slower is standing still.
    static constexpr float    skMinSpeedMs = 0.5f;
    static constexpr float    skMaxSpeedMs = 65.0f;
    /// Barometric altitude: the example's smoothing and the smallest change counted as climb, metres.
    static constexpr float    skAltitudeAlpha = 0.8f;
    static constexpr float    skAltitudeMinChangeM = 2.0f;
    static constexpr uint32_t skGpsRetryMs = 2000u;

    bool sessionActive() const;

    void startSession();
    void lap();
    void pause();
    void resume();
    /// End the ride. @p bank: the app is being stopped, not the person ending it.
    void finish(bool save, bool bank);

    void tick();
    void connectSensors();
    void disconnectSensors();
    /// Outdoor: keep the GPS location subscribed while the screen is up (a subscribe can lose the start-up race).
    void maintainGps(uint32_t nowMs);
    void releaseGps();
    Ride::GpsState gpsState(uint32_t nowMs) const;
    void sendGps(uint32_t nowMs);
    /// The motion since the last sample, from the GPS counters (all zero indoors).
    Ride::Motion motionNow(uint32_t nowMs);
    void onSensorData(uint16_t handle, SDK::Sensor::DataBatch &batch);
    /// The lap toast and buzz for a lap that has just ended (by R2 or the auto lap).
    void lapEnded(uint32_t nowMs, bool autoEnded);

    uint16_t currentHr(uint32_t nowMs) const;

    /// The watch's weight and zone thresholds (RequestSystemSettings), as the example reads them.
    void readSystemSettings();
    void loadSettings();
    void saveSettings();
    void setAutoLap(uint8_t minutes);
    void setMode(bool outdoor);

    void requestAccessoryPrepare();
    void requestAccessoryRelease();

    /// The saved ride, for HybridX Streak (hybridx-link/docs/ACTIVITY_LINK.md).
    void writeActivityLink(const Ride::FitSession &fs);
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
    SDK::Sensor::Connection mGpsLocation;   ///< Outdoor, from the screen opening
    SDK::Sensor::Connection mGpsSpeed;      ///< Outdoor, from Start
    SDK::Sensor::Connection mGpsDistance;
    SDK::Sensor::Connection mPressure;

    bool        mGuiStarted = false;
    uint32_t    mGuiGoneAtMs = 0;
    bool        mFitOpen = false;
    bool        mLogOk = true;
    std::time_t mStartUtc = 0;
    Ride::SecondGate mSecond;      ///< One record and one sample per UTC second

    Ride::RideSettings mSettings;
    Ride::HrZones mZones;
    uint32_t    mWeightG = Ride::kDefaultWeightG;

    // GPS (outdoor). The counters pause with the ride, as the Cycling example's.
    bool     mGpsWanted = false;
    uint32_t mGpsRetryAtMs = 0;
    bool     mFix = false;
    uint32_t mFixAtMs = 0;
    float    mLat = 0.0f;
    float    mLon = 0.0f;
    float    mSpeedMs = 0.0f;
    bool     mSpeedValid = false;
    uint32_t mSpeedAtMs = 0;
    uint32_t mSpeedNowMmS = 0;     ///< The speed the last sample used
    float    mSeaLevelPa = 0.0f;
    uint32_t mBadAltitude = 0;     ///< Barometer readings dropped as unusable, this ride
    uint32_t mSentDistCm = 0;      ///< What the model has been given so far
    uint32_t mSentAscentCm = 0;
    uint32_t mSentDescentCm = 0;
    SDK::Metric::MonotonicCounter<float> mDistance;
    SDK::Metric::DeltaCounter            mAltitude;
    SDK::Filter::SimpleLPF               mAltitudeFilter;
    uint8_t  mGpsSentState = 255;  ///< Last GpsState sent to the screen (255: none yet)
    bool     mImperial = false;

    float    mHrBpm = 0.0f;
    uint8_t  mHrTrust = 0;
    uint32_t mHrAtMs = 0;
    bool     mHaveHr = false;
    uint8_t  mBattery = Ride::kBatteryUnknown;
};

#endif // SERVICE_HPP
