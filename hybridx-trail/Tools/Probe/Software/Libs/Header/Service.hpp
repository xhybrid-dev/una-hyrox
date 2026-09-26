/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   The Trail Probe's service: the route check, memory, then sensors.
 *
 * On start it runs Probe::Runner over its own "Routes/" folder (the GPX copied
 * over USB), measures the largest block it can allocate, and saves probe.txt
 * straight away, so the route half of the answer is on the watch even if the
 * screen is closed at once.
 *
 * When the GUI opens it connects GPS_LOCATION, MAGNETIC_FIELD and
 * ACCELEROMETER for a three-minute window, and once a second sends the GUI
 * what it has: time to first fix, the GPS's precision, whether the compass is
 * calibrated, the level and tilt-compensated bearings, and, with a fix and a
 * route, how far the watch is from the route. It writes a line to the report
 * every 15 s, and at the end (or when the screen is closed) saves probe.txt
 * again and appends one line to probe-history.txt. Both files are in the
 * probe's own folder, /Apps/HXTrailProbe/.
 ******************************************************************************
 */

#ifndef TRAIL_PROBE_SERVICE_HPP
#define TRAIL_PROBE_SERVICE_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

#include "GeoPoint.hpp"
#include "ProbeRunner.hpp"
#include "RouteBuilder.hpp"

class Service : private Probe::Host
{
public:
    explicit Service(SDK::Kernel& kernel);

    void run();

private:
    // Probe::Host
    void line(const char* text) override;

    void     probeRoute();
    void     probeMemory();
    void     clock();
    uint16_t historyRuns();
    void     saveReport();
    void     appendHistory();

    void startSensors();
    void stopSensors(const char* why);
    void tick();
    void onSensorData(uint16_t handle, SDK::Sensor::DataBatch& batch);
    void sendResult();

    SDK::Kernel&        mKernel;
    Trail::RouteBuilder mRoute;
    Probe::Result       mResult {};
    size_t              mReportLen   = 0;
    bool                mGuiStarted  = false;
    bool                mSensing     = false;
    bool                mHistoryDone = false;
    char                mStamp[40] {};

    SDK::Sensor::Connection mGps;
    SDK::Sensor::Connection mMag;
    SDK::Sensor::Connection mAccel;
    uint32_t        mSenseStartMs = 0;
    uint16_t        mLastLoggedS  = 0;
    bool            mHaveFix      = false;
    Trail::GeoPoint mFix {};
    bool            mHaveAccel    = false;
    float           mAx = 0.0f, mAy = 0.0f, mAz = 0.0f;

    static constexpr uint32_t skStartupGraceMs = 5000u;
    static constexpr uint32_t skWaitMs         = 250u;
    static constexpr uint16_t skSenseWindowS   = 180u;   ///< long enough for a cold GPS start
    static constexpr uint16_t skLogEveryS      = 15u;
    static constexpr uint32_t skMaxAllocProbeB = 512u * 1024u;
};

#endif // TRAIL_PROBE_SERVICE_HPP
