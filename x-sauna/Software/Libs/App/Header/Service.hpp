/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   X Sauna background logic: runs a sauna session and records it.
 *
 * The session lives here, not in the screen, so it survives the screen going
 * away: if the GUI closes mid-session the service keeps recording for
 * kGuiGoneGraceMs, then saves and exits; if the kernel stops the app it saves
 * what it has (the file says so: completed = 0). A boot that finds a recording
 * interrupted finishes the file first (ActivityWriter::recoverInterrupted).
 *
 * Once a second (each new UTC second, not "a second since the last": see
 * SecondGate.hpp) while a session runs: one FIT record (heart rate and the
 * watch's temperature) and one sample into the SessionModel. Laps and the
 * session are written when it ends (see ActivityWriter.hpp for why).
 *
 * The heat hint (HeatHint.hpp) is not used here: its thresholds are untuned,
 * so the temperature is only recorded.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <cstdint>
#include <ctime>

#include "ActivityWriter.hpp"
#include "Commands.hpp"
#include "SecondGate.hpp"
#include "SessionModel.hpp"
#include "SessionSnapshot.hpp"

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

    /// If the screen is gone this long with a session running, save and exit.
    static constexpr uint32_t kGuiGoneGraceMs = 5u * 60u * 1000u;

private:
    static constexpr uint32_t skWaitMs = 200u;           ///< Longest wait for a message
    static constexpr uint32_t skNoGuiGraceMs = 10000u;   ///< Exit if no screen ever opens
    /// A heart rate or temperature older than this is not "now".
    static constexpr uint32_t skHrMaxAgeMs = 5000u;
    static constexpr uint32_t skTempMaxAgeMs = 10000u;
    /// Heart-rate gate, as HybridX Race applies it to what goes in the file.
    static constexpr float    skHrMinValid = 20.0f;
    /// How long a wrist raise or a lap lights the screen.
    static constexpr uint32_t skBacklightMs = 5000u;
    static constexpr uint32_t skSamplePeriod = 1000u;
    static constexpr uint32_t skSampleLatency = 1000u;

    bool sessionActive() const;

    void startSession(uint8_t mode, uint8_t rest);
    void lap();
    void pause();
    void resume();
    /// End the session. @p bank: the app is being stopped, not the person ending it.
    void finish(bool save, bool bank);

    void tick();
    void connectSensors();
    void disconnectSensors();
    void onSensorData(uint16_t handle, SDK::Sensor::DataBatch &batch);

    uint16_t currentHr(uint32_t nowMs) const;
    int32_t  currentTemp(uint32_t nowMs) const;

    void sendUpdate();
    void sendSaved(bool saved);
    void sendLaps(uint8_t first);
    void notifyNewActivity();
    void buzz(uint8_t effect);
    /// The phase just begun, felt without looking: sauna one pulse, plunge two,
    /// rest one short (as HybridX Race's run, station and Roxzone).
    void phaseBuzz(Sauna::Phase phase);
    void backlightOn(uint32_t timeoutMs = skBacklightMs);

    SDK::Kernel &mKernel;
    ActivityWriter mWriter;
    SDK::Metric::MonotonicTime<SDK::Interface::ISystem> mTimeTracker;

    SDK::Sensor::Connection mHrSensor;
    SDK::Sensor::Connection mTempSensor;
    SDK::Sensor::Connection mBatterySensor;
    SDK::Sensor::Connection mWristSensor;   ///< While the screen is up: a wrist raise lights it

    bool        mGuiStarted = false;
    uint32_t    mGuiGoneAtMs = 0;
    bool        mFitOpen = false;
    bool        mLogOk = true;
    std::time_t mStartUtc = 0;
    Sauna::SecondGate mSecond;      ///< One record and one sample per UTC second

    float    mHrBpm = 0.0f;
    uint8_t  mHrTrust = 0;
    uint32_t mHrAtMs = 0;
    bool     mHaveHr = false;
    int32_t  mTempX100 = Sauna::kX100Invalid;
    uint32_t mTempAtMs = 0;
    uint8_t  mBattery = Sauna::kBatteryUnknown;
};

#endif // SERVICE_HPP
