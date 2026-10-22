/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   X Sauna background logic: the Gate 0 heat probe (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "FixedPoint.hpp"
#include "SimTemp.hpp"

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRate.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserTemperature.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace
{
/// The rows waiting to be written. Static, so they are off the service's
/// small stack: five seconds of rows is about 400 bytes, and a run header and
/// a few notes take a little more.
constexpr size_t kCsvCap = 1024;
char             sCsv[kCsvCap];
size_t           sCsvLen = 0;

char sLine[192];     ///< One line being built
char sChunk[256];    ///< A piece of the log being read to number the run

/// "2026-10-20 14:32:05" in the watch's local time.
void clockStamp(char *out, size_t cap)
{
    const std::time_t utc = std::time(nullptr);
    std::tm           local {};
    localtime_r(&utc, &local);
    std::snprintf(out, cap, "%04d-%02d-%02d %02d:%02d:%02d", local.tm_year + 1900, local.tm_mon + 1,
                  local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
}

/// The probe judges the hint, so it runs with the hint switched on and every
/// threshold at its untuned placeholder (HeatHint.hpp).
Sauna::HeatHintConfig probeHintConfig()
{
    Sauna::HeatHintConfig config;
    config.enabled = true;
    return config;
}

uint16_t bpmFromX100(int32_t x100)
{
    if (x100 == Sauna::kX100Invalid || x100 <= 0) {
        return 0;
    }
    const int32_t bpm = (x100 + 50) / 100;
    return bpm > 65535 ? 65535 : static_cast<uint16_t>(bpm);
}
} // namespace

Service::Service(SDK::Kernel &kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mTemp(SDK::Sensor::Type::AMBIENT_TEMPERATURE, 0, 0)
    , mHr(SDK::Sensor::Type::HEART_RATE, 0, 0)
{
    (void)kernel;
}

void Service::run()
{
    LOG_INFO("thread started\n");
    const uint32_t bootMs = mKernel.sys.getTimeMs();

    while (true) {
        SDK::MessageBase *msg = nullptr;
        if (mKernel.comm.getMessage(msg, skWaitMs)) {
            switch (msg->getType()) {
            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
                stopSensors("app stopped");
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
                mGuiStarted = true;
                if (!mSensing && !mFinished) {
                    startSensors();
                }
                sendUpdate();
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped, exiting service\n");
                mGuiStarted = false;
                stopSensors("screen closed");
                // The kernel does not stop a service when its GUI closes, and
                // nothing else will reclaim it.
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto *event = static_cast<SDK::Message::Sensor::EventData *>(msg);
                SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                onSensorData(event->handle, batch);
            } break;

            case CustomMessage::PROBE_MARK:
                if (mSensing) {
                    if (mMarks < UINT16_MAX) {
                        ++mMarks;
                    }
                    mPhase = Sauna::nextProbePhase(mPhase);
                    buzz(SDK::Message::RequestVibroPlay::STRONG_CLICK_100);
                    std::snprintf(sLine, sizeof(sLine), "phase %s at t=%lu s", Sauna::probePhaseName(mPhase),
                                  static_cast<unsigned long>(mElapsedS));
                    note(sLine);
                    sendUpdate();
                }
                break;

            case CustomMessage::PROBE_CLEAR_RANGE:
                mTempStats.clearRange();
                mHrStats.clearRange();
                sendUpdate();
                break;

            default:
                break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }

        if (mSensing) {
            tick();
        }

        // Unsigned subtraction stays right across the millisecond clock's wrap.
        if (!mGuiStarted && mKernel.sys.getTimeMs() - bootMs >= skNoGuiGraceMs) {
            LOG_INFO("No GUI: exiting\n");
            stopSensors("no screen opened");
            return;
        }
    }
}

// -- Sensors -------------------------------------------------------------------

void Service::startSensors()
{
    mSensing     = true;
    mFinished    = false;
    mSimulated   = false;
    mStartMs     = mKernel.sys.getTimeMs();
    mElapsedS    = 0;
    mLastFlushS  = 0;
    mTempStats   = Sauna::StreamStats();
    mHrStats     = Sauna::StreamStats();
    mTempBad     = 0;
    mTempFields  = 0;
    mTrustX100   = Sauna::kX100Invalid;
    mMarks       = 0;
    mPhase       = Sauna::ProbePhase::Ambient;
    mHint        = Sauna::HeatHint(probeHintConfig());
    sCsvLen      = 0;

    openLog();

    // Default period and latency, as the SDK's Sensors tutorial connects them:
    // the probe is there to see what the sensors do on their own.
    mTempSense = mTemp.connect() ? Sauna::Sense::NoData : Sauna::Sense::ConnectFailed;
    mHrSense   = mHr.connect() ? Sauna::Sense::NoData : Sauna::Sense::ConnectFailed;

    std::snprintf(sLine, sizeof(sLine), "sensors: temperature %s, heart rate %s",
                  Sauna::senseName(mTempSense), Sauna::senseName(mHrSense));
    note(sLine);
}

void Service::stopSensors(const char *why)
{
    if (!mSensing) {
        flushLog();
        return;
    }
    mSensing  = false;
    mFinished = true;
    mTemp.disconnect();
    mHr.disconnect();

    std::snprintf(sLine, sizeof(sLine),
                  "stopped at t=%lu s (%s): temperature samples %lu, rejected %lu; heart rate samples %lu",
                  static_cast<unsigned long>(mElapsedS), why,
                  static_cast<unsigned long>(mTempStats.count()), static_cast<unsigned long>(mTempBad),
                  static_cast<unsigned long>(mHrStats.count()));
    note(sLine);
    flushLog();
}

void Service::tick()
{
    // Unsigned subtraction stays right across the millisecond clock's wrap.
    const uint32_t nowMs   = mKernel.sys.getTimeMs();
    const uint32_t elapsed = (nowMs - mStartMs) / 1000u;
    if (elapsed == mElapsedS) {
        return;
    }
    mElapsedS = elapsed;

#if defined(SIMULATOR)
    // The simulator has no temperature sensor: make a reading up, and say so.
    if (mTempSense != Sauna::Sense::Ok) {
        mSimulated  = true;
        mTempFields = 1;
        mTempStats.add(nowMs, Sauna::syntheticTempX100(elapsed));
    }
#endif

    if (mTempStats.has()) {
        mHint.add(nowMs, mTempStats.last());
    }

    Sauna::CsvRow row;
    row.tMs        = nowMs - mStartMs;
    row.tS         = elapsed;
    row.mark       = mMarks;
    row.tempX100   = mTempStats.last();
    row.tempN      = mTempStats.count();
    row.tempBad    = mTempBad;
    row.tempFields = mTempFields;
    row.tempGapMs  = mTempStats.lastGapMs();
    row.hrN        = mHrStats.count();
    row.hr         = bpmFromX100(mHrStats.last());
    row.trustX100  = mTrustX100;
    row.phase         = static_cast<uint8_t>(mPhase);
    row.hintDeltaX100 = mHint.deltaX100();
    row.hintLevel     = static_cast<uint8_t>(mHint.level());
    row.hintTrend     = static_cast<uint8_t>(mHint.trend());
    const size_t len = Sauna::formatCsvRow(sLine, sizeof(sLine), row);
    queue(sLine, len);

    if (elapsed - mLastFlushS >= kFlushEveryS) {
        flushLog();
        mLastFlushS = elapsed;
    }

    if (elapsed >= kWindowS) {
        stopSensors("time limit");
    }
    if (mGuiStarted) {
        sendUpdate();
    }
}

void Service::onSensorData(uint16_t handle, SDK::Sensor::DataBatch &batch)
{
    if (!mSensing) {
        return;
    }
    if (mTemp.matchesDriver(handle)) {
        onTemperature(batch);
    } else if (mHr.matchesDriver(handle)) {
        onHeartRate(batch);
    }
}

void Service::onTemperature(SDK::Sensor::DataBatch &batch)
{
    for (uint16_t i = 0; i < batch.size(); ++i) {
        const SDK::Sensor::DataView view = batch[i];
        mTempFields = view.getFieldCount();

        SDK::SensorDataParser::Temperature p(view);
        if (!p.isDataValid()) {
            ++mTempBad;
            continue;
        }
        const int32_t x100 = Sauna::toX100(p.getTemperature());
        if (x100 == Sauna::kX100Invalid) {
            ++mTempBad;
            continue;
        }
        mTempStats.add(p.getTimestamp(), x100);
        mTempSense = Sauna::Sense::Ok;
    }
}

void Service::onHeartRate(SDK::Sensor::DataBatch &batch)
{
    for (uint16_t i = 0; i < batch.size(); ++i) {
        SDK::SensorDataParser::HeartRate p(batch[i]);
        if (!p.isDataValid()) {
            continue;
        }
        const int32_t bpmX100 = Sauna::toX100(p.getBpm());
        if (bpmX100 == Sauna::kX100Invalid) {
            continue;
        }
        mHrStats.add(p.getTimestamp(), bpmX100);
        mTrustX100 = Sauna::toX100(p.getTrustLevel());
        mHrSense   = Sauna::Sense::Ok;
    }
}

// -- To the screen ---------------------------------------------------------------

Sauna::ProbeSnapshot Service::snapshot() const
{
    Sauna::ProbeSnapshot s;
    s.elapsedS     = mElapsedS;
    s.tempX100     = mTempStats.last();
    s.tempMinX100  = mTempStats.min();
    s.tempMaxX100  = mTempStats.max();
    s.tempSamples  = mTempStats.count();
    s.tempBad      = mTempBad;
    s.tempGapMs    = mTempStats.lastGapMs();
    s.tempMinGapMs = mTempStats.minGapMs();
    s.tempMaxGapMs = mTempStats.maxGapMs();
    s.trustX100    = mTrustX100;
    s.hrSamples    = mHrStats.count();
    s.hr           = bpmFromX100(mHrStats.last());
    s.marks        = mMarks;
    s.run          = mRun;
    s.tempFields   = mTempFields;
    s.tempSense    = static_cast<uint8_t>(mTempSense);
    s.hrSense      = static_cast<uint8_t>(mHrSense);
    s.log          = static_cast<uint8_t>(mLog);
    s.flags        = static_cast<uint8_t>((mSimulated ? Sauna::kFlagSimulated : 0u) |
                                          (mFinished ? Sauna::kFlagFinished : 0u) |
                                          (mHint.baselineReady() ? Sauna::kFlagHintReady : 0u));
    s.phase          = static_cast<uint8_t>(mPhase);
    s.hintDeltaX100  = mHint.deltaX100();
    s.hintLevel      = static_cast<uint8_t>(mHint.level());
    s.hintTrend      = static_cast<uint8_t>(mHint.trend());
    return s;
}

void Service::sendUpdate()
{
    SDK::send_msg<CustomMessage::ProbeUpdate>(mKernel, snapshot());
}

void Service::buzz(uint8_t effect)
{
    auto vibro = SDK::make_msg<SDK::Message::RequestVibroPlay>(mKernel);
    if (!vibro) {
        return;
    }
    vibro->notes[0].effect = effect;
    vibro->notesCount = 1;
    vibro.send();
}

// -- The log -----------------------------------------------------------------------

void Service::openLog()
{
    const bool existed = mKernel.fs.exist(kLogFile);

    // Number this run after the ones already in the file.
    Sauna::RunCounter runs;
    if (existed) {
        if (auto file = mKernel.fs.file(kLogFile); file && file->open(false, false)) {
            size_t got = 0;
            while (file->read(sChunk, sizeof(sChunk), got) && got > 0) {
                runs.feed(sChunk, got);
            }
            file->close();
        }
    }
    mRun = static_cast<uint16_t>(runs.runs() + 1u);

    // 80 bytes, not the 20 the text needs: the compiler must be able to see
    // that even six full-width ints fit, or it warns (-Wformat-truncation).
    char stamp[80];
    clockStamp(stamp, sizeof(stamp));
    const size_t len = Sauna::formatRunLine(sLine, sizeof(sLine), mRun, stamp);
    queue(sLine, len);
    // The columns at the start of every run, not only a new file: they grew
    // between versions, so a file that spans two must say which is which.
    const size_t headerLen = Sauna::formatCsvHeader(sLine, sizeof(sLine));
    queue(sLine, headerLen);
    LOG_INFO("run %u, log %s\n", static_cast<unsigned>(mRun), existed ? "exists" : "is new");
}

void Service::queue(const char *text, size_t len)
{
    if (len == 0 || len >= kCsvCap) {
        return;
    }
    if (sCsvLen + len > kCsvCap) {
        flushLog();
    }
    std::memcpy(sCsv + sCsvLen, text, len);
    sCsvLen += len;
}

void Service::note(const char *text)
{
    char line[192];
    const size_t len = Sauna::formatNoteLine(line, sizeof(line), text);
    queue(line, len);
    LOG_INFO("%s\n", text);
}

void Service::flushLog()
{
    if (sCsvLen == 0) {
        return;
    }
    const bool ok = appendToFile(sCsv, sCsvLen);
    mLog = ok ? Sauna::LogState::Ok : Sauna::LogState::Failed;
    if (!ok) {
        // The rows are dropped, not kept: a log that cannot be written must not
        // grow in memory. The screen says so.
        LOG_ERROR("Cannot write %s\n", kLogFile);
    }
    sCsvLen = 0;
}

bool Service::appendToFile(const char *text, size_t len)
{
    auto file = mKernel.fs.file(kLogFile);
    if (!file) {
        return false;
    }
    if (!file->open(true, false)) {
        // Appending to a file that is not there yet: create it.
        if (mKernel.fs.exist(kLogFile) || !file->open(true, true)) {
            return false;
        }
    }
    size_t written = 0;
    file->seek(file->size());
    const bool ok = file->write(text, len, written) && written == len;
    file->close();
    return ok;
}
