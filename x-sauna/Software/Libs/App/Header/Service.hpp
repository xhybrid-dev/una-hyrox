/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   X Sauna background logic. At Gate 0 this is the heat probe: it
 *          connects the temperature and heart-rate sensors, keeps what they
 *          do, writes it to a CSV once a second and tells the GUI.
 *
 * Sensors run only while the screen is open, and stop after kWindowS so a
 * probe left on a bench cannot run a battery flat. The CSV is appended in
 * batches (kFlushEveryS), because a watch that gets too hot may switch off:
 * the log should hold everything up to the last few seconds.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <cstdint>

#include "Commands.hpp"
#include "ProbeCsv.hpp"
#include "ProbeSnapshot.hpp"
#include "ProbeStats.hpp"

#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

class Service
{
public:
    explicit Service(SDK::Kernel &kernel);

    virtual ~Service() = default;

    void run();

    /// The log, in the app's folder on the watch.
    static constexpr const char *kLogFile = "heat-probe.csv";

    /// How long the sensors run before the probe stops itself, in seconds.
    static constexpr uint32_t kWindowS = 45u * 60u;
    /// How often the buffered rows are written to the file, in seconds.
    static constexpr uint32_t kFlushEveryS = 5u;

private:
    static constexpr uint32_t skWaitMs = 200u;           ///< Longest wait for a message
    static constexpr uint32_t skNoGuiGraceMs = 10000u;   ///< Exit if no screen opens

    void startSensors();
    void stopSensors(const char *why);
    void tick();
    void onSensorData(uint16_t handle, SDK::Sensor::DataBatch &batch);
    void onTemperature(SDK::Sensor::DataBatch &batch);
    void onHeartRate(SDK::Sensor::DataBatch &batch);

    void sendUpdate();
    Sauna::ProbeSnapshot snapshot() const;

    // The log
    void openLog();
    void queue(const char *text, size_t len);
    void note(const char *text);
    void flushLog();
    bool appendToFile(const char *text, size_t len);

    void buzz(uint8_t effect);

    SDK::Kernel &mKernel;

    SDK::Sensor::Connection mTemp;
    SDK::Sensor::Connection mHr;

    bool     mGuiStarted = false;
    bool     mSensing = false;
    bool     mFinished = false;
    bool     mSimulated = false;
    uint32_t mStartMs = 0;
    uint32_t mElapsedS = 0;
    uint32_t mLastFlushS = 0;

    Sauna::Sense       mTempSense = Sauna::Sense::NotStarted;
    Sauna::Sense       mHrSense = Sauna::Sense::NotStarted;
    Sauna::StreamStats mTempStats;
    Sauna::StreamStats mHrStats;
    uint32_t           mTempBad = 0;
    uint16_t           mTempFields = 0;
    int32_t            mTrustX100 = Sauna::kX100Invalid;
    uint16_t           mMarks = 0;
    uint16_t           mRun = 0;
    Sauna::LogState    mLog = Sauna::LogState::Off;
};

#endif // SERVICE_HPP
