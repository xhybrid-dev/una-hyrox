/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   The Energy Probe's service (x-energy/docs/PROBE.md, Gate SL0): a
 *          resident logger. It runs from boot (APP_AUTOSTART), listens to the
 *          sensors the night's mode asks for, and writes a row a minute (ten
 *          minutes when Off), each night's battery drain, and the sensor
 *          check. It never exits because its screen closed.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <cstddef>
#include <cstdint>

#include "Commands.hpp"
#include "MinuteEpoch.hpp"
#include "NightSummary.hpp"
#include "ProbeMode.hpp"
#include "Schedule.hpp"
#include "StateFile.hpp"

#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/SensorLayer/SensorTypes.hpp"

class Service
{
public:
    explicit Service(SDK::Kernel& kernel);
    virtual ~Service() = default;

    void run();

private:
    // --- time ---
    struct Clock {
        uint32_t              utc   = 0;
        EnergyProbe::LocalTime local;
        char                  text[20] = {};   ///< "2026-11-24T22:01"
        bool                  valid = false;   ///< the clock has a real date
    };
    Clock clockNow() const;

    // --- start and stop ---
    void start();
    void stop();

    // --- the sensors per mode ---
    void applyMode(EnergyProbe::Mode mode);
    bool connectGroup(uint32_t group);
    void disconnectGroup(uint32_t group);
    void onSensor(uint16_t handle, SDK::Sensor::DataBatch& batch);

    // --- rows ---
    void beginRow(const Clock& c);
    void closeRow(const Clock& c);
    void sparseHeart(const Clock& c);
    uint32_t rowEndUtc(uint32_t startUtc) const;

    // --- the night ---
    void enterPeriod(const EnergyProbe::Period& p, const Clock& c, bool atStart);
    void closeNight(const EnergyProbe::NightResult& r);

    // --- files ---
    void flush();
    bool append(const char* path, const char* header, const char* data, size_t len);
    void event(const char* what, const char* detail);
    void loadState();
    void saveState();

    // --- the GUI ---
    void sendStatus();
    void sendNights();
    void sendCheckLines();
    void onOverride(uint8_t value);
    void onWindow(uint8_t value);
    void onDayMode(uint8_t value);

    // --- the sensor check ---
    void startCheck();
    void checkData(uint16_t handle, SDK::Sensor::DataBatch& batch);
    void finishCheck();
    bool sharedHandle(uint32_t handle);

    SDK::Kernel& mKernel;

    // Sensors. Battery in every mode; the rest by group (EnergyProbe::Group).
    SDK::Sensor::Connection mBattLevel;
    SDK::Sensor::Connection mBattCharge;
    SDK::Sensor::Connection mBattMetrics;
    SDK::Sensor::Connection mAccel;
    SDK::Sensor::Connection mAccelRaw;
    SDK::Sensor::Connection mHeart;
    SDK::Sensor::Connection mBeat;
    SDK::Sensor::Connection mTemp;
    SDK::Sensor::Connection mTouch;
    SDK::Sensor::Connection mMotionDet;
    SDK::Sensor::Connection mActRec;
    SDK::Sensor::Connection mWrist;
    SDK::Sensor::Connection mHrDaily;
    SDK::Sensor::Connection mSteps;
    SDK::Sensor::Connection mActive;
    SDK::Sensor::Connection mFloors;
    bool     mAccelIsRaw     = false;
    bool     mHeartOn        = false;      ///< for Heart 1 in 5: connected this minute
    uint32_t mHeartOnMs      = 0;
    bool     mHeartTrustedYet = false;
    uint32_t mGroupsOn       = 0;
    uint32_t mGroupsFailed   = 0;
    uint32_t mBattPeriodMs   = 0;

    // State.
    EnergyProbe::ProbeState   mState;
    EnergyProbe::NightTracker mNight;
    EnergyProbe::MinuteEpoch  mEpoch;
    EnergyProbe::Mode         mMode     = EnergyProbe::Mode::Off;
    EnergyProbe::Period       mPeriod;
    bool                      mHavePeriod = false;
    bool                      mGuiStarted = false;
    uint32_t                  mRowStartUtc = 0;
    uint32_t                  mRowEndUtc   = 0;
    uint32_t                  mRowStartMs  = 0;
    char                      mRowLocal[20] = {};
    int32_t                   mRowDay      = 0;
    int32_t                   mBufferDay   = 0;   ///< the local date the buffered rows belong to

    // Write buffers (fixed: no allocation in the loop).
    static constexpr size_t kRowBuf   = 4096;
    static constexpr size_t kBeatBuf  = 3072;
    static constexpr size_t kEventBuf = 1536;
    static constexpr uint32_t kFlushRows = 10;
    char     mRows[kRowBuf]     = {};
    size_t   mRowsLen           = 0;
    uint32_t mRowsBuffered      = 0;
    char     mBeats[kBeatBuf]   = {};
    size_t   mBeatsLen          = 0;
    char     mEvents[kEventBuf] = {};
    size_t   mEventsLen         = 0;
    uint32_t mRowsWritten       = 0;
    uint32_t mBytesWritten      = 0;
    uint32_t mWriteErrors       = 0;
    uint32_t mDroppedBeats      = 0;
    char     mStateText[1536]   = {};   ///< state.txt, read or written

    // The sensor check.
    struct CheckEntry {
        SDK::Sensor::Type type;
        const char*       name;
        float             period;
        uint32_t          latency;
        uint32_t          handle;
        uint32_t          handles;
        char              desc[33];
        bool              connected;
        bool              shared;
        uint32_t          msgs;
        uint32_t          samples;
        uint16_t          fields;
        uint16_t          stride;
        uint32_t          firstTsMs;
        uint32_t          firstNowMs;
        uint32_t          lastTsMs;
        uint8_t           rawN;
        uint32_t          raw[3][4];
    };
    static constexpr uint8_t  kCheckCount = 21;
    static constexpr uint32_t kCheckMs    = 120000;
    CheckEntry mCheck[kCheckCount] {};
    bool       mCheckRunning = false;
    bool       mCheckDone    = false;
    uint32_t   mCheckStartMs = 0;
    uint32_t   mCheckLastProgressMs = 0;
};

#endif // SERVICE_HPP
