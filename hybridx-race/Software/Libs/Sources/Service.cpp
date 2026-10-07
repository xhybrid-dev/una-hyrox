/**
 ******************************************************************************
 * @file    Service.cpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   Background process: race logic, sensors, FIT writing, persistence.
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstring>
#include <ctime>
#include <memory>
#include <new>

#include "SDK/Messages/AccessoryMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryMetrics.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserFusionRaw.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserWristMotion.hpp"
#include "SDK/Timer/Timer.hpp"
#include "SDK/Tools/FirmwareVersion.hpp"
#include "SDK/Utils/Utils.hpp"

#include "AppConfigFields.hpp"
#include "LapTiming.hpp"

#define LOG_MODULE_PRX   "Service"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{

/// Milliseconds to whole seconds, rounded down.
constexpr std::time_t msToSec(uint32_t ms)
{
    return static_cast<std::time_t>(ms / 1000u);
}

constexpr uint8_t kSpoolHeartRate = 1u << 0;
constexpr uint8_t kSpoolBattery   = 1u << 1;

}  // namespace

Service::Service(SDK::Kernel &kernel)
        : mKernel(kernel)
        , mSettingsSerializer(mKernel, "settings.json")
        , mActivitySummarySerializer(mKernel, "Activity/summary.json")
        , mActivityWriter(mKernel, "Activity")
        , mSensorHr(SDK::Sensor::Type::HEART_RATE_EX, skSamplePeriod, skSampleLatency)
        , mSensorBatteryLevel(SDK::Sensor::Type::BATTERY_LEVEL)
        , mSensorBatteryMetrics(SDK::Sensor::Type::BATTERY_METRICS, skSamplePeriod, skSampleLatency)
        , mSensorWristMotion(SDK::Sensor::Type::WRIST_MOTION)
        , mSensorFusion(SDK::Sensor::Type::FUSION_RAW, 1000.0f / skFusionSampleRateHz, 100)
        , mTimeTracker(kernel.sys)
        , mBatterySoc(kernel.sys)
        , mBatteryVoltage(kernel.sys)
        , mWristTiltDetector()
{
    // Out-of-range samples are excluded from the statistics, so a dropout can
    // never drag the race average down.
    mHrCounter.init(skHrMinValid, skHrMaxValid);

    WristTiltDetector::Config config {};
    config.sampleRateHz = skFusionSampleRateHz;
    mWristTiltDetector.setConfig(config);
    mWristTiltDetector.setListener(this);

    mHrThresholds.fill(0);
}

Service::~Service()
{
    disconnect();
}

uint32_t Service::nowMs() const
{
    return mKernel.sys.getTimeMs();
}

// =============================================================================
// Main loop
// =============================================================================

void Service::run()
{
    LOG_INFO("Started\n");

    mTimeTracker.init();

    if (!mSettingsSerializer.load(mSettings)) {
        LOG_WARNING("Failed to load settings\n");
    }
    if (!mActivitySummarySerializer.load(mSummary)) {
        LOG_WARNING("No previous race summary\n");
    }

    // AppConfig is read here rather than in the constructor: in the simulator
    // the logger does not exist yet at construction time (brief 9.4).
    mConfig.reset(new SDK::AppConfig(mKernel, RaceConfig::kFileName, RaceConfig::kFields,
                                     RaceConfig::kFieldCount));
    loadConfiguration();

    // Repair anything a previous boot left half-written before a new race can
    // start.
    if (mActivityWriter.recoverInterrupted()) {
        LOG_INFO("Recovered an interrupted activity\n");
        notifyNewActivity();
    }

    SDK::Timer guiInitTimeout(TIMER_SECONDS(5));
    guiInitTimeout.start();

    std::time_t processedUtc = 0;

    while (true) {
        SDK::MessageBase *msg = nullptr;
        if (mKernel.comm.getMessage(msg, 500)) {
            switch (msg->getType()) {

            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
                disconnect();
                if (mRace.state() != Race::RaceModel::State::Idle && mFitOpen) {
                    // The kernel is taking the app down: bank what we have
                    // rather than lose it.
                    bankRace();
                }
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
                onStartGUI();
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped\n");
                onStopGUI();
                break;

            case CustomMessage::SETTINGS_SAVE:
                handleEvent(*static_cast<CustomMessage::SettingsSave *>(msg));
                break;

            case CustomMessage::RACE_START:
                handleEvent(*static_cast<CustomMessage::RaceStart *>(msg));
                break;

            case CustomMessage::RACE_SPLIT:
                handleEvent(*static_cast<CustomMessage::RaceSplit *>(msg));
                break;

            case CustomMessage::RACE_UNDO_SPLIT:
                handleUndoSplit();
                break;

            case CustomMessage::RACE_PAUSE:
                handlePause();
                break;

            case CustomMessage::RACE_RESUME:
                handleResume();
                break;

            case CustomMessage::RACE_FINISH_EARLY:
                handleFinishEarly();
                break;

            case CustomMessage::RACE_UNDO_FINISH:
                handleUndoFinish();
                break;

            case CustomMessage::RACE_SAVE:
                handleSave();
                break;

            case CustomMessage::RACE_DISCARD:
                handleDiscard();
                break;

            case CustomMessage::SUMMARY_REQUEST:
                sendSummary();
                break;

            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto *event = static_cast<SDK::Message::Sensor::EventData *>(msg);
                SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                handleSensorsData(event->handle, batch);
            } break;

            case SDK::MessageType::EVENT_ACCESSORY_STATUS: {
                auto *evt = static_cast<SDK::Message::Accessory::EventStatus *>(msg);
                LOG_INFO("Accessory status: state %u\n", evt->state);
                auto out = SDK::make_msg<CustomMessage::AccessoryStatusUpd>(mKernel);
                if (out) {
                    out->state = evt->state;
                    std::strncpy(out->name, evt->name, sizeof(out->name) - 1u);
                    out.send();
                }
            } break;

            default:
                // Unknown messages are ignored, but still released: that is the
                // only forward-compatible shape (Docs/service-lifecycle.md).
                break;
            }

            mKernel.comm.releaseMessage(msg);
        }

        // -- Periodic work -----------------------------------------------------
        const std::time_t utc = mTimeTracker.getExpectedUTC();
        if (processedUtc != utc) {
            processedUtc = utc;

            if (mGuiStarted) {
                const std::tm tmNow = mTimeTracker.getLocalTime(std::time(nullptr));
                auto t = SDK::make_msg<CustomMessage::LocalTime>(mKernel);
                if (t) {
                    t->hour = static_cast<uint8_t>(tmNow.tm_hour);
                    t->minute = static_cast<uint8_t>(tmNow.tm_min);
                    t->second = static_cast<uint8_t>(tmNow.tm_sec);
                    t->month = static_cast<uint8_t>(tmNow.tm_mon + 1);
                    t->day = static_cast<uint8_t>(tmNow.tm_mday);
                    t->weekday = static_cast<uint8_t>(tmNow.tm_wday);
                    t.send();
                }

                SDK::send_msg<CustomMessage::Battery>(
                        mKernel, static_cast<uint8_t>(mBatterySoc.get()));
            }

            // The race is processed whether or not a GUI is watching: an
            // athlete who left the app is still racing.
            if (mRace.state() != Race::RaceModel::State::Idle) {
                processRace();
            }
        }

        // -- Exit conditions ----------------------------------------------------
        if (!mGuiStarted) {
            if (hasWorkOutstanding()) {
                // A race is still running with no GUI. Keep timing, but do not
                // do it forever: after the grace window, save and go.
                const uint32_t goneMs = nowMs() - mGuiGoneAtMs;
                if (mGuiGoneAtMs != 0u && goneMs >= skGuiGoneGraceMs) {
                    LOG_INFO("GUI gone %u ms with a race running: saving and exiting\n",
                             static_cast<unsigned>(goneMs));
                    bankRace();
                    disconnect();
                    return;
                }
            } else if (guiInitTimeout.expired()) {
                // No GUI, nothing outstanding. This is the SDK's exit test with
                // the missing term added (Docs/service-lifecycle.md 5.4).
                LOG_INFO("No race in progress and no GUI, exiting service\n");
                disconnect();
                return;
            }
        }
    }
}

bool Service::hasWorkOutstanding() const
{
    const Race::RaceModel::State s = mRace.state();
    return s == Race::RaceModel::State::Running || s == Race::RaceModel::State::Paused ||
           s == Race::RaceModel::State::Finished;
}

// =============================================================================
// Lifecycle
// =============================================================================

void Service::onStartGUI()
{
    mGuiStarted = true;
    mGuiGoneAtMs = 0u;

    setCapabilities();
    requestAccessoryPrepare();

    if (!mSensorWristMotion.isConnected()) {
        mSensorWristMotion.connect();
    }
    if (!mSensorBatteryLevel.isConnected()) {
        mSensorBatteryLevel.connect();
    }
    if (!mSensorHr.isConnected()) {
        mSensorHr.connect();
    }

    sendInitialInfoToGui();
}

void Service::onStopGUI()
{
    mGuiStarted = false;
    mGuiGoneAtMs = nowMs();

    // A GUI that has gone away cannot show a backlight or a wrist raise.
    mSensorWristMotion.disconnect();

    if (!hasWorkOutstanding()) {
        requestAccessoryRelease();
    }
}

void Service::connectSensors()
{
    // Idempotent and self-healing: only connect what is not connected, so a
    // subscribe that lost the ack race at race start is retried each tick
    // rather than dropped for the whole race.
    if (!mSensorBatteryLevel.isConnected()) {
        mSensorBatteryLevel.connect();
    }
    if (!mSensorBatteryMetrics.isConnected()) {
        mSensorBatteryMetrics.connect();
    }
    if (!mSensorHr.isConnected()) {
        mSensorHr.connect();
    }
    if (!mSensorFusion.isConnected()) {
        mSensorFusion.connect();
    }

    mIsSensorsConnected = true;
}

void Service::disconnect()
{
    if (mIsSensorsConnected || mSensorWristMotion.isConnected()) {
        LOG_DEBUG("Disconnecting sensors\n");
        mSensorFusion.disconnect();
        mSensorHr.disconnect();
        mSensorBatteryLevel.disconnect();
        mSensorBatteryMetrics.disconnect();
        mSensorWristMotion.disconnect();
        mIsSensorsConnected = false;
    }
}

// =============================================================================
// Configuration and settings
// =============================================================================

void Service::loadConfiguration()
{
    if (!mConfig) {
        return;
    }

    // AppConfig is the source of truth for the fields the phone can edit
    // (brief 10.3); the race format stays in our own settings file. A field
    // overrides settings.json only when the values file holds it: an absent
    // one is the app's default, not a choice, and must not undo one made on
    // the watch. That covers a USB install, which has no values file at all,
    // and a values file written before a field existed.
    if (mConfig->has(RaceConfig::kRoxzoneSplits)) {
        mSettings.roxzoneSplits = mConfig->getBool(RaceConfig::kRoxzoneSplits);
    }
    if (mConfig->has(RaceConfig::kRunDistanceM)) {
        mSettings.runDistanceM = Race::clampRunDistanceM(
                static_cast<uint16_t>(mConfig->getInt(RaceConfig::kRunDistanceM)));
    }
    if (mConfig->has(RaceConfig::kSplitLockoutSec)) {
        mSettings.splitLockoutSec =
                static_cast<uint8_t>(mConfig->getInt(RaceConfig::kSplitLockoutSec));
    }
    if (mConfig->has(RaceConfig::kVibrateOnSplit)) {
        mSettings.vibrateOnSplit = mConfig->getBool(RaceConfig::kVibrateOnSplit);
    }

    LOG_INFO("Config: roxzone %u, run %u m, lockout %u s, vibrate %u\n",
             mSettings.roxzoneSplits, mSettings.runDistanceM, mSettings.splitLockoutSec,
             mSettings.vibrateOnSplit);
}

void Service::handleEvent(const CustomMessage::SettingsSave &event)
{
    mSettings = event.settings;

    if (!mSettingsSerializer.save(mSettings)) {
        LOG_WARNING("Failed to save settings\n");
    }

    // Write the phone-visible fields back so the two agree. Last writer wins,
    // which is what brief 10.3 specifies.
    if (mConfig) {
        mConfig->setBool(RaceConfig::kRoxzoneSplits, mSettings.roxzoneSplits);
        mConfig->setInt(RaceConfig::kRunDistanceM, mSettings.runDistanceM);
        mConfig->setInt(RaceConfig::kSplitLockoutSec, mSettings.splitLockoutSec);
        mConfig->setBool(RaceConfig::kVibrateOnSplit, mSettings.vibrateOnSplit);
        if (!mConfig->save()) {
            LOG_WARNING("Failed to write app config\n");
        }
    }

    setCapabilities();
    sendSettings();
}

void Service::sendSettings()
{
    auto msg = SDK::make_msg<CustomMessage::SettingsUpd>(mKernel, mSettings);
    if (msg) {
        msg->isImperial = mIsImperial;
        msg->is12HourFormat = mTimeFormat12h;
        std::memcpy(msg->hrThresholds, mHrThresholds.data(), mHrThresholds.size());
        msg->hrThresholdsCount = mHrThresholdCount;
        msg.send();
    }
}

void Service::sendInitialInfoToGui()
{
    uint8_t thresholds[CustomMessage::kHrThresholdsCount] = {};
    std::memcpy(thresholds, CustomMessage::kHrThresholdsDefault, sizeof(thresholds));

    if (auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel)) {
        if (msg.send(100) && msg.ok()) {
            mIsImperial = msg->imperialUnits;
            mTimeFormat12h = msg->timeFormat;

            if (msg->heartRateCount > CustomMessage::kHrThresholdsCount) {
                msg->heartRateCount = CustomMessage::kHrThresholdsCount;
            }
            if (msg->heartRateCount > 0) {
                uint8_t i = 0;
                for (; i < msg->heartRateCount; ++i) {
                    thresholds[i] = msg->heartRateTh[i];
                }
                // Fill any the profile did not supply, so the zone arc always
                // has a complete ladder.
                for (; i < CustomMessage::kHrThresholdsCount; ++i) {
                    thresholds[i] = (i > 0) ? static_cast<uint8_t>(thresholds[i - 1] + 20)
                                            : CustomMessage::kHrThresholdsDefault[0];
                }
            }
        }
    }

    std::memcpy(mHrThresholds.data(), thresholds, sizeof(thresholds));
    mHrThresholdCount = CustomMessage::kHrThresholdsCount;

    sendSettings();
    SDK::send_msg<CustomMessage::RaceStateUpd>(mKernel, mTrackState);
    SDK::send_msg<CustomMessage::Battery>(mKernel, static_cast<uint8_t>(mBatterySoc.get()));

    // A GUI that reconnects mid-race needs the current state immediately, not
    // at the next tick.
    if (mRace.state() != Race::RaceModel::State::Idle) {
        publishRaceData();
    }
    if (mSummary.valid) {
        sendSummary();
    }
}

// =============================================================================
// Race control
// =============================================================================

void Service::handleEvent(const CustomMessage::RaceStart &event)
{
    startRace(event.format);
}

void Service::startRace(Race::Format format)
{
    if (mRace.state() != Race::RaceModel::State::Idle) {
        LOG_WARNING("Race already in progress\n");
        return;
    }

    Race::RaceModel::Config cfg;
    cfg.format = format;
    cfg.roxzone = mSettings.roxzoneSplits;
    cfg.lockoutMs = mSettings.lockoutMs();

    if (!mRace.start(cfg, nowMs())) {
        LOG_ERROR("Could not start the race\n");
        return;
    }

    mSettings.format = format;
    if (!mSettingsSerializer.save(mSettings)) {
        LOG_WARNING("Failed to remember the race format\n");
    }

    mRaceStartUtc = mTimeTracker.getExpectedUTC();
    mHrCounter.init(skHrMinValid, skHrMaxValid);
    mWristTiltDetector.reset();

    connectSensors();

    ActivityWriter::AppInfo info {};
    info.timestamp = mRaceStartUtc;
    info.appVersion = SDK::ParseVersion(BUILD_VERSION).u32;
    info.devID = DEV_ID;
    info.appID = APP_ID;
    mActivityWriter.start(info);
    mFitOpen = true;
    mSpool.reset(new (std::nothrow) Spool());
    if (!mSpool) {
        LOG_WARNING("No memory for the record spool: records carry flat distance\n");
    }
    emitRaceWorkout();

    mTrackState = Track::State::ACTIVE;
    SDK::send_msg<CustomMessage::RaceStateUpd>(mKernel, mTrackState);

    onSegmentOpened();
    publishRaceData();

    LOG_INFO("Race started: format %u, roxzone %u, %u segments\n",
             static_cast<unsigned>(format), mSettings.roxzoneSplits, mRace.plannedCount());
}

void Service::handleEvent(const CustomMessage::RaceSplit &event)
{
    if (mRace.state() != Race::RaceModel::State::Running) {
        return;
    }

    const uint8_t closingIndex = mRace.currentIndex();

    // The GUI stamped the press; the model decides whether it counts.
    if (!mRace.split(event.pressMs)) {
        LOG_DEBUG("Split ignored by the lockout\n");
        return;  // Brief 7.5 invariant 3: no state change, no feedback.
    }

    closeSpoolSegment();

    const Race::SegmentResult *closed = mRace.recorded(closingIndex);
    const bool finished = (mRace.state() == Race::RaceModel::State::Finished);

    if (closed != nullptr) {
        Track::SplitEvent split {};
        split.desc = closed->desc;
        split.index = closingIndex;
        split.activeMs = closed->activeMs;
        split.raceFinished = finished;
        SDK::send_msg<CustomMessage::SplitEvent>(mKernel, split);
    }

    if (finished) {
        finishRace(true);
    } else {
        onSegmentOpened();
    }

    publishRaceData();
}

void Service::handleUndoSplit()
{
    if (!mRace.undoSplit()) {
        return;
    }
    LOG_INFO("Split undone, back on segment %u\n", mRace.currentIndex());
    backlightOn();
    publishRaceData();
}

void Service::handlePause()
{
    if (!mRace.pause(nowMs())) {
        return;
    }
    // The timer-stop event is written at once and must not land before records
    // that are older than it, so the seconds waiting in the spool go first, at
    // the distance already reached. The rest of the segment ramps after resume.
    if (mSpool) {
        mSpool->close(mSpool->writtenCm(), [this](const SpooledRecord &r, uint32_t cm) {
            writeSpooled(r, cm);
        });
    }
    mActivityWriter.pause(mTimeTracker.getExpectedUTC());
    mTrackState = Track::State::PAUSED;
    SDK::send_msg<CustomMessage::RaceStateUpd>(mKernel, mTrackState);
    publishRaceData();
}

void Service::handleResume()
{
    if (!mRace.resume(nowMs())) {
        return;
    }
    mActivityWriter.resume(mTimeTracker.getExpectedUTC());
    mTrackState = Track::State::ACTIVE;
    SDK::send_msg<CustomMessage::RaceStateUpd>(mKernel, mTrackState);
    publishRaceData();
}

void Service::handleFinishEarly()
{
    if (mRace.state() != Race::RaceModel::State::Running &&
        mRace.state() != Race::RaceModel::State::Paused) {
        return;
    }
    if (!mRace.finishEarly(nowMs())) {
        return;
    }
    closeSpoolSegment();
    finishRace(false);
}

void Service::handleUndoFinish()
{
    if (!mRace.undoFinish()) {
        return;
    }
    LOG_INFO("Finish undone\n");
    mTrackState = Track::State::ACTIVE;
    SDK::send_msg<CustomMessage::RaceStateUpd>(mKernel, mTrackState);
    onSegmentOpened();
    publishRaceData();
}

void Service::handleSave()
{
    saveRace(false);
}

void Service::handleDiscard()
{
    saveRace(true);
}

void Service::finishRace(bool completed)
{
    if (mRace.state() == Race::RaceModel::State::Running ||
        mRace.state() == Race::RaceModel::State::Paused) {
        mRace.finishEarly(nowMs());
    }

    mTrackState = Track::State::INACTIVE;
    SDK::send_msg<CustomMessage::RaceStateUpd>(mKernel, mTrackState);
    SDK::send_msg<CustomMessage::RaceFinished>(mKernel, mRace.completed() && completed);

    backlightOn();
    // One long pulse for the finish (brief 8.3).
    playBuzzerPattern(400, 1);
    if (mSettings.vibrateOnSplit) {
        playVibroPattern(SDK::Message::RequestVibroPlay::Effect::ALERT_1000MS_100);
    }

    LOG_INFO("Race finished: %u of %u segments, completed %u\n", mRace.recordedCount(),
             mRace.plannedCount(), mRace.completed());
}

void Service::bankRace()
{
    // A race still under way ends here, with the finish signal. One already
    // finished was signalled when it finished, so it is only saved: replaying
    // the long buzz minutes later, with the athlete elsewhere, means nothing.
    if (mRace.state() == Race::RaceModel::State::Running ||
        mRace.state() == Race::RaceModel::State::Paused) {
        finishRace(false);
    }
    saveRace(false);
}

void Service::emitRaceWorkout()
{
    // A structured workout describing the race the athlete just started. The
    // Each step carries the segment's name, its distance and that it is work,
    // and each lap points at its step by index. That name is the only route to
    // a labelled lap: the FIT lap message has no name field, and neither Garmin
    // Connect nor Strava displays the developer fields that also identify the
    // segment (NOTES.md 5.9, 5.11).
    Race::SegmentDesc plan[Race::kMaxSegments] = {};
    const uint8_t n = Race::RaceModel::buildTemplate(mSettings.format,
                                                     mSettings.roxzoneSplits,
                                                     plan, Race::kMaxSegments);
    if (n == 0u) {
        return;
    }

    // The names have to outlive addWorkout(), which copies them into the file,
    // so they live here rather than in the loop.
    static char names[Race::kMaxSegments][Race::kMaxNameLen];

    ActivityWriter::WorkoutStepData steps[Race::kMaxSegments] = {};
    for (uint8_t i = 0u; i < n; ++i) {
        const uint16_t metres = Race::RaceModel::distanceM(plan[i], mSettings.runDistanceM);
        Race::RaceModel::name(plan[i], names[i], sizeof(names[i]));
        steps[i].name = names[i];
        steps[i].intensity = SDK::Fit::Intensity::Active;
        if (metres > 0u) {
            steps[i].durationType = SDK::Fit::WktStepDuration::Distance;
            steps[i].durationValue = static_cast<uint32_t>(metres) * 100u;  // cm
        } else {
            // Wall Balls are reps and a Roxzone is however long it takes.
            steps[i].durationType = SDK::Fit::WktStepDuration::Open;
            steps[i].durationValue = 0u;
        }
    }

    // "HYROX-format", as the store description puts it: HYROX describes the
    // format and never names the app (decision D1). Garmin Connect can show
    // this name. The longest, with a shortened run, is 41 bytes.
    const char *shape = "HYROX-format full race";
    switch (mSettings.format) {
    case Race::Format::HalfA: shape = "HYROX-format half, rounds 1-4"; break;
    case Race::Format::HalfB: shape = "HYROX-format half, rounds 5-8"; break;
    case Race::Format::Full:
    default:                  break;
    }

    // A shortened run makes this a simulation rather than the race, and the
    // file should say so plainly.
    char name[48];
    if (mSettings.runDistanceM == Race::kRunDistanceDefaultM) {
        snprintf(name, sizeof(name), "%s", shape);
    } else {
        snprintf(name, sizeof(name), "%s, %u m runs", shape,
                 static_cast<unsigned>(mSettings.runDistanceM));
    }

    mActivityWriter.addWorkout(name, steps, n);
}

void Service::saveRace(bool discard)
{
    if (!mFitOpen) {
        return;
    }

    if (discard) {
        mActivityWriter.discard();
        mFitOpen = false;
        mSpool.reset();
        mRace.discard();
        mRace.reset();
        LOG_INFO("Race discarded\n");
        disconnect();
        return;
    }

    // Every record goes to the file before the first lap does, so any seconds
    // still waiting in the spool are written now. Normally there are none (the
    // last split or the early finish drained it); this catches a save that
    // reaches here any other way.
    closeSpoolSegment();
    mSpool.reset();

    // Laps are written here, not at split time, because a split can be undone
    // until the race is saved (brief 10.1). Chronological order, all before the
    // session message -- proven to decode in Phase 0 (NOTES.md 0.8).
    const std::time_t startUtc = mRaceStartUtc;
    uint32_t cursorMs = 0u;
    uint32_t raceDistanceM = 0u;

    for (uint8_t i = 0u; i < mRace.recordedCount(); ++i) {
        const Race::SegmentResult *seg = mRace.recorded(i);
        if (seg == nullptr) {
            continue;
        }

        const uint32_t wallMs = seg->activeMs + seg->pausedMs;

        // Both ends rounded, then differenced, so every lap starts where the
        // last one ended and the laps add up to the session (LapTiming.hpp).
        const Race::LapSeconds secs = Race::lapSeconds(cursorMs, seg->activeMs, seg->pausedMs);

        ActivityWriter::LapData lap {};
        lap.timeStart = startUtc + secs.startSec;
        lap.timestamp = startUtc + secs.startSec + secs.elapsedSec;
        lap.duration = secs.activeSec;
        lap.elapsed = secs.elapsedSec;
        lap.hrAvg = static_cast<float>(seg->hrAvg());
        lap.hrMax = static_cast<float>(seg->hrMax);
        lap.segmentType = static_cast<uint8_t>(seg->desc.type);
        lap.round = seg->desc.round;
        lap.stationId = seg->desc.stationId;
        lap.distanceM = Race::RaceModel::distanceM(seg->desc, mSettings.runDistanceM);
        // The plan and the laps run in step, so segment i is step i. A race
        // ended early simply stops referencing the rest of the plan.
        lap.wktStepIndex = i;
        raceDistanceM += lap.distanceM;

        mActivityWriter.addLap(lap);
        cursorMs += wallMs;
    }

    const uint32_t totalActive = mRace.totalActiveMs(nowMs());
    const uint32_t totalElapsed = mRace.totalElapsedMs(nowMs());

    ActivityWriter::TrackData track {};
    track.timeStart = startUtc;
    track.timestamp = startUtc + msToSec(totalElapsed);
    track.duration = msToSec(totalActive);
    track.elapsed = msToSec(totalElapsed);
    // From the laps' own samples, so the race and its laps always agree.
    const Race::HeartRateTotals hr = mRace.heartRateTotals();
    track.hrAvg = static_cast<float>(hr.avg);
    track.hrMax = static_cast<float>(hr.max);
    track.raceFormat = static_cast<uint8_t>(mSettings.format);
    track.roxzoneMode = mSettings.roxzoneSplits ? 1u : 0u;
    track.completed = mRace.completed() ? 1u : 0u;
    track.distanceM = raceDistanceM;
    track.runDistanceM = mSettings.runDistanceM;

    const bool ok = mActivityWriter.stop(track);
    mFitOpen = false;

    if (ok) {
        notifyNewActivity();
        LOG_INFO("Race saved: %u laps\n", mRace.recordedCount());
    } else {
        LOG_ERROR("Failed to write the activity file\n");
    }

    buildSummary();
    if (!mActivitySummarySerializer.save(mSummary)) {
        LOG_WARNING("Failed to save the race summary\n");
    }
    sendSummary();

    mRace.save();
    // Back to Idle, or the next START is refused and the race screen freezes.
    // It also stops processRace() reconnecting the sensors behind the summary.
    mRace.reset();
    disconnect();
}

void Service::onSegmentOpened()
{
    const Race::SegmentDesc *seg = mRace.currentSegment();
    if (seg == nullptr) {
        return;
    }

    backlightOn();
    notifySegment(seg->type);
}

void Service::notifySegment(Race::SegmentType type)
{
    // Brief 8.3: a run is one strong pulse, a station two, a Roxzone one short.
    // The buzzer carries the same shape for anyone who can hear it.
    using Effect = SDK::Message::RequestVibroPlay::Effect;

    switch (type) {
    case Race::SegmentType::Run:
        playBuzzerPattern(150, 1);
        if (mSettings.vibrateOnSplit) {
            playVibroPattern(Effect::STRONG_CLICK_100, 1);
        }
        break;

    case Race::SegmentType::Station:
        playBuzzerPattern(150, 2);
        if (mSettings.vibrateOnSplit) {
            playVibroPattern(Effect::STRONG_CLICK_100, 2);
        }
        break;

    case Race::SegmentType::RoxIn:
    case Race::SegmentType::RoxOut:
        playBuzzerPattern(80, 1);
        if (mSettings.vibrateOnSplit) {
            playVibroPattern(Effect::SHARP_TICK_1_100, 1);
        }
        break;

    default:
        break;
    }
}

void Service::processRace()
{
    connectSensors();  // self-healing, see the comment there

    if (mRace.state() == Race::RaceModel::State::Running) {
        // A FIT record a second, gated exactly as the SDK's apps gate it.
        const ActivityWriter::RecordData record = prepareRecordData();
        recordSecond(record);

        const float hr = mHrCounter.getCurrent();
        if (hr > skHrMinValid && mHrTrust >= 1u && mHrTrust <= 3u) {
            mRace.addHeartRate(static_cast<uint8_t>(hr));
        }
    }

    publishRaceData();
}

uint32_t Service::completedDistanceCm() const
{
    uint32_t cm = 0u;
    for (uint8_t i = 0u; i < mRace.recordedCount(); ++i) {
        const Race::SegmentResult *seg = mRace.recorded(i);
        if (seg != nullptr) {
            cm += static_cast<uint32_t>(
                          Race::RaceModel::distanceM(seg->desc, mSettings.runDistanceM)) * 100u;
        }
    }
    return cm;
}

void Service::recordSecond(const ActivityWriter::RecordData &record)
{
    if (!mSpool) {
        ActivityWriter::RecordData direct = record;
        direct.distanceCm = completedDistanceCm();
        mActivityWriter.addRecord(direct);
        return;
    }

    SpooledRecord s {};
    s.timestamp = static_cast<uint32_t>(record.timestamp);
    s.batteryVoltage = record.batteryVoltage;
    s.flags = static_cast<uint8_t>(
            (record.has(ActivityWriter::RecordData::Field::HEART_RATE) ? kSpoolHeartRate : 0u) |
            (record.has(ActivityWriter::RecordData::Field::BATTERY) ? kSpoolBattery : 0u));
    s.heartRate = static_cast<uint8_t>(record.heartRate);
    s.hrSource = record.hrSource;
    s.hrOpticalBpm = record.hrOpticalBpm;
    s.hrExternalBpm = record.hrExternalBpm;
    s.batteryLevel = record.batteryLevel;

    // Only a full spool writes from here: the oldest second, at the distance
    // already reached.
    mSpool->add(s, [this](const SpooledRecord &r, uint32_t cm) {
        writeSpooled(r, cm);
    });
}

void Service::writeSpooled(const SpooledRecord &s, uint32_t distanceCm)
{
    ActivityWriter::RecordData r {};
    r.timestamp = static_cast<std::time_t>(s.timestamp);
    r.set(ActivityWriter::RecordData::Field::HEART_RATE, (s.flags & kSpoolHeartRate) != 0u);
    r.set(ActivityWriter::RecordData::Field::BATTERY, (s.flags & kSpoolBattery) != 0u);
    r.heartRate = static_cast<float>(s.heartRate);
    r.hrSource = s.hrSource;
    r.hrOpticalBpm = s.hrOpticalBpm;
    r.hrExternalBpm = s.hrExternalBpm;
    r.batteryLevel = s.batteryLevel;
    r.batteryVoltage = s.batteryVoltage;
    r.distanceCm = distanceCm;
    mActivityWriter.addRecord(r);
}

void Service::closeSpoolSegment()
{
    if (!mSpool) {
        return;
    }
    mSpool->close(completedDistanceCm(), [this](const SpooledRecord &r, uint32_t cm) {
        writeSpooled(r, cm);
    });
}

void Service::publishRaceData()
{
    const uint32_t now = nowMs();

    mRaceData = Track::Data {};

    const Race::SegmentDesc *cur = mRace.currentSegment();
    if (cur != nullptr) {
        mRaceData.current = *cur;
    }
    const Race::SegmentDesc *next = mRace.nextSegment();
    if (next != nullptr) {
        mRaceData.next = *next;
        mRaceData.hasNext = true;
    }

    mRaceData.segmentIndex = mRace.currentIndex();
    mRaceData.segmentCount = mRace.plannedCount();
    mRaceData.segmentMs = mRace.currentSegmentActiveMs(now);
    mRaceData.totalMs = mRace.totalActiveMs(now);
    mRaceData.elapsedMs = mRace.totalElapsedMs(now);

    mRaceData.hr = static_cast<uint8_t>(mHrCounter.getCurrent());
    mRaceData.hrTrust = mHrTrust;
    mRaceData.hrSource = mHrSource;
    mRaceData.hrAvg = static_cast<uint8_t>(mHrCounter.getAverage());
    mRaceData.hrMax = static_cast<uint8_t>(mHrCounter.getMaximum());
    mRaceData.completed = mRace.completed();

    if (mGuiStarted) {
        SDK::send_msg<CustomMessage::RaceDataUpd>(mKernel, mRaceData);
    }
}

ActivityWriter::RecordData Service::prepareRecordData()
{
    ActivityWriter::RecordData record {};
    record.timestamp = mTimeTracker.getExpectedUTC();

    // The trust gate is for what goes into the file; the live readout on screen
    // is deliberately ungated (brief 14.12).
    const float hr = mHrCounter.getCurrent();
    const bool hasHeartRate = (hr > skHrMinValid && mHrTrust >= 1u && mHrTrust <= 3u);

    record.set(ActivityWriter::RecordData::Field::HEART_RATE, hasHeartRate);
    record.heartRate = hr;
    record.hrSource = hasHeartRate ? mHrSource : 0u;
    record.hrOpticalBpm = mHrOpticalBpm;
    record.hrExternalBpm = mHrExternalBpm;

    // Both battery samples must be checked every call; evaluate separately so
    // short-circuiting cannot skip one.
    const bool socReady = mBatterySoc.isDue();
    const bool voltReady = mBatteryVoltage.isDue();
    if (socReady && voltReady) {
        record.set(ActivityWriter::RecordData::Field::BATTERY);
        record.batteryLevel = static_cast<uint8_t>(mBatterySoc.get());
        record.batteryVoltage = static_cast<uint16_t>(mBatteryVoltage.get() * 1000.0f);
    }

    return record;
}

// =============================================================================
// Summary
// =============================================================================

void Service::buildSummary()
{
    mSummary = ActivitySummary {};

    mSummary.format = mSettings.format;
    mSummary.roxzone = mSettings.roxzoneSplits;
    mSummary.completed = mRace.completed();
    mSummary.startUtc = mRaceStartUtc;
    mSummary.totalMs = mRace.totalActiveMs(nowMs());
    mSummary.runsMs = mRace.totalActiveMsOfType(Race::SegmentType::Run);
    mSummary.stationsMs = mRace.totalActiveMsOfType(Race::SegmentType::Station);
    mSummary.roxzoneMs = mRace.totalActiveMsOfType(Race::SegmentType::RoxIn) +
                         mRace.totalActiveMsOfType(Race::SegmentType::RoxOut);
    const Race::HeartRateTotals hr = mRace.heartRateTotals();
    mSummary.hrAvg = hr.avg;
    mSummary.hrMax = hr.max;

    uint8_t n = 0u;
    for (uint8_t i = 0u; i < mRace.recordedCount() && n < Race::kMaxSegments; ++i) {
        const Race::SegmentResult *seg = mRace.recorded(i);
        if (seg == nullptr) {
            continue;
        }
        SegmentSummary &out = mSummary.segments[n];
        out.type = static_cast<uint8_t>(seg->desc.type);
        out.round = seg->desc.round;
        out.stationId = seg->desc.stationId;
        out.durationMs = seg->activeMs;
        out.hrAvg = seg->hrAvg();
        out.hrMax = seg->hrMax;
        ++n;
    }

    mSummary.count = n;
    mSummary.valid = (n > 0u);
}

void Service::sendSummary()
{
    if (!mGuiStarted || !mSummary.valid) {
        return;
    }

    auto meta = SDK::make_msg<CustomMessage::SummaryMeta>(mKernel);
    if (!meta) {
        return;
    }
    meta->format = mSummary.format;
    meta->roxzone = mSummary.roxzone;
    meta->completed = mSummary.completed;
    meta->segmentCount = mSummary.count;
    meta->totalMs = mSummary.totalMs;
    meta->runsMs = mSummary.runsMs;
    meta->stationsMs = mSummary.stationsMs;
    meta->roxzoneMs = mSummary.roxzoneMs;
    meta->hrAvg = mSummary.hrAvg;
    meta->hrMax = mSummary.hrMax;
    meta.send();

    // Paged, because 31 segments do not fit a 256-byte pool block and an
    // oversized send fails silently.
    for (uint8_t first = 0u; first < mSummary.count;
         first = static_cast<uint8_t>(first + CustomMessage::SummaryPage::kEntriesPerPage)) {

        auto page = SDK::make_msg<CustomMessage::SummaryPage>(mKernel);
        if (!page) {
            LOG_WARNING("Could not allocate a summary page\n");
            return;
        }

        page->firstIndex = first;
        uint8_t n = 0u;
        for (; n < CustomMessage::SummaryPage::kEntriesPerPage &&
               (first + n) < mSummary.count;
             ++n) {
            const SegmentSummary &s = mSummary.segments[first + n];
            page->entries[n].desc.type = static_cast<Race::SegmentType>(s.type);
            page->entries[n].desc.round = s.round;
            page->entries[n].desc.stationId = s.stationId;
            page->entries[n].activeMs = s.durationMs;
            page->entries[n].hrAvg = s.hrAvg;
            page->entries[n].hrMax = s.hrMax;
        }
        page->count = n;
        page.send();
    }
}

// =============================================================================
// Sensors
// =============================================================================

void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch &data)
{
    if (mSensorHr.matchesDriver(handle)) {
        SDK::SensorDataParser::HeartRateEx parser(data[0]);
        if (parser.isDataValid()) {
            mHrCounter.add(parser.getBpm());
            mHrTrust = static_cast<uint8_t>(parser.getTrustLevel());
            mHrSource = static_cast<uint8_t>(parser.getSource());
            mHrOpticalBpm = static_cast<uint8_t>(parser.getOpticalBpm());
            mHrExternalBpm = static_cast<uint8_t>(parser.getExternalBpm());

            if (mGuiStarted && mRace.state() == Race::RaceModel::State::Idle) {
                SDK::send_msg<CustomMessage::HrUpdate>(
                        mKernel, static_cast<uint8_t>(mHrCounter.getCurrent()), mHrSource);
            }
        }
    } else if (mSensorBatteryLevel.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryLevel parser(data[0]);
        if (parser.isDataValid()) {
            mBatterySoc.set(parser.getCharge());
        }
    } else if (mSensorBatteryMetrics.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryMetrics parser(data[0]);
        if (parser.isDataValid()) {
            mBatteryVoltage.set(parser.getVoltage());
        }
    } else if (mSensorWristMotion.matchesDriver(handle)) {
        SDK::SensorDataParser::WristMotion parser(data[0]);
        if (parser.isDataValid()) {
            backlightOn();
        }
    } else if (mSensorFusion.matchesDriver(handle)) {
        // Batched: this is the one sensor that delivers more than one sample.
        static constexpr uint16_t kBatchSize = 10u;
        TiltImuSample batch[kBatchSize];
        uint16_t batchLen = 0u;

        for (uint16_t i = 0u; i < data.size(); ++i) {
            SDK::SensorDataParser::FusionRaw parser(data[i]);
            if (parser.isDataValid()) {
                SDK::SensorDataParser::FusionRaw::Data sample {};
                parser.getData(sample);
                batch[batchLen].ayLsb = sample.accel.y;
                batch[batchLen].azLsb = sample.accel.z;
                batch[batchLen].gxLsb = sample.gyro.x;
                batch[batchLen].timestampMs = parser.getTimestamp();
                ++batchLen;

                if (batchLen == kBatchSize) {
                    mWristTiltDetector.addBatch(batch, kBatchSize);
                    batchLen = 0u;
                }
            }
        }

        if (batchLen > 0u) {
            mWristTiltDetector.addBatch(batch, batchLen);
        }
    }
}

void Service::onWristTilt(uint32_t /*timestampMs*/)
{
    backlightOn();
}

// =============================================================================
// Notifications
// =============================================================================

void Service::setCapabilities()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::RequestSetCapabilities>();
    if (msg) {
        msg->enPhoneNotification = true;
        msg->enUsbChargingScreen = false;
        msg->enMusicControl = true;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::requestAccessoryPrepare()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::Accessory::RequestPrepare>();
    if (msg) {
        msg->kinds = SDK::Accessory::Kind::HRM;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::requestAccessoryRelease()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::Accessory::RequestRelease>();
    if (msg) {
        msg->kinds = 0;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::notifyNewActivity()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::CommandAppNewActivity>();
    if (msg) {
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::backlightOn(uint32_t timeoutMs)
{
    auto bl = SDK::make_msg<SDK::Message::RequestBacklightSet>(mKernel);
    if (bl) {
        bl->brightness = 100;
        bl->autoOffTimeoutMs = timeoutMs;
        bl.send();
    }
}

void Service::playBuzzerPattern(uint16_t beepMs, uint8_t count, uint16_t silenceMs)
{
    if (count == 0u) {
        return;
    }

    // N beeps need 2N-1 notes; the pool caps that at 5 beeps.
    const uint8_t maxCount = (SDK::Message::RequestBuzzerPlay::skMaxNotes + 1u) / 2u;
    if (count > maxCount) {
        count = maxCount;
    }

    auto *msg = mKernel.comm.allocateMessage<SDK::Message::RequestBuzzerPlay>();
    if (msg) {
        uint8_t n = 0u;
        for (uint8_t i = 0u; i < count; ++i) {
            msg->notes[n].volume = 100;
            msg->notes[n].time = beepMs;
            ++n;
            if (i < count - 1u) {
                msg->notes[n].volume = 0;
                msg->notes[n].time = silenceMs;
                ++n;
            }
        }
        msg->notesCount = n;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::playVibroPattern(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count,
                               uint16_t silenceMs)
{
    if (count == 0u) {
        return;
    }

    // N effects need 2N-1 notes; the pool caps that at 4 effects.
    const uint8_t maxCount = (SDK::Message::RequestVibroPlay::skMaxNotes + 1u) / 2u;
    if (count > maxCount) {
        count = maxCount;
    }

    auto *msg = mKernel.comm.allocateMessage<SDK::Message::RequestVibroPlay>();
    if (msg) {
        uint8_t n = 0u;
        for (uint8_t i = 0u; i < count; ++i) {
            msg->notes[n].effect = static_cast<uint8_t>(effect);
            msg->notes[n].pause = 0;
            ++n;
            if (i < count - 1u) {
                msg->notes[n].effect =
                        static_cast<uint8_t>(SDK::Message::RequestVibroPlay::Effect::NO_EFFECT);
                msg->notes[n].pause = silenceMs;
                ++n;
            }
        }
        msg->notesCount = n;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}
