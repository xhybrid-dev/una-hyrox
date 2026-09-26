/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   The Trail Probe's service (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <new>

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAccelerometer.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserMagneticField.hpp"

#include "ProbeMessages.hpp"
#include "RouteMath.hpp"

#define LOG_MODULE_PRX   "Probe"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{
constexpr const char* kReportFile  = "probe.txt";
constexpr const char* kHistoryFile = "probe-history.txt";

/// The report, built line by line. Static so it is off the 10 KB stack.
constexpr size_t kReportMax = 6 * 1024;
char             sReport[kReportMax];

/// The route: 2,000 points is 16 KB, a point every 25 m over 50 km before
/// any extra thinning. The real app's figure is decided at Gate T0.
constexpr uint16_t kRoutePoints = 2000;
Trail::GeoPoint    sRoutePoints[kRoutePoints];

/// The runner holds a 512-byte read buffer and the reader's tag buffer: static
/// for the same reason as the report.
alignas(Probe::Runner) uint8_t sRunnerStorage[sizeof(Probe::Runner)];

char sChunk[256];
char sLine[256];

// GPS as RunLVGL connects it (its Service.hpp skSamplePeriod/Latency). The
// SDK leaves the unit to the driver (SensorConnection.hpp); RunLVGL's fusion
// connection, 1000.0f / Hz, shows it is milliseconds. 5 Hz is plenty for a
// compass that a runner glances at.
constexpr float    kGpsPeriod     = 1000.0f;
constexpr uint32_t kGpsLatency    = 1000u;
constexpr float    kCompassPeriod = 200.0f;
constexpr uint32_t kCompassLatency = 1000u;

/// "51.507" from 515072000: three decimals, about 100 m. The report is sent
/// back in conversation, so it says roughly where the test was, not a doorstep.
void coarse(int32_t e7, char* out, size_t cap)
{
    const bool     neg = e7 < 0;
    const uint32_t a   = neg ? static_cast<uint32_t>(-(e7 / 10000)) : static_cast<uint32_t>(e7 / 10000);
    snprintf(out, cap, "%s%lu.%03lu", neg ? "-" : "", static_cast<unsigned long>(a / 1000u),
             static_cast<unsigned long>(a % 1000u));
}
} // namespace

Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel)
    , mRoute(sRoutePoints, kRoutePoints)
    , mGps(SDK::Sensor::Type::GPS_LOCATION, kGpsPeriod, kGpsLatency)
    , mMag(SDK::Sensor::Type::MAGNETIC_FIELD, kCompassPeriod, kCompassLatency)
    , mAccel(SDK::Sensor::Type::ACCELEROMETER, kCompassPeriod, kCompassLatency)
{
}

void Service::run()
{
    LOG_INFO("Started\n");
    mReportLen = 0;
    line("HybridX Trail probe, app " APP_NAME);
    clock();
    mResult.run = static_cast<uint16_t>(historyRuns() + 1u);
    probeRoute();
    probeMemory();
    saveReport();

    const uint32_t startMs = mKernel.sys.getTimeMs();
    while (true) {
        SDK::MessageBase* msg = nullptr;
        if (mKernel.comm.getMessage(msg, skWaitMs)) {
            switch (msg->getType()) {
                case SDK::MessageType::COMMAND_APP_STOP:
                    stopSensors("app stopped");
                    mKernel.comm.releaseMessage(msg);
                    return;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                    mGuiStarted = true;
                    if (!mSensing && !mResult.finished) {
                        startSensors();
                    }
                    sendResult();
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                    mGuiStarted = false;
                    stopSensors("screen closed");
                    break;

                case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                    auto* event = static_cast<SDK::Message::Sensor::EventData*>(msg);
                    SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                    onSensorData(event->handle, batch);
                } break;

                default:
                    break;
            }
            mKernel.comm.releaseMessage(msg);
        }
        if (mSensing) {
            tick();
        }
        // Unsigned subtraction stays right across the millisecond clock's wrap.
        if (!mGuiStarted && mKernel.sys.getTimeMs() - startMs >= skStartupGraceMs) {
            LOG_INFO("No GUI: exiting\n");
            appendHistory();   // once only: a no-op if the screen already closed the run
            return;
        }
    }
}

// -- Probe::Host ------------------------------------------------------------------

void Service::line(const char* text)
{
    LOG_INFO("%s\n", text);
    const size_t len = std::strlen(text);
    if (mReportLen + len + 2 > kReportMax) {
        return;   // a full report drops later lines rather than corrupt earlier ones
    }
    std::memcpy(sReport + mReportLen, text, len);
    mReportLen += len;
    sReport[mReportLen++] = '\n';
}

// -- Route and memory ----------------------------------------------------------------

void Service::probeRoute()
{
    const uint32_t t0     = mKernel.sys.getTimeMs();
    auto*          runner = new (sRunnerStorage) Probe::Runner(mKernel.fs, *this, mRoute);
    runner->run(mResult);
    runner->~Runner();
    mResult.readMs = mKernel.sys.getTimeMs() - t0;

    snprintf(sLine, sizeof(sLine), "Route: %s (listing and reading took %lu ms)", Probe::verdictName(mResult.verdict),
             static_cast<unsigned long>(mResult.readMs));
    line(sLine);
}

void Service::probeMemory()
{
    // Binary search for the largest single block, to the nearest KB. Each
    // attempt is freed at once: nothing is held past this function.
    uint32_t lo = 0;
    uint32_t hi = skMaxAllocProbeB / 1024u;
    while (lo < hi) {
        const uint32_t mid = (lo + hi + 1u) / 2u;
        void*          p   = mKernel.mem.malloc(mid * 1024u);
        if (p != nullptr) {
            mKernel.mem.free(p);
            lo = mid;
        } else {
            hi = mid - 1u;
        }
    }
    mResult.largestAllocB = lo * 1024u;
    snprintf(sLine, sizeof(sLine), "Memory: largest single allocation %lu KB (probed up to %lu KB)",
             static_cast<unsigned long>(lo), static_cast<unsigned long>(skMaxAllocProbeB / 1024u));
    line(sLine);
}

// -- Sensors ---------------------------------------------------------------------------

void Service::startSensors()
{
    mSensing      = true;
    mSenseStartMs = mKernel.sys.getTimeMs();
    mLastLoggedS  = 0;
    mResult.gps     = mGps.connect() ? Probe::Sense::NoData : Probe::Sense::ConnectFailed;
    mResult.compass = mMag.connect() ? Probe::Sense::NoData : Probe::Sense::ConnectFailed;
    const bool accel = mAccel.connect();
    snprintf(sLine, sizeof(sLine), "Sensors: GPS %s, compass %s, accelerometer %s", Probe::senseName(mResult.gps),
             Probe::senseName(mResult.compass), accel ? "connected" : "connect failed");
    line(sLine);
    line("  (t = seconds since the screen opened)");
}

void Service::stopSensors(const char* why)
{
    if (!mSensing) {
        return;
    }
    mSensing = false;
    mGps.disconnect();
    mMag.disconnect();
    mAccel.disconnect();
    mResult.finished = true;

    snprintf(sLine, sizeof(sLine), "Sensors stopped at t=%u s: %s", static_cast<unsigned>(mResult.elapsedS), why);
    line(sLine);
    snprintf(sLine, sizeof(sLine), "  GPS %s: %lu samples, first fix %s%u s, precision %u.%u m",
             Probe::senseName(mResult.gps), static_cast<unsigned long>(mResult.gpsSamples),
             mResult.fixAfterS ? "after " : "none, ", static_cast<unsigned>(mResult.fixAfterS),
             static_cast<unsigned>(mResult.precisionDm / 10u), static_cast<unsigned>(mResult.precisionDm % 10u));
    line(sLine);
    snprintf(sLine, sizeof(sLine), "  Compass %s: %lu samples, %lu calibrated, accelerometer %lu samples",
             Probe::senseName(mResult.compass), static_cast<unsigned long>(mResult.magSamples),
             static_cast<unsigned long>(mResult.magCalibrated), static_cast<unsigned long>(mResult.accelSamples));
    line(sLine);
    saveReport();
    appendHistory();
    sendResult();
}

void Service::tick()
{
    const uint32_t ms      = mKernel.sys.getTimeMs() - mSenseStartMs;
    const uint16_t elapsed = static_cast<uint16_t>(ms / 1000u);
    if (elapsed == mResult.elapsedS) {
        return;
    }
    mResult.elapsedS = elapsed;

    if (mHaveFix && mRoute.count() > 0) {
        const auto n      = Trail::RouteMath::nearest(mRoute.points(), mRoute.count(), mFix);
        mResult.offRouteM = n.valid ? static_cast<int32_t>(n.distanceM + 0.5f) : -1;
    }

    if (elapsed >= mLastLoggedS + skLogEveryS) {
        mLastLoggedS = elapsed;
        char lat[16] = "-";
        char lon[16] = "-";
        if (mHaveFix) {
            coarse(mFix.latE7, lat, sizeof(lat));
            coarse(mFix.lonE7, lon, sizeof(lon));
        }
        snprintf(sLine, sizeof(sLine),
                 "t=%u GPS %s n=%lu prec %u.%u m at %s,%s | compass %s n=%lu cal=%lu bearing %d tilted %d | "
                 "off route %ld m",
                 static_cast<unsigned>(elapsed), Probe::senseName(mResult.gps),
                 static_cast<unsigned long>(mResult.gpsSamples), static_cast<unsigned>(mResult.precisionDm / 10u),
                 static_cast<unsigned>(mResult.precisionDm % 10u), lat, lon, Probe::senseName(mResult.compass),
                 static_cast<unsigned long>(mResult.magSamples), static_cast<unsigned long>(mResult.magCalibrated),
                 static_cast<int>(mResult.bearingDeg), static_cast<int>(mResult.tiltedDeg),
                 static_cast<long>(mResult.offRouteM));
        line(sLine);
    }

    if (elapsed >= skSenseWindowS) {
        stopSensors("three minutes up");
        return;
    }
    if (mGuiStarted) {
        sendResult();
    }
}

void Service::onSensorData(uint16_t handle, SDK::Sensor::DataBatch& batch)
{
    if (mGps.matchesDriver(handle)) {
        for (uint16_t i = 0; i < batch.size(); ++i) {
            SDK::SensorDataParser::GpsLocation p(batch[i]);
            if (!p.isDataValid()) {
                continue;
            }
            ++mResult.gpsSamples;
            if (!p.isCoordinatesValid()) {
                if (mResult.gps != Probe::Sense::Ok) {
                    mResult.gps = Probe::Sense::Searching;
                }
                continue;
            }
            const Trail::GeoPoint g = Trail::Geo::fromDegrees(p.getLatitude(), p.getLongitude());
            if (!Trail::Geo::valid(g)) {
                continue;
            }
            if (!mHaveFix) {
                mResult.fixAfterS = static_cast<uint16_t>((mKernel.sys.getTimeMs() - mSenseStartMs) / 1000u);
                if (mResult.fixAfterS == 0) {
                    mResult.fixAfterS = 1;   // 0 means "no fix yet"
                }
            }
            mHaveFix    = true;
            mFix        = g;
            mResult.gps = Probe::Sense::Ok;
            const float prec = p.getPrecision();
            mResult.precisionDm =
                prec <= 0.0f ? 0 : (prec >= 6553.0f ? 65535 : static_cast<uint16_t>(prec * 10.0f + 0.5f));
        }
    } else if (mAccel.matchesDriver(handle)) {
        for (uint16_t i = 0; i < batch.size(); ++i) {
            SDK::SensorDataParser::Accelerometer p(batch[i]);
            if (p.isDataValid()) {
                ++mResult.accelSamples;
                mHaveAccel = true;
                mAx        = p.getX();
                mAy        = p.getY();
                mAz        = p.getZ();
            }
        }
    } else if (mMag.matchesDriver(handle)) {
        for (uint16_t i = 0; i < batch.size(); ++i) {
            SDK::SensorDataParser::MagneticField p(batch[i]);
            if (!p.isDataValid()) {
                continue;
            }
            ++mResult.magSamples;
            if (!p.isCalibrated()) {
                if (mResult.compass != Probe::Sense::Ok) {
                    mResult.compass = Probe::Sense::Searching;
                }
                continue;
            }
            ++mResult.magCalibrated;
            mResult.compass    = Probe::Sense::Ok;
            mResult.bearingDeg = p.isAzimuthValid() ? static_cast<int16_t>(p.getAzimuthDeg()) : -1;
            float tilted       = 0.0f;
            // Paired with the latest accelerometer sample: both arrive in
            // batches at 5 Hz, so it is at most a batch (1 s) old. Fine for a
            // probe; the app will pair them by timestamp.
            mResult.tiltedDeg = mHaveAccel && p.getAzimuthDegTilted(mAx, mAy, mAz, tilted)
                                    ? static_cast<int16_t>(tilted)
                                    : -1;
        }
    }
}

void Service::sendResult()
{
    SDK::send_msg<CustomMessage::ProbeResult>(mKernel, mResult);
}

// -- Files -----------------------------------------------------------------------------

void Service::clock()
{
    const std::time_t utc = std::time(nullptr);
    std::tm           local {};
    localtime_r(&utc, &local);
    snprintf(mStamp, sizeof(mStamp), "%04d-%02d-%02d %02d:%02d:%02d", local.tm_year + 1900, local.tm_mon + 1,
             local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
    snprintf(sLine, sizeof(sLine), "Clock: local %s, UTC %lu", mStamp, static_cast<unsigned long>(utc));
    line(sLine);
}

uint16_t Service::historyRuns()
{
    auto file = mKernel.fs.file(kHistoryFile);
    if (!file || !file->open(false, false)) {
        return 0;
    }
    uint16_t lines = 0;
    size_t   got   = 0;
    while (file->read(sChunk, sizeof(sChunk), got) && got > 0) {
        for (size_t i = 0; i < got; ++i) {
            if (sChunk[i] == '\n' && lines < UINT16_MAX) {
                ++lines;
            }
        }
    }
    file->close();
    return lines;
}

void Service::saveReport()
{
    if (auto file = mKernel.fs.file(kReportFile); file && file->open(true, true)) {
        size_t written = 0;
        file->write(sReport, mReportLen, written);
        file->close();
        LOG_INFO("Wrote %s (%u bytes)\n", kReportFile, static_cast<unsigned>(written));
    } else {
        LOG_ERROR("Cannot write %s\n", kReportFile);
    }
}

void Service::appendHistory()
{
    if (mHistoryDone) {
        return;
    }
    mHistoryDone = true;

    const int len = snprintf(
        sLine, sizeof(sLine) - 1,
        "run %u | %s | route %s \"%s\" %lu B, %lu pts -> %u, %lu m, %lu ms | mem %lu KB | gps %s fix %us "
        "prec %u.%um | compass %s cal %lu/%lu | off %ld m",
        static_cast<unsigned>(mResult.run), mStamp, Probe::verdictName(mResult.verdict),
        mResult.file[0] ? mResult.file : "-", static_cast<unsigned long>(mResult.fileBytes),
        static_cast<unsigned long>(mResult.rawPoints), static_cast<unsigned>(mResult.kept),
        static_cast<unsigned long>(mResult.lengthM), static_cast<unsigned long>(mResult.readMs),
        static_cast<unsigned long>(mResult.largestAllocB / 1024u), Probe::senseName(mResult.gps),
        static_cast<unsigned>(mResult.fixAfterS), static_cast<unsigned>(mResult.precisionDm / 10u),
        static_cast<unsigned>(mResult.precisionDm % 10u), Probe::senseName(mResult.compass),
        static_cast<unsigned long>(mResult.magCalibrated), static_cast<unsigned long>(mResult.magSamples),
        static_cast<long>(mResult.offRouteM));
    size_t n = len > 0 ? static_cast<size_t>(len) : 0;
    if (n >= sizeof(sLine) - 1) {
        n = sizeof(sLine) - 2;
    }
    sLine[n++] = '\n';

    if (auto file = mKernel.fs.file(kHistoryFile); file && file->open(true, false)) {
        size_t written = 0;
        file->seek(file->size());
        file->write(sLine, n, written);
        file->close();
    } else {
        LOG_ERROR("Cannot write %s\n", kHistoryFile);
    }
}
