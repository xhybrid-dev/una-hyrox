/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   HybridX Workout background logic: runs a workout and records it
 *          (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstdio>
#include <cstring>
#include <memory>
#include <new>

#include "ActivityLink.hpp"
#include "SafeFile.hpp"
#include "StreakNow.hpp"
#include "WorkoutFit.hpp"
#include "WorkoutText.hpp"

#include "SDK/Messages/AccessoryMessages.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserWristMotion.hpp"
#include "SDK/Tools/FirmwareVersion.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace
{
/// The workout and its laps (about 3.6 KB) live in static storage, not on the
/// service's small stack.
Workout::WorkoutModel sModel;
Workout::FitLap       sFitLaps[Workout::kMaxLaps];

/// The app's folder name: what Streak shows and keys sessions by.
constexpr const char* kAppFolder = "HybridXWorkout";
} // namespace

Service::Service(SDK::Kernel &kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mWriter(mKernel, "Activity")
    , mTimeTracker(mKernel.sys)
    , mHrSensor(SDK::Sensor::Type::HEART_RATE_EX, skSamplePeriod, skSampleLatency)
    , mBatterySensor(SDK::Sensor::Type::BATTERY_LEVEL)
    , mWristSensor(SDK::Sensor::Type::WRIST_MOTION)
{
    (void)kernel;
}

bool Service::sessionActive() const
{
    const Workout::RunState s = sModel.state();
    return s == Workout::RunState::Running || s == Workout::RunState::Paused;
}

void Service::run()
{
    LOG_INFO("thread started\n");
    mTimeTracker.init();

    // Finish anything a previous boot left half-written before a new workout
    // can start.
    if (mWriter.recoverInterrupted()) {
        LOG_INFO("Recovered an interrupted activity\n");
        notifyNewActivity();
    }
    loadSettings();

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
                if (!mWristSensor.isConnected()) {
                    mWristSensor.connect();
                }
                // Heart rate (and the strap, when external HR is on in the watch's
                // settings) from the start screen on, so both are ready by Start.
                if (!mHrSensor.isConnected()) {
                    mHrSensor.connect();
                }
                requestAccessoryPrepare();
                readSystemSettings();
                sendSettings();
                sendUpdate();
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped\n");
                mGuiStarted = false;
                // A screen that has gone cannot be lit by a wrist raise.
                mWristSensor.disconnect();
                if (!sessionActive()) {
                    // Nothing is running and nothing else will reclaim the
                    // service: leave, as Knitting does.
                    requestAccessoryRelease();
                    mKernel.comm.releaseMessage(msg);
                    disconnectSensors();
                    return;
                }
                // A workout is running: keep recording. Saved if the screen stays away.
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

            case SDK::MessageType::EVENT_ACCESSORY_STATUS: {
                auto *evt = static_cast<SDK::Message::Accessory::EventStatus *>(msg);
                LOG_INFO("Accessory status: state %u\n", static_cast<unsigned>(evt->state));
                auto out = SDK::make_msg<CustomMessage::AccessoryStatusUpd>(mKernel);
                if (out) {
                    out->state = static_cast<uint8_t>(evt->state);
                    std::strncpy(out->name, evt->name, sizeof(out->name) - 1u);
                    out.send();
                }
            } break;

            case CustomMessage::SESSION_START:
                startSession();
                break;

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

            case CustomMessage::SESSION_LAPS_REQUEST: {
                auto *m = static_cast<CustomMessage::SessionLapsRequest *>(msg);
                sendLaps(m->first);
            } break;

            case CustomMessage::SET_AUTO_LAP: {
                auto *m = static_cast<CustomMessage::SetAutoLap *>(msg);
                setAutoLap(m->minutes);
            } break;

            default:
                // Unknown messages are ignored, but still released
                // (Docs/service-lifecycle.md).
                break;
            }

            mKernel.comm.releaseMessage(msg);
        }

        tick();

        // Unsigned subtraction stays right across the millisecond clock's wrap.
        const uint32_t nowMs = mKernel.sys.getTimeMs();
        if (!mGuiStarted) {
            if (sessionActive()) {
                if (mGuiGoneAtMs != 0 && nowMs - mGuiGoneAtMs >= kGuiGoneGraceMs) {
                    LOG_INFO("GUI gone %lu ms with a workout running: saving and exiting\n",
                             static_cast<unsigned long>(nowMs - mGuiGoneAtMs));
                    finish(true, true);
                    return;
                }
            } else if (nowMs - bootMs >= skNoGuiGraceMs) {
                LOG_INFO("No workout and no GUI: exiting\n");
                disconnectSensors();
                return;
            }
        }
    }
}

// -- The workout ---------------------------------------------------------------

void Service::startSession()
{
    if (sessionActive()) {
        return;
    }

    Workout::WorkoutConfig cfg;
    cfg.zones     = mZones;
    cfg.weightG   = mWeightG;
    cfg.autoLapMs = static_cast<uint32_t>(mSettings.autoLapMin) * 60000u;

    mStartUtc = mTimeTracker.getExpectedUTC();
    mSecond.reset();
    mLogOk    = true;
    mBattery  = Workout::kBatteryUnknown;

    connectSensors();

    ActivityWriter::AppInfo info {};
    info.timestamp        = mStartUtc;
    info.appVersion       = SDK::ParseVersion(BUILD_VERSION).u32;
    info.devID            = DEV_ID;
    info.appID            = APP_ID;
    info.sportProfileName = "Workout";   // Race 5.28's experiment: whether anything honours it is unknown
    mWriter.start(info);
    mFitOpen = true;

    const uint32_t nowMs = mKernel.sys.getTimeMs();
    sModel.start(nowMs, cfg);
    LOG_INFO("Workout started: auto lap %u min, weight %lu g\n", static_cast<unsigned>(mSettings.autoLapMin),
             static_cast<unsigned long>(mWeightG));
    sendUpdate();
}

void Service::lap()
{
    const uint32_t nowMs = mKernel.sys.getTimeMs();
    const Workout::LapResult r = sModel.lap(nowMs);
    if (r == Workout::LapResult::Ok) {
        lapEnded(nowMs, false);
    } else if (r == Workout::LapResult::Full) {
        LOG_WARNING("Lap refused: the workout is full\n");
        buzz(SDK::Message::RequestVibroPlay::SHORT_DOUBLE_CLICK_STRONG_1_100);
    }
    sendUpdate();
}

void Service::lapEnded(uint32_t nowMs, bool autoEnded)
{
    (void)nowMs;
    if (sModel.lapCount() < 2) {
        return;
    }
    const uint8_t done = static_cast<uint8_t>(sModel.lapCount() - 2u);
    const Workout::LapStats &l = sModel.lapAt(done);
    Workout::LapEventData ev;
    ev.finishedLap = static_cast<uint8_t>(done + 1u);
    ev.autoEnded   = autoEnded ? 1u : 0u;
    ev.durationMs  = l.activeMs;
    ev.hrAvg       = l.hrAvg();
    buzz(SDK::Message::RequestVibroPlay::STRONG_CLICK_100);
    backlightOn();
    SDK::send_msg<CustomMessage::SessionLapEvent>(mKernel, ev);
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
    mBatterySensor.disconnect();

    if (!save) {
        mWriter.discard();
        mFitOpen = false;
        LOG_INFO("Workout discarded\n");
        sendSaved(false);
        sendUpdate();
        return;
    }

    Workout::FitSession fs;
    const uint8_t n = Workout::buildFit(sModel, sFitLaps, Workout::kMaxLaps, fs);
    for (uint8_t i = 0; i < n; ++i) {
        ActivityWriter::LapData lap {};
        lap.timeStart   = mStartUtc + static_cast<std::time_t>(sFitLaps[i].startSec);
        lap.timestamp   = mStartUtc + static_cast<std::time_t>(sFitLaps[i].startSec + sFitLaps[i].elapsedSec);
        lap.elapsed     = sFitLaps[i].elapsedSec;
        lap.duration    = sFitLaps[i].activeSec;
        lap.hrAvg       = sFitLaps[i].hrAvg;
        lap.hrMax       = sFitLaps[i].hrMax;
        lap.kcal        = sFitLaps[i].kcal;
        lap.restingKcal = sFitLaps[i].restingKcal;
        lap.autoEnded   = sFitLaps[i].autoEnded;
        mWriter.addLap(lap);
    }

    ActivityWriter::TrackData track {};
    track.timeStart     = mStartUtc;
    track.timestamp     = mStartUtc + static_cast<std::time_t>(fs.elapsedSec);
    track.elapsed       = fs.elapsedSec;
    track.duration      = fs.timerSec;
    track.hrAvg         = fs.hrAvg;
    track.hrMax         = fs.hrMax;
    track.kcal          = fs.kcal;
    track.metabolicKcal = fs.restingKcal;

    const bool ok = mWriter.stop(track);
    mFitOpen = false;
    if (ok) {
        notifyNewActivity();
        LOG_INFO("Workout saved: %u laps, %u kcal%s\n", static_cast<unsigned>(n), static_cast<unsigned>(fs.kcal),
                 bank ? " (banked: the app was stopped)" : "");
        writeActivityLink(fs);   // only for a workout whose .fit is safely saved
        sendStreakLine();
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
    // twenty from X Sauna's file.
    const std::time_t utc = mTimeTracker.getExpectedUTC();
    if (!mSecond.take(static_cast<int64_t>(utc))) {
        return;
    }
    const uint32_t nowMs = mKernel.sys.getTimeMs();
    const uint16_t hr = currentHr(nowMs);

    if (sModel.state() == Workout::RunState::Running) {
        ActivityWriter::RecordData rec {};
        rec.timestamp = utc;
        rec.heartRate = static_cast<uint8_t>(hr > 255u ? 255u : hr);
        if (!mWriter.addRecord(rec)) {
            mLogOk = false;
        }
        if (sModel.addSample(nowMs, hr)) {
            LOG_INFO("Auto lap: lap %u\n", static_cast<unsigned>(sModel.lapCount() - 1u));
            lapEnded(nowMs, true);
        }
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
    if (!mBatterySensor.isConnected()) {
        mBatterySensor.connect();
    }
}

void Service::disconnectSensors()
{
    mHrSensor.disconnect();
    mBatterySensor.disconnect();
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
    } else if (mBatterySensor.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryLevel p(batch[0]);
        if (p.isDataValid()) {
            const float level = p.getCharge();
            mBattery = level < 0.0f ? 0u : (level > 100.0f ? 100u : static_cast<uint8_t>(level + 0.5f));
        }
    } else if (mWristSensor.matchesDriver(handle)) {
        SDK::SensorDataParser::WristMotion p(batch[0]);
        if (p.isDataValid()) {
            backlightOn();
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

// -- Settings ----------------------------------------------------------------------

void Service::readSystemSettings()
{
    // As the SDK's Workout example (sendInitialInfoToGui): the profile's weight
    // when it has one, and its zone thresholds, the missing ones filled 20 bpm
    // apart. Read when the screen opens, so a change in the watch's settings
    // applies from the next workout.
    auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel);
    if (!msg || !msg.send(100) || !msg.ok()) {
        LOG_WARNING("System settings not read: the defaults stand\n");
        return;
    }
    if (msg->weightKg > 0.0f && msg->weightKg < 1000.0f) {
        mWeightG = static_cast<uint32_t>(msg->weightKg * 1000.0f + 0.5f);
    }
    const uint8_t count = msg->heartRateCount > Workout::kThresholdCount ? Workout::kThresholdCount
                                                                          : msg->heartRateCount;
    if (count > 0) {
        uint8_t i = 0;
        for (; i < count; ++i) {
            mZones.th[i] = msg->heartRateTh[i];
        }
        for (; i < Workout::kThresholdCount; ++i) {
            const unsigned next = mZones.th[i - 1] + 20u;
            mZones.th[i] = static_cast<uint8_t>(next > 255u ? 255u : next);
        }
        mZones.count = Workout::kThresholdCount;
    }
    LOG_INFO("Weight %lu g; zones %u %u %u %u %u %u\n", static_cast<unsigned long>(mWeightG),
             mZones.th[0], mZones.th[1], mZones.th[2], mZones.th[3], mZones.th[4], mZones.th[5]);
}

void Service::loadSettings()
{
    char buf[Workout::WorkoutSettings::kMaxJsonBytes];
    for (int backup = 0; backup < 2; ++backup) {
        const size_t len = Workout::SafeFile::read(mKernel.fs, Workout::kSettingsPath, backup != 0, buf, sizeof(buf));
        if (len > 0 && mSettings.fromJson(buf, len)) {
            return;
        }
    }
    mSettings = Workout::WorkoutSettings();
}

void Service::saveSettings()
{
    char buf[Workout::WorkoutSettings::kMaxJsonBytes];
    const size_t len = mSettings.toJson(buf, sizeof(buf));
    if (len == 0 || !Workout::SafeFile::write(mKernel.fs, Workout::kSettingsPath, buf, len)) {
        LOG_WARNING("Could not save the settings\n");
    }
}

void Service::setAutoLap(uint8_t minutes)
{
    // Only one of the example's choices; the next workout takes it up.
    mSettings.autoLapMin = Workout::kAutoLapMinutes[Workout::autoLapIndex(minutes)];
    saveSettings();
    sendSettings();
}

// -- The strap ---------------------------------------------------------------------

void Service::requestAccessoryPrepare()
{
    // HybridX Race's: ask the kernel for the heart-rate strap. It does nothing
    // unless external HR is switched on in the watch's settings.
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

// -- HybridX Streak ------------------------------------------------------------------

void Service::writeActivityLink(const Workout::FitSession &fs)
{
    // The workout, in the HybridX activity link's format, for HybridX Streak to
    // count (hybridx-link/docs/ACTIVITY_LINK.md). Its start is the one the .fit
    // is named from. Nothing here may get in the way of the save: a failure is a
    // warning. As Race's and Run's.
    using namespace HybridXLink;

    const std::tm  local    = mTimeTracker.getLocalTime(mStartUtc);
    const uint32_t startUtc = static_cast<uint32_t>(mStartUtc);
    const int16_t  tzMin    = tzOffsetMinutes(startUtc, calendarSeconds(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                                                                       local.tm_hour, local.tm_min, local.tm_sec));
    const Entry e = makeEntry(startUtc, tzMin, static_cast<float>(fs.timerSec), 0.0f, static_cast<float>(fs.hrAvg),
                              static_cast<float>(fs.hrMax), Sport::Workout);
    if (!valid(e)) {
        LOG_WARNING("Not telling Streak about this workout: its start (%lu) is not valid\n",
                    static_cast<unsigned long>(e.startUtc));
        return;
    }

    // Two kMaxBytes buffers (the old file, the new one) on the heap, not the service stack.
    std::unique_ptr<char[]> buf(new (std::nothrow) char[2 * kMaxBytes]);
    if (!buf) {
        LOG_WARNING("No memory to tell Streak about this workout\n");
        return;
    }
    char* const oldText = buf.get();
    char* const newText = buf.get() + kMaxBytes;

    // The old file: the primary if it reads, else its backup, else none (append starts afresh).
    const char* path = slotPath(Slot::Workout);
    size_t oldLen    = Workout::SafeFile::read(mKernel.fs, path, false, oldText, kMaxBytes);
    ActivityFile probe;   // about 330 bytes of stack: only to see that the primary reads
    if (oldLen == 0 || !decode(oldText, oldLen, probe)) {
        oldLen = Workout::SafeFile::read(mKernel.fs, path, true, oldText, kMaxBytes);
    }

    const size_t len = append(oldLen > 0 ? oldText : nullptr, oldLen, kAppFolder, e, newText, kMaxBytes);
    if (len == 0 || !mKernel.fs.mkdir(kSharedDir) || !Workout::SafeFile::write(mKernel.fs, path, newText, len)) {
        LOG_WARNING("Could not save the activity summary for Streak\n");
    }
}

void Service::sendStreakLine()
{
    // The Streak line (hybridx-link StreakNow): this week as HybridX Streak last saved it, plus
    // what the HybridX apps saved since, this one included, by Streak's own rules. Once, after a
    // save; the buffer is on the heap (the service stack is small). As Race's.
    using namespace HybridXLink;
    auto msg = SDK::make_msg<CustomMessage::StreakLineUpd>(mKernel);
    if (!msg) {
        return;
    }
    std::unique_ptr<char[]> buf(new (std::nothrow) char[kMaxBytes]);
    StreakNow now;
    bool have = false;
    for (int backup = 0; buf && backup < 2 && !have; ++backup) {
        const size_t len = Workout::SafeFile::read(mKernel.fs, kStreakNowPath, backup != 0, buf.get(), kMaxBytes);
        have = len > 0 && decodeStreakNow(buf.get(), len, now);
    }
    if (have) {
        const std::tm t = mTimeTracker.getLocalTime(std::time(nullptr));
        const int32_t today = static_cast<int32_t>(
            calendarSeconds(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, 0, 0, 0) / 86400);
        StreakProjector projector(now, today);
        ActivityFile file;
        for (uint8_t s = 0; s < kSlotCount; ++s) {
            for (int backup = 0; backup < 2; ++backup) {
                const size_t len = Workout::SafeFile::read(mKernel.fs, kSlotPaths[s], backup != 0, buf.get(), kMaxBytes);
                if (len > 0 && decode(buf.get(), len, file)) {
                    projector.add(file);
                    break;
                }
            }
        }
        msg->show = formatStreakLine(projector.result(), msg->line1, sizeof(msg->line1), msg->line2,
                                     sizeof(msg->line2));
        LOG_INFO("Streak line: \"%s\" \"%s\"\n", msg->line1, msg->line2);
    } else {
        LOG_INFO("Streak line: none (no streak_now.json)\n");
    }
    msg.send();
}

// -- To the screen ---------------------------------------------------------------

void Service::sendUpdate()
{
    const uint32_t nowMs = mKernel.sys.getTimeMs();
    Workout::SessionSnapshot s;
    s.state      = static_cast<uint8_t>(sModel.state());
    s.logOk      = mLogOk ? 1u : 0u;
    s.battery    = mBattery;
    s.laps       = sModel.lapCount();
    s.autoLapMin = static_cast<uint8_t>(sModel.config().autoLapMs / 60000u);
    s.hr         = currentHr(nowMs);
    s.zone       = mZones.zoneOf(s.hr);
    s.lapMs      = sModel.lapActiveMs(nowMs);
    s.activeMs   = sModel.activeMs(nowMs);
    s.elapsedMs  = sModel.elapsedMs(nowMs);

    const Workout::SessionSummary sum = sModel.summary();
    s.kcalX10 = sum.kcalX10;
    for (uint8_t z = 0; z < Workout::kZoneCount; ++z) {
        s.zoneMs[z] = sum.zoneMs[z];
    }

    const std::tm local = mTimeTracker.getExpectedLocalTime();
    s.hour   = static_cast<uint8_t>(local.tm_hour < 0 ? 0 : (local.tm_hour > 23 ? 23 : local.tm_hour));
    s.minute = static_cast<uint8_t>(local.tm_min < 0 ? 0 : (local.tm_min > 59 ? 59 : local.tm_min));
    SDK::send_msg<CustomMessage::SessionUpdate>(mKernel, s);
}

void Service::sendSettings()
{
    SDK::send_msg<CustomMessage::SettingsUpdate>(mKernel, mSettings.autoLapMin);
}

void Service::sendSaved(bool saved)
{
    Workout::SavedSession s;
    s.saved    = saved ? 1u : 0u;
    s.summary  = sModel.summary();
    s.activeMs = sModel.activeMs(mKernel.sys.getTimeMs());
    SDK::send_msg<CustomMessage::SessionSaved>(mKernel, s);
}

void Service::sendLaps(uint8_t first)
{
    Workout::LapsPageData page;
    page.total = sModel.lapCount();
    page.first = first < page.total ? first : 0;
    for (uint8_t i = page.first; i < page.total && page.count < Workout::kLapsPerPage; ++i) {
        const Workout::LapStats &l = sModel.lapAt(i);
        Workout::LapRow &row = page.rows[page.count++];
        row.autoEnded  = l.autoEnded ? 1u : 0u;
        row.hrAvg      = l.hrAvg();
        row.hrMax      = l.hrMax;
        row.kcal       = Workout::detail::kcalOf(sModel.lapKcalX10(i));
        row.durationMs = l.activeMs;
    }
    SDK::send_msg<CustomMessage::SessionLapsPage>(mKernel, page);
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

void Service::backlightOn(uint32_t timeoutMs)
{
    auto bl = SDK::make_msg<SDK::Message::RequestBacklightSet>(mKernel);
    if (bl) {
        bl->brightness       = 100;
        bl->autoOffTimeoutMs = timeoutMs;
        bl.send();
    }
}
