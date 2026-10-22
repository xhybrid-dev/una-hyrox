/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   X Sauna background logic: runs a sauna session and records it
 *          (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstdio>
#include <cstring>

#include "FixedPoint.hpp"
#include "SessionFit.hpp"
#include "SimTemp.hpp"

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserTemperature.hpp"
#include "SDK/Tools/FirmwareVersion.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace
{
/// The session and its laps (about 4 KB) live in static storage, not on the
/// service's small stack.
Sauna::SessionModel sModel;
Sauna::FitLap       sFitLaps[Sauna::kMaxLaps];
} // namespace

Service::Service(SDK::Kernel &kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mWriter(mKernel, "Activity")
    , mTimeTracker(mKernel.sys)
    , mHrSensor(SDK::Sensor::Type::HEART_RATE_EX, skSamplePeriod, skSampleLatency)
    , mTempSensor(SDK::Sensor::Type::AMBIENT_TEMPERATURE, 0, 0)
{
    (void)kernel;
}

bool Service::sessionActive() const
{
    const Sauna::RunState s = sModel.state();
    return s == Sauna::RunState::Running || s == Sauna::RunState::Paused;
}

void Service::run()
{
    LOG_INFO("thread started\n");
    mTimeTracker.init();

    // Finish anything a previous boot left half-written before a new session
    // can start.
    if (mWriter.recoverInterrupted()) {
        LOG_INFO("Recovered an interrupted activity\n");
        notifyNewActivity();
    }

    const uint32_t bootMs = mKernel.sys.getTimeMs();

    while (true) {
        SDK::MessageBase *msg = nullptr;
        if (mKernel.comm.getMessage(msg, skWaitMs)) {
            switch (msg->getType()) {
            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
                // The kernel is taking the app down: bank what we have.
                finish(true, true);
                disconnectSensors();
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
                mGuiStarted  = true;
                mGuiGoneAtMs = 0;
                sendUpdate();
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped\n");
                mGuiStarted = false;
                if (!sessionActive()) {
                    // Nothing is running and nothing else will reclaim the
                    // service: leave, as Knitting does.
                    mKernel.comm.releaseMessage(msg);
                    disconnectSensors();
                    return;
                }
                // A session is running: keep recording (the screen turning off
                // is not the end of a sauna). Saved if the screen stays away.
                mGuiGoneAtMs = mKernel.sys.getTimeMs();
                if (mGuiGoneAtMs == 0) {
                    mGuiGoneAtMs = 1;
                }
                break;

            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto *event = static_cast<SDK::Message::Sensor::EventData *>(msg);
                SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                onSensorData(event->handle, batch);
            } break;

            case CustomMessage::SESSION_START: {
                auto *m = static_cast<CustomMessage::SessionStart *>(msg);
                startSession(m->mode, m->rest);
            } break;

            case CustomMessage::SESSION_LAP:
                lap();
                break;

            case CustomMessage::SESSION_PAUSE:
                pause();
                break;

            case CustomMessage::SESSION_RESUME:
                resume();
                break;

            case CustomMessage::SESSION_STOP: {
                auto *m = static_cast<CustomMessage::SessionStop *>(msg);
                finish(m->save != 0, false);
            } break;

            default:
                break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }

        tick();

        // Unsigned subtraction stays right across the millisecond clock's wrap.
        const uint32_t nowMs = mKernel.sys.getTimeMs();
        if (!mGuiStarted) {
            if (sessionActive()) {
                if (mGuiGoneAtMs != 0 && nowMs - mGuiGoneAtMs >= kGuiGoneGraceMs) {
                    LOG_INFO("GUI gone %lu ms with a session running: saving and exiting\n",
                             static_cast<unsigned long>(nowMs - mGuiGoneAtMs));
                    finish(true, true);
                    return;
                }
            } else if (nowMs - bootMs >= skNoGuiGraceMs) {
                LOG_INFO("No session and no GUI: exiting\n");
                disconnectSensors();
                return;
            }
        }
    }
}

// -- The session ---------------------------------------------------------------

void Service::startSession(uint8_t mode, uint8_t rest)
{
    if (sessionActive()) {
        return;
    }

    Sauna::SessionConfig cfg;
    cfg.mode = mode == 0 ? Sauna::Mode::SaunaPlunge : Sauna::Mode::SaunaOnly;
    cfg.rest = rest != 0;

    mStartUtc      = mTimeTracker.getExpectedUTC();
    mSecond.reset();
    mLogOk         = true;
    mHaveHr        = false;
    mTempX100      = Sauna::kX100Invalid;

    connectSensors();

    ActivityWriter::AppInfo info {};
    info.timestamp        = mStartUtc;
    info.appVersion       = SDK::ParseVersion(BUILD_VERSION).u32;
    info.devID            = DEV_ID;
    info.appID            = APP_ID;
    info.sportProfileName = "Sauna";    // Race 5.28's experiment: whether anything honours it is unknown
    mWriter.start(info);
    mFitOpen = true;

    const uint32_t nowMs = mKernel.sys.getTimeMs();
    sModel.start(nowMs, cfg);
    LOG_INFO("Session started: mode %u, rest %u\n", static_cast<unsigned>(mode), static_cast<unsigned>(rest));
    sendUpdate();
}

void Service::lap()
{
    const Sauna::LapResult r = sModel.lap(mKernel.sys.getTimeMs());
    if (r == Sauna::LapResult::Ok) {
        buzz(SDK::Message::RequestVibroPlay::STRONG_CLICK_100);
    } else if (r == Sauna::LapResult::Full) {
        LOG_WARNING("Lap refused: the session is full\n");
        buzz(SDK::Message::RequestVibroPlay::SHORT_DOUBLE_CLICK_STRONG_1_100);
    }
    sendUpdate();
}

void Service::pause()
{
    if (sModel.pause(mKernel.sys.getTimeMs())) {
        mWriter.pause(mTimeTracker.getExpectedUTC());
        sendUpdate();
    }
}

void Service::resume()
{
    if (sModel.resume(mKernel.sys.getTimeMs())) {
        mWriter.resume(mTimeTracker.getExpectedUTC());
        sendUpdate();
    }
}

void Service::finish(bool save, bool bank)
{
    if (!sessionActive()) {
        return;
    }
    const uint32_t nowMs = mKernel.sys.getTimeMs();
    sModel.stop(nowMs);
    disconnectSensors();

    if (!save) {
        mWriter.discard();
        mFitOpen = false;
        LOG_INFO("Session discarded\n");
        sendSaved(false);
        sendUpdate();
        return;
    }

    Sauna::FitSession fs;
    const uint8_t n = Sauna::buildFit(sModel, sFitLaps, Sauna::kMaxLaps, fs);
    for (uint8_t i = 0; i < n; ++i) {
        ActivityWriter::LapData lap {};
        lap.timeStart   = mStartUtc + static_cast<std::time_t>(sFitLaps[i].startSec);
        lap.timestamp   = mStartUtc + static_cast<std::time_t>(sFitLaps[i].startSec + sFitLaps[i].elapsedSec);
        lap.elapsed     = sFitLaps[i].elapsedSec;
        lap.duration    = sFitLaps[i].activeSec;
        lap.hrAvg       = sFitLaps[i].hrAvg;
        lap.hrMax       = sFitLaps[i].hrMax;
        lap.phase       = sFitLaps[i].phase;
        lap.round       = sFitLaps[i].round;
        lap.hasRecovery = sFitLaps[i].hasRecovery;
        lap.recoveryBpm = sFitLaps[i].recoveryBpm;
        mWriter.addLap(lap);
    }

    ActivityWriter::TrackData track {};
    track.timeStart   = mStartUtc;
    track.timestamp   = mStartUtc + static_cast<std::time_t>(fs.elapsedSec);
    track.elapsed     = fs.elapsedSec;
    track.duration    = fs.timerSec;
    track.hrAvg       = fs.hrAvg;
    track.hrMax       = fs.hrMax;
    track.mode        = fs.mode;
    track.restEnabled = fs.rest;
    track.completed   = bank ? 0u : 1u;

    const bool ok = mWriter.stop(track);
    mFitOpen = false;
    if (ok) {
        notifyNewActivity();
        LOG_INFO("Session saved: %u laps\n", static_cast<unsigned>(n));
    } else {
        LOG_ERROR("Failed to write the activity file\n");
    }
    sendSaved(ok);
    sendUpdate();
}

// -- Once a second ---------------------------------------------------------------

void Service::tick()
{
    if (!sessionActive()) {
        return;
    }
    // Each new UTC second, however the 200 ms polling falls: a timer of "a
    // second since the last" drifted slow and dropped about one second in
    // twenty from the file (Jon's first session).
    const std::time_t utc = mTimeTracker.getExpectedUTC();
    if (!mSecond.take(static_cast<int64_t>(utc))) {
        return;
    }
    const uint32_t nowMs = mKernel.sys.getTimeMs();

    const uint16_t hr   = currentHr(nowMs);
    const int32_t  temp = currentTemp(nowMs);

    if (sModel.state() == Sauna::RunState::Running) {
        ActivityWriter::RecordData rec {};
        rec.timestamp     = utc;
        rec.heartRate     = static_cast<uint8_t>(hr > 255u ? 255u : hr);
        rec.watchTempX100 = temp;
        if (!mWriter.addRecord(rec)) {
            mLogOk = false;
        }
        sModel.addSample(nowMs, hr, temp);
    } else {
        // Paused: no records, but a lap's recovery keeps being measured.
        sModel.addSample(nowMs, hr, Sauna::kX100Invalid);
    }

    if (mGuiStarted) {
        sendUpdate();
    }
}

// -- Sensors ---------------------------------------------------------------------

void Service::connectSensors()
{
    if (!mHrSensor.isConnected()) {
        mHrSensor.connect();
    }
    if (!mTempSensor.isConnected()) {
        mTempSensor.connect();
    }
}

void Service::disconnectSensors()
{
    mHrSensor.disconnect();
    mTempSensor.disconnect();
}

void Service::onSensorData(uint16_t handle, SDK::Sensor::DataBatch &batch)
{
    if (batch.size() == 0) {
        return;
    }
    if (mHrSensor.matchesDriver(handle)) {
        SDK::SensorDataParser::HeartRateEx p(batch[0]);
        if (p.isDataValid()) {
            mHrBpm   = p.getBpm();
            mHrTrust = static_cast<uint8_t>(p.getTrustLevel());
            mHrAtMs  = mKernel.sys.getTimeMs();
            mHaveHr  = true;
        }
    } else if (mTempSensor.matchesDriver(handle)) {
        SDK::SensorDataParser::Temperature p(batch[0]);
        if (p.isDataValid()) {
            const int32_t x100 = Sauna::toX100(p.getTemperature());
            if (x100 != Sauna::kX100Invalid) {
                mTempX100 = x100;
                mTempAtMs = mKernel.sys.getTimeMs();
            }
        }
    }
}

uint16_t Service::currentHr(uint32_t nowMs) const
{
    // The gate HybridX Race applies to what goes in the file: a real reading
    // with a trust level of 1 to 3, and not stale.
    if (!mHaveHr || nowMs - mHrAtMs > skHrMaxAgeMs) {
        return 0;
    }
    if (mHrBpm <= skHrMinValid || mHrTrust < 1u || mHrTrust > 3u) {
        return 0;
    }
    const int32_t bpm = static_cast<int32_t>(mHrBpm + 0.5f);
    return bpm > 255 ? 255u : static_cast<uint16_t>(bpm);
}

int32_t Service::currentTemp(uint32_t nowMs) const
{
    if (mTempX100 != Sauna::kX100Invalid && nowMs - mTempAtMs <= skTempMaxAgeMs) {
        return mTempX100;
    }
#if defined(SIMULATOR)
    // The simulator has no temperature sensor: make a reading up, so the file
    // has something to decode. It never reaches a watch.
    return Sauna::syntheticTempX100(sModel.activeMs(nowMs) / 1000u);
#else
    return Sauna::kX100Invalid;
#endif
}

// -- To the screen ---------------------------------------------------------------

void Service::sendUpdate()
{
    const uint32_t nowMs = mKernel.sys.getTimeMs();
    Sauna::SessionSnapshot s;
    s.state     = static_cast<uint8_t>(sModel.state());
    s.mode      = static_cast<uint8_t>(sModel.config().mode);
    s.rest      = sModel.config().rest ? 1u : 0u;
    s.logOk     = mLogOk ? 1u : 0u;
    if (sModel.lapCount() > 0) {
        s.phase = static_cast<uint8_t>(sModel.phase());
        s.round = sModel.round();
        s.laps  = sModel.lapCount();
        if (sModel.lapCount() > 1) {
            const Sauna::LapStats &prev = sModel.lapAt(static_cast<uint8_t>(sModel.lapCount() - 2));
            s.prevRecovery = static_cast<uint8_t>(prev.recovery.state);
            s.prevDrop     = prev.recovery.drop;
        }
    }
    s.hr        = sessionActive() ? currentHr(nowMs) : 0;
    s.lapMs     = sModel.lapActiveMs(nowMs);
    s.activeMs  = sModel.activeMs(nowMs);
    s.elapsedMs = sModel.elapsedMs(nowMs);
    SDK::send_msg<CustomMessage::SessionUpdate>(mKernel, s);
}

void Service::sendSaved(bool saved)
{
    Sauna::SavedSession s;
    s.saved   = saved ? 1u : 0u;
    s.summary = sModel.summary();
    SDK::send_msg<CustomMessage::SessionSaved>(mKernel, s);
}

void Service::notifyNewActivity()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::CommandAppNewActivity>();
    if (msg) {
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
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
