/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   The Energy Probe's service (see the header and x-energy/docs/PROBE.md).
 *
 * The loop waits for a message or the end of the row, whichever comes first
 * (Docs/service-lifecycle.md 4.2: nothing paces a resident service). Elapsed
 * time comes from the clock, never from counting wake-ups. Rows are buffered
 * and appended every ten rows; the state is saved with them, because a reboot
 * gives no notice (ibid. 7).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "CivilDate.hpp"
#include "Fixed.hpp"

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAccelerometer.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserAccelerometerRaw.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserActivity.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserActivityRecognition.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryCharging.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryMetrics.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserFloorCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateMetrics.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserMotionDetect.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserStepCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserTemperature.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserTouch.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserWristMotion.hpp"

#define LOG_MODULE_PRX      "EnergyProbe"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

using namespace EnergyProbe;
using SDK::Sensor::Type;

static_assert(sizeof(CustomMessage::ProbeNights) <= 256, "a message must fit the kernel's 256 bytes");
static_assert(sizeof(CustomMessage::ProbeStatus) <= 256, "a message must fit the kernel's 256 bytes");
static_assert(sizeof(CustomMessage::ProbeCheck) <= 256, "a message must fit the kernel's 256 bytes");

namespace
{

// Periods in ms, latencies in ms (the Bike Probe's HEART_RATE_EX 1000/1000 and RunLVGL's
// 100 ms IMU are the watch-proven shapes).
constexpr float    kAccelPeriodMs    = 100.0f;    ///< 10 Hz
constexpr uint32_t kAccelLatencyMs   = 1000;      ///< ten samples a message
constexpr float    kHeartPeriodMs    = 1000.0f;
constexpr uint32_t kHeartLatencyMs   = 1000;
constexpr uint32_t kBattPeriodOnMs   = 60000;
constexpr uint32_t kBattPeriodOffMs  = 600000;
constexpr uint32_t kSparseEvery      = 5;         ///< Heart 1 in 5: on in one minute of five
constexpr uint32_t kBeatRawMinutes   = 10;        ///< raw beats for the first 10 minutes of each hour
constexpr uint32_t kSaneUtc          = 1577836800u;   ///< 2020-01-01: before this the clock is unset
constexpr uint32_t kMaxWaitMs        = 60000;

const char* kLogDir      = "Logs";
const char* kStatePath   = "Logs/state.txt";
const char* kStateNew    = "Logs/state.new";
const char* kNightsPath  = "Logs/nights.csv";
const char* kEventsPath  = "Logs/events.csv";
const char* kEventsHeader = "utc,uptime_s,event,detail\n";
const char* kBeatsHeader  = "utc,ts_s,ts_us,fields,f0,f1,f2,f3,u0\n";

/// A row of text into a fixed buffer, if it fits.
bool put(char* buf, size_t cap, size_t& len, const char* text, size_t n)
{
    if (len + n + 1 > cap) {
        return false;
    }
    std::memcpy(buf + len, text, n);
    len += n;
    buf[len] = '\0';
    return true;
}

float asFloat(uint32_t u)
{
    float f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Construction and the loop
// ---------------------------------------------------------------------------------------------

Service::Service(SDK::Kernel& kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mBattLevel(Type::BATTERY_LEVEL, kBattPeriodOnMs, 0)
    , mBattCharge(Type::BATTERY_CHARGING, kBattPeriodOnMs, 0)
    , mBattMetrics(Type::BATTERY_METRICS, kBattPeriodOnMs, 0)
    , mAccel(Type::ACCELEROMETER, kAccelPeriodMs, kAccelLatencyMs)
    , mAccelRaw(Type::ACCELEROMETER_RAW, kAccelPeriodMs, kAccelLatencyMs)
    , mHeart(Type::HEART_RATE_EX, kHeartPeriodMs, kHeartLatencyMs)
    , mBeat(Type::HEART_BEAT, 0, 1000)
    , mTemp(Type::AMBIENT_TEMPERATURE, 0, 0)
    , mTouch(Type::TOUCH_DETECT, 0, 0)
    , mMotionDet(Type::MOTION_DETECT, 0, 0)
    , mActRec(Type::ACTIVITY_RECOGNITION, 0, 0)
    , mWrist(Type::WRIST_MOTION, 0, 0)
    , mHrDaily(Type::HEART_RATE_METRICS_DAILY, 0, 0)
    , mSteps(Type::STEP_COUNTER_DAILY, 0, 0)
    , mActive(Type::ACTIVITY_TIME_DAILY, 0, 0)
    , mFloors(Type::FLOOR_COUNTER_DAILY, 0, 0)
{
    (void)kernel;
    const struct {
        Type        type;
        const char* name;
        float       period;
        uint32_t    latency;
    } kList[kCheckCount] = {
        {Type::HEART_BEAT, "Beat", 0, 1000},
        {Type::PPG, "PPG", 0, 1000},
        {Type::SPO2, "SpO2", 1000, 1000},
        {Type::ECG, "ECG", 0, 1000},
        {Type::HEART_RATE, "HR", 1000, 1000},
        {Type::HEART_RATE_EX, "HR ex", 1000, 1000},
        {Type::HEART_RATE_METRICS_DAILY, "HR daily", 0, 0},
        {Type::ACCELEROMETER, "Accel", 100, 1000},
        {Type::ACCELEROMETER_RAW, "Accel raw", 100, 1000},
        {Type::AMBIENT_TEMPERATURE, "Temp", 0, 0},
        {Type::TOUCH_DETECT, "Touch", 0, 0},
        {Type::MOTION_DETECT, "Motion", 0, 0},
        {Type::ACTIVITY_RECOGNITION, "Activity", 0, 0},
        {Type::GESTURE_RECOGNITION, "Gesture", 0, 0},
        {Type::WRIST_MOTION, "Wrist", 0, 0},
        {Type::STEP_COUNTER_DAILY, "Steps day", 0, 0},
        {Type::ACTIVITY_TIME_DAILY, "Active day", 0, 0},
        {Type::FLOOR_COUNTER_DAILY, "Floors day", 0, 0},
        {Type::BATTERY_LEVEL, "Batt level", 0, 0},
        {Type::BATTERY_CHARGING, "Batt charge", 0, 0},
        {Type::BATTERY_METRICS, "Batt metrics", 1000, 0},
    };
    for (uint8_t i = 0; i < kCheckCount; ++i) {
        mCheck[i] = CheckEntry{};
        mCheck[i].type = kList[i].type;
        mCheck[i].name = kList[i].name;
        mCheck[i].period = kList[i].period;
        mCheck[i].latency = kList[i].latency;
    }
}

void Service::run()
{
    LOG_INFO("Energy Probe service started\n");
    start();

    while (true) {
        const uint32_t nowMs = mKernel.sys.getTimeMs();
        // Wait until the row ends (by the UTC clock), or the check's next step, at most a minute.
        uint32_t waitMs = kMaxWaitMs;
        const uint32_t utc = static_cast<uint32_t>(std::time(nullptr));
        if (mRowEndUtc > utc) {
            const uint32_t s = mRowEndUtc - utc;
            waitMs = s * 1000u < kMaxWaitMs ? s * 1000u : kMaxWaitMs;
        } else {
            waitMs = 0;
        }
        if (mCheckRunning) {
            const uint32_t elapsed = static_cast<uint32_t>(nowMs - mCheckStartMs);   // unsigned: wraps
            const uint32_t left = elapsed < kCheckMs ? kCheckMs - elapsed : 0;
            const uint32_t step = left < 5000 ? left : 5000;
            if (step < waitMs) {
                waitMs = step;
            }
        }

        SDK::MessageBase* msg = nullptr;
        if (waitMs > 0 && mKernel.comm.getMessage(msg, waitMs)) {
            mEpoch.wake();
            bool leave = false;
            switch (msg->getType()) {
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("COMMAND_APP_STOP: saving and leaving\n");
                    leave = true;
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                    mGuiStarted = true;
                    event("gui", "open");
                    sendStatus();
                    sendNights();
                    sendCheckLines();
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                    // The screen closed. The logger stays: that is the point of it.
                    mGuiStarted = false;
                    event("gui", "closed");
                    break;

                case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                    auto* e = static_cast<SDK::Message::Sensor::EventData*>(msg);
                    SDK::Sensor::DataBatch batch(e->data, static_cast<uint16_t>(e->count),
                                                 static_cast<uint16_t>(e->stride));
                    onSensor(static_cast<uint16_t>(e->handle), batch);
                } break;

                case CustomMessage::SET_OVERRIDE:
                    onOverride(static_cast<CustomMessage::SetOverride*>(msg)->value);
                    break;
                case CustomMessage::SET_WINDOW:
                    onWindow(static_cast<CustomMessage::SetWindow*>(msg)->value);
                    break;
                case CustomMessage::SET_DAY_MODE:
                    onDayMode(static_cast<CustomMessage::SetDayMode*>(msg)->value);
                    break;
                case CustomMessage::START_CHECK:
                    startCheck();
                    break;

                default:
                    break;   // not ours: ignore, and release below
            }
            mKernel.comm.releaseMessage(msg);
            if (leave) {
                stop();
                return;
            }
        }

        // Time-driven work, after a message or a timeout alike.
        if (mCheckRunning) {
            const uint32_t elapsed = static_cast<uint32_t>(mKernel.sys.getTimeMs() - mCheckStartMs);
            if (elapsed >= kCheckMs) {
                finishCheck();
            } else if (static_cast<uint32_t>(mKernel.sys.getTimeMs() - mCheckLastProgressMs) >= 10000) {
                mCheckLastProgressMs = mKernel.sys.getTimeMs();
                if (mGuiStarted) {
                    auto m = SDK::make_msg<CustomMessage::ProbeCheck>(mKernel);
                    if (m) {
                        m->secsLeft = static_cast<uint8_t>((kCheckMs - elapsed + 999) / 1000);
                        m.send();
                    }
                }
            }
        }
        const Clock c = clockNow();
        if (c.utc >= mRowEndUtc || c.utc + 120 < mRowStartUtc) {   // the row is over, or the clock went back
            closeRow(c);
        }
    }
}

Service::Clock Service::clockNow() const
{
    Clock c;
    c.utc = static_cast<uint32_t>(std::time(nullptr));
    c.valid = c.utc >= kSaneUtc;
    const time_t t = static_cast<time_t>(c.utc);
    struct tm tm {};
    if (localtime_r(&t, &tm) != nullptr) {
        c.local.day = daysFromCivil(tm.tm_year + 1900, static_cast<uint32_t>(tm.tm_mon + 1),
                                    static_cast<uint32_t>(tm.tm_mday));
        c.local.hour = static_cast<uint8_t>(tm.tm_hour);
        c.local.minute = static_cast<uint8_t>(tm.tm_min);
        const CivilDate d = civilFromDays(c.local.day);
        const unsigned  y = d.year < 0 ? 0u : (d.year > 9999 ? 9999u : static_cast<unsigned>(d.year));
        std::snprintf(c.text, sizeof(c.text), "%04u-%02u-%02uT%02u:%02u", y, static_cast<unsigned>(d.month),
                      static_cast<unsigned>(d.day), static_cast<unsigned>(c.local.hour % 24u),
                      static_cast<unsigned>(c.local.minute % 60u));
    } else {
        c.local.day = static_cast<int32_t>(c.utc / 86400u);
        c.local.hour = static_cast<uint8_t>((c.utc / 3600u) % 24u);
        c.local.minute = static_cast<uint8_t>((c.utc / 60u) % 60u);
        std::snprintf(c.text, sizeof(c.text), "utc");
    }
    return c;
}

// ---------------------------------------------------------------------------------------------
// Start and stop
// ---------------------------------------------------------------------------------------------

void Service::start()
{
    mKernel.fs.mkdir(kLogDir);
    loadState();
    ++mState.starts;

    const Clock c = clockNow();
    char detail[48];
    std::snprintf(detail, sizeof(detail), "start %lu, uptime %lu s, clock %s",
                  static_cast<unsigned long>(mState.starts),
                  static_cast<unsigned long>(mKernel.sys.getTimeMs() / 1000u), c.valid ? "set" : "unset");
    event("start", detail);

    // The battery, in every mode.
    mBattPeriodMs = kBattPeriodOnMs;
    if (!mBattLevel.connect()) {
        event("connect_failed", "BATTERY_LEVEL");
    }
    if (!mBattCharge.connect()) {
        event("connect_failed", "BATTERY_CHARGING");
    }
    if (!mBattMetrics.connect()) {
        event("connect_failed", "BATTERY_METRICS");
    }

    const Period p = periodAt(c.local, window(mState.settings.window));
    enterPeriod(p, c, true);
    beginRow(c);
    saveState();
    flush();
}

void Service::stop()
{
    event("stop", "COMMAND_APP_STOP");
    // The part-row so far, so nothing is lost; the night stays open for the restart.
    char row[1024];
    RowContext ctx;
    ctx.utc = mRowStartUtc;
    ctx.local = mRowLocal;
    ctx.uptimeS = mKernel.sys.getTimeMs() / 1000u;
    ctx.mode = modeCode(mMode);
    ctx.night = mPeriod.night;
    const size_t n = mEpoch.format(row, sizeof(row), ctx);
    if (n > 0 && put(mRows, kRowBuf, mRowsLen, row, n)) {
        ++mRowsBuffered;
    }
    saveState();
    flush();
    disconnectGroup(0xFFFFFFFFu);
    mBattLevel.disconnect();
    mBattCharge.disconnect();
    mBattMetrics.disconnect();
    if (mCheckRunning) {
        for (uint8_t i = 0; i < kCheckCount; ++i) {
            CheckEntry& e = mCheck[i];
            if (e.connected && !e.shared) {
                auto d = SDK::make_msg<SDK::Message::Sensor::RequestDisconnect>(mKernel);
                if (d) {
                    d->handle = e.handle;
                    d.send();
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// The sensors per mode
// ---------------------------------------------------------------------------------------------

void Service::applyMode(Mode mode)
{
    const Mode before = mMode;
    mMode = mode;
    uint32_t want = groups(mode) & ~static_cast<uint32_t>(kGroupBattery);
    // Heart 1 in 5 is connected minute by minute (sparseHeart), not here.
    const uint32_t have = mGroupsOn;
    disconnectGroup(have & ~want);
    uint32_t failed = 0;
    for (uint32_t bit = 1; bit != 0 && bit <= kGroupDaily; bit <<= 1) {
        if ((want & bit) && !(have & bit) && bit != kGroupHeartSparse) {
            if (!connectGroup(bit)) {
                failed |= bit;
            }
        }
    }
    if (!(want & kGroupHeartSparse) && mHeartOn && !(want & kGroupHeart)) {
        mHeart.disconnect();
        mHeartOn = false;
    }
    mGroupsFailed = (mGroupsFailed & want) | failed;

    // The battery's period follows the mode: the Off baseline asks least.
    const uint32_t battPeriod = mode == Mode::Off ? kBattPeriodOffMs : kBattPeriodOnMs;
    if (battPeriod != mBattPeriodMs) {
        mBattPeriodMs = battPeriod;
        mBattLevel.disconnect();
        mBattCharge.disconnect();
        mBattMetrics.disconnect();
        mBattLevel.connect(static_cast<float>(battPeriod), 0);
        mBattCharge.connect(static_cast<float>(battPeriod), 0);
        mBattMetrics.connect(static_cast<float>(battPeriod), 0);
    }

    if (before != mode || mState.starts == 0) {
        char detail[40];
        std::snprintf(detail, sizeof(detail), "%s -> %s, failed 0x%lx", modeCode(before), modeCode(mode),
                      static_cast<unsigned long>(failed));
        event("mode", detail);
    }
}

bool Service::connectGroup(uint32_t group)
{
    bool ok = true;
    auto tryConnect = [&](SDK::Sensor::Connection& c, const char* name) {
        if (c.isConnected()) {
            return true;
        }
        if (c.connect()) {
            return true;
        }
        event("connect_failed", name);
        return false;
    };
    switch (group) {
        case kGroupMotion:
            if (tryConnect(mAccel, "ACCELEROMETER")) {
                mAccelIsRaw = false;
                mEpoch.setAccelUnitsMs2(true);
            } else if (tryConnect(mAccelRaw, "ACCELEROMETER_RAW")) {
                mAccelIsRaw = true;
                mEpoch.setAccelUnitsMs2(false);
                event("accel", "using ACCELEROMETER_RAW");
            } else {
                ok = false;
            }
            break;
        case kGroupHeart:
            if (!mHeartOn) {
                ok = tryConnect(mHeart, "HEART_RATE_EX");
                mHeartOn = ok;
                mHeartOnMs = mKernel.sys.getTimeMs();
                mHeartTrustedYet = false;
            }
            break;
        case kGroupBeats:
            ok = tryConnect(mBeat, "HEART_BEAT");
            break;
        case kGroupTemperature:
            ok = tryConnect(mTemp, "AMBIENT_TEMPERATURE");
            break;
        case kGroupEvents: {
            const bool a = tryConnect(mTouch, "TOUCH_DETECT");
            const bool b = tryConnect(mMotionDet, "MOTION_DETECT");
            const bool d = tryConnect(mActRec, "ACTIVITY_RECOGNITION");
            const bool e = tryConnect(mWrist, "WRIST_MOTION");
            ok = a && b && d && e;
        } break;
        case kGroupDaily: {
            const bool a = tryConnect(mHrDaily, "HEART_RATE_METRICS_DAILY");
            const bool b = tryConnect(mSteps, "STEP_COUNTER_DAILY");
            const bool d = tryConnect(mActive, "ACTIVITY_TIME_DAILY");
            const bool e = tryConnect(mFloors, "FLOOR_COUNTER_DAILY");
            ok = a && b && d && e;
        } break;
        default:
            return true;
    }
    // A group counts as on even if one of its sensors failed: the others still log.
    mGroupsOn |= group;
    return ok;
}

void Service::disconnectGroup(uint32_t group)
{
    if (group & kGroupMotion) {
        mAccel.disconnect();
        mAccelRaw.disconnect();
    }
    if (group & (kGroupHeart | kGroupHeartSparse)) {
        mHeart.disconnect();
        mHeartOn = false;
    }
    if (group & kGroupBeats) {
        mBeat.disconnect();
    }
    if (group & kGroupTemperature) {
        mTemp.disconnect();
    }
    if (group & kGroupEvents) {
        mTouch.disconnect();
        mMotionDet.disconnect();
        mActRec.disconnect();
        mWrist.disconnect();
    }
    if (group & kGroupDaily) {
        mHrDaily.disconnect();
        mSteps.disconnect();
        mActive.disconnect();
        mFloors.disconnect();
    }
    mGroupsOn &= ~group;
}

void Service::onSensor(uint16_t handle, SDK::Sensor::DataBatch& batch)
{
    if (mCheckRunning) {
        checkData(handle, batch);
    }
    const uint16_t n = batch.size();
    if (n == 0) {
        return;
    }
    const uint32_t nowMs = mKernel.sys.getTimeMs();
    const uint32_t offsetMs = static_cast<uint32_t>(nowMs - mRowStartMs);   // unsigned: wraps

    if (mBattLevel.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryLevel p(batch[n - 1]);
        if (p.isDataValid()) {
            mEpoch.batteryLevel(p.getCharge());
        }
    } else if (mBattCharge.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryCharging p(batch[n - 1]);
        if (p.isDataValid()) {
            const int8_t before = mEpoch.charging();
            mEpoch.batteryCharging(p.isUsbConnected(), p.isCharging());
            if (before != mEpoch.charging()) {
                event("charging", p.isCharging() ? "on" : "off");
            }
        }
    } else if (mBattMetrics.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryMetrics p(batch[n - 1]);
        if (p.isDataValid()) {
            mEpoch.batteryMetrics(p.getVoltage(), p.getCurrent(), p.getAverageCurrent(), p.getCapacity());
        }
    } else if (mAccel.matchesDriver(handle)) {
        for (uint16_t i = 0; i < n; ++i) {
            SDK::SensorDataParser::Accelerometer p(batch[i]);
            if (p.isDataValid()) {
                mEpoch.accel(offsetMs, p.getX(), p.getY(), p.getZ());
            }
        }
    } else if (mAccelRaw.matchesDriver(handle)) {
        for (uint16_t i = 0; i < n; ++i) {
            SDK::SensorDataParser::AccelerometerRaw p(batch[i]);
            if (p.isDataValid()) {
                mEpoch.accel(offsetMs, static_cast<float>(p.getX()), static_cast<float>(p.getY()),
                             static_cast<float>(p.getZ()));
            }
        }
    } else if (mHeart.matchesDriver(handle)) {
        for (uint16_t i = 0; i < n; ++i) {
            SDK::SensorDataParser::HeartRateEx p(batch[i]);
            if (!p.isDataValid()) {
                continue;
            }
            mEpoch.heart(p.getBpm(), p.getTrustLevel(), static_cast<uint8_t>(p.getSource()), p.getOpticalBpm());
            if (!mHeartTrustedYet && trustedHeart(p.getBpm(), p.getTrustLevel())) {
                mHeartTrustedYet = true;
                mEpoch.heartFirstTrusted(static_cast<uint32_t>(nowMs - mHeartOnMs));
            }
        }
    } else if (mBeat.matchesDriver(handle)) {
        const Clock c = clockNow();
        const bool raw = c.local.minute < kBeatRawMinutes;
        for (uint16_t i = 0; i < n; ++i) {
            const SDK::Sensor::DataView v = batch[i];
            const uint64_t us = v.getTimestampUs();
            mEpoch.beat(us);
            if (!raw) {
                continue;
            }
            // The raw beat, as it came: its timestamp and the first four fields, as floats and the
            // first as hex too. No parser exists, so the layout is what this tells us.
            char line[160];
            size_t len = 0;
            int w = std::snprintf(line, sizeof(line), "%lu,%lu,%06lu,%u", static_cast<unsigned long>(c.utc),
                                  static_cast<unsigned long>(us / 1000000u),
                                  static_cast<unsigned long>(us % 1000000u),
                                  static_cast<unsigned>(v.getFieldCount()));
            len = w > 0 ? static_cast<size_t>(w) : 0;
            for (uint16_t f = 0; f < 4; ++f) {
                char num[24] = "";
                if (f < v.getFieldCount()) {
                    formatFixed(num, sizeof(num), v.f[f], 3);
                }
                w = std::snprintf(line + len, sizeof(line) - len, ",%s", num);
                len += w > 0 ? static_cast<size_t>(w) : 0;
            }
            w = std::snprintf(line + len, sizeof(line) - len, ",%08lx\n",
                              static_cast<unsigned long>(v.getFieldCount() > 0 ? v.u[0] : 0));
            len += w > 0 ? static_cast<size_t>(w) : 0;
            if (len >= sizeof(line) || !put(mBeats, kBeatBuf, mBeatsLen, line, len)) {
                ++mDroppedBeats;
            }
        }
    } else if (mTemp.matchesDriver(handle)) {
        for (uint16_t i = 0; i < n; ++i) {
            SDK::SensorDataParser::Temperature p(batch[i]);
            if (p.isDataValid()) {
                mEpoch.temperature(p.getTemperature());
            }
        }
    } else if (mTouch.matchesDriver(handle)) {
        for (uint16_t i = 0; i < n; ++i) {
            SDK::SensorDataParser::Touch p(batch[i]);
            if (p.isDataValid()) {
                mEpoch.touch(p.isTouched());
            }
        }
    } else if (mMotionDet.matchesDriver(handle)) {
        for (uint16_t i = 0; i < n; ++i) {
            SDK::SensorDataParser::MotionDetect p(batch[i]);
            if (p.isDataValid()) {
                mEpoch.motionDetect(static_cast<uint32_t>(p.getID()));
            }
        }
    } else if (mActRec.matchesDriver(handle)) {
        for (uint16_t i = 0; i < n; ++i) {
            SDK::SensorDataParser::ActivityRecognition p(batch[i]);
            if (p.isDataValid()) {
                mEpoch.activity(static_cast<uint32_t>(p.getID()), p.getConfidence());
            }
        }
    } else if (mWrist.matchesDriver(handle)) {
        for (uint16_t i = 0; i < n; ++i) {
            SDK::SensorDataParser::WristMotion p(batch[i]);
            if (p.isDataValid() && p.isWristMotion()) {
                mEpoch.wrist();
            }
        }
    } else if (mHrDaily.matchesDriver(handle)) {
        SDK::SensorDataParser::HeartRateMetrics p(batch[n - 1]);
        if (p.isDataValid()) {
            mEpoch.dailyHeart(p.getAhr(), p.getRhr());
        }
    } else if (mSteps.matchesDriver(handle)) {
        SDK::SensorDataParser::StepCounter p(batch[n - 1]);
        if (p.isDataValid()) {
            mEpoch.dailySteps(p.getStepCount());
        }
    } else if (mActive.matchesDriver(handle)) {
        SDK::SensorDataParser::Activity p(batch[n - 1]);
        if (p.isDataValid()) {
            mEpoch.dailyActive(p.getDuration());
        }
    } else if (mFloors.matchesDriver(handle)) {
        SDK::SensorDataParser::FloorCounter p(batch[n - 1]);
        if (p.isDataValid()) {
            mEpoch.dailyFloors(p.getFloorsUp());
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Rows
// ---------------------------------------------------------------------------------------------

uint32_t Service::rowEndUtc(uint32_t startUtc) const
{
    // Rows end on the wall-clock minute (or ten minutes when Off), so they line up across nights.
    const uint32_t step = rowMinutes(mMode) * 60u;
    return (startUtc / step + 1u) * step;
}

void Service::beginRow(const Clock& c)
{
    mEpoch.reset();
    mRowStartUtc = c.utc;
    mRowEndUtc = rowEndUtc(c.utc);
    mRowStartMs = mKernel.sys.getTimeMs();
    std::memcpy(mRowLocal, c.text, sizeof(mRowLocal));
    mRowLocal[sizeof(mRowLocal) - 1] = '\0';
    mRowDay = c.local.day;
    sparseHeart(c);
}

void Service::sparseHeart(const Clock& c)
{
    if (mMode != Mode::HeartSparse) {
        return;
    }
    const bool on = (c.utc / 60u) % kSparseEvery == 0;
    if (on && !mHeartOn) {
        mHeartOn = mHeart.connect();
        mHeartOnMs = mKernel.sys.getTimeMs();
        mHeartTrustedYet = false;
        if (!mHeartOn) {
            mGroupsFailed |= kGroupHeartSparse;
            event("connect_failed", "HEART_RATE_EX (1 in 5)");
        } else {
            mGroupsOn |= kGroupHeartSparse;
        }
    } else if (!on && mHeartOn) {
        mHeart.disconnect();
        mHeartOn = false;
        mGroupsOn &= ~static_cast<uint32_t>(kGroupHeartSparse);
    }
}

void Service::closeRow(const Clock& c)
{
    // The row as it stands.
    RowContext ctx;
    ctx.utc = mRowStartUtc;
    ctx.local = mRowLocal;
    ctx.uptimeS = mKernel.sys.getTimeMs() / 1000u;
    ctx.mode = modeCode(mMode);
    ctx.night = mPeriod.night;
    char row[1024];
    const size_t n = mEpoch.format(row, sizeof(row), ctx);

    // A new local date starts a new file: write what belongs to the old one first.
    if (mRowsLen > 0 && mRowDay != mBufferDay) {
        flush();
    }
    if (mRowsLen == 0) {
        mBufferDay = mRowDay;
    }
    if (n > 0) {
        if (!put(mRows, kRowBuf, mRowsLen, row, n)) {
            flush();
            mBufferDay = mRowDay;
            if (put(mRows, kRowBuf, mRowsLen, row, n)) {
                ++mRowsBuffered;
            }
        } else {
            ++mRowsBuffered;
        }
    }

    // The night's sums.
    const uint32_t minutes = (c.utc > mRowStartUtc ? c.utc - mRowStartUtc + 30u : 0u) / 60u;
    if (mNight.isOpen() && mPeriod.night) {
        mNight.addRow(c.utc, minutes > 0 ? minutes : 1u, mEpoch.worn(), mEpoch.heartTrusted() > 0,
                      mEpoch.charging(), mEpoch.batteryPctX100(), mEpoch.batteryMahX10(), mEpoch.hasAvgCurrent(),
                      mEpoch.batteryAvgMaX100());
    }
    // Has the period changed? Then the mode may have too.
    const Period p = periodAt(c.local, window(mState.settings.window));
    if (p != mPeriod) {
        enterPeriod(p, c, false);
    }

    if (mRowsBuffered >= kFlushRows || mRowsLen + 1024 > kRowBuf) {
        saveState();
        flush();
    }
    beginRow(c);
    if (mGuiStarted) {
        sendStatus();
    }
}

// ---------------------------------------------------------------------------------------------
// The night
// ---------------------------------------------------------------------------------------------

void Service::enterPeriod(const Period& p, const Clock& c, bool atStart)
{
    const Window w = window(mState.settings.window);

    // A night left open: finished now, or by a restart that came after it ended.
    if (mNight.isOpen() && (!p.night || mNight.state().day != p.day)) {
        if (atStart) {
            closeNight(mNight.closeAtLastRow());
        } else {
            closeNight(mNight.close(c.utc, mEpoch.batteryPctX100(), mEpoch.batteryMahX10()));
        }
    }
    if (p.night) {
        if (mState.settings.firstNight < 0) {
            mState.settings.firstNight = p.day;
        }
        if (mNight.isOpen() && mNight.state().day == p.day) {
            mNight.markRestarted();   // the same night, after a restart
        } else {
            const Mode m = modeFor(p, mState.settings);
            mNight.open(p.day, static_cast<uint8_t>(m), c.utc, mEpoch.batteryPctX100(), mEpoch.batteryMahX10(),
                        minutesIntoPeriod(c.local, w));
        }
    }
    mPeriod = p;
    mHavePeriod = true;
    applyMode(modeFor(p, mState.settings));
    char detail[40];
    const uint32_t ymd = yyyymmdd(p.day);
    std::snprintf(detail, sizeof(detail), "%s %lu %s", p.night ? "night" : "day", static_cast<unsigned long>(ymd),
                  modeCode(mMode));
    event("period", detail);
    saveState();
}

void Service::closeNight(const NightResult& r)
{
    char row[256];
    const size_t n = formatNight(row, sizeof(row), r, modeCode(modeFrom(r.mode)), yyyymmdd(r.day));
    if (n > 0 && !append(kNightsPath, nightHeader(), row, n)) {
        ++mWriteErrors;
    }
    pushHistory(mState, briefOf(r));
    char detail[40];
    std::snprintf(detail, sizeof(detail), "%lu %s drain %ld", static_cast<unsigned long>(yyyymmdd(r.day)),
                  modeCode(modeFrom(r.mode)), static_cast<long>(r.drainX100 == INT32_MIN ? -1 : r.drainX100));
    event("night", detail);
    saveState();
    if (mGuiStarted) {
        sendNights();
    }
}

// ---------------------------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------------------------

bool Service::append(const char* path, const char* header, const char* data, size_t len)
{
    // Open for writing without truncating, then write at the end (as X Weigh-in's LogStore).
    auto file = mKernel.fs.file(path);
    if (!file || !file->open(true, false)) {
        return false;
    }
    const size_t size = file->size();
    bool ok = file->seek(size);
    size_t w = 0;
    if (ok && size == 0 && header != nullptr) {
        const size_t hl = std::strlen(header);
        ok = file->write(header, hl, w) && w == hl;
        mBytesWritten += static_cast<uint32_t>(w);
    }
    if (ok && len > 0) {
        ok = file->write(data, len, w) && w == len;
        mBytesWritten += static_cast<uint32_t>(w);
    }
    ok = file->flush() && ok;
    file->close();
    return ok;
}

void Service::flush()
{
    if (mRowsLen > 0) {
        char path[40];
        std::snprintf(path, sizeof(path), "Logs/min_%08lu.csv", static_cast<unsigned long>(yyyymmdd(mBufferDay)));
        if (append(path, MinuteEpoch::header(), mRows, mRowsLen)) {
            mRowsWritten += mRowsBuffered;
        } else {
            ++mWriteErrors;
            LOG_WARNING("could not write %s\n", path);
        }
        mRowsLen = 0;
        mRowsBuffered = 0;
        mRows[0] = '\0';
    }
    if (mBeatsLen > 0) {
        char path[40];
        std::snprintf(path, sizeof(path), "Logs/beats_%08lu.csv", static_cast<unsigned long>(yyyymmdd(mBufferDay)));
        if (!append(path, kBeatsHeader, mBeats, mBeatsLen)) {
            ++mWriteErrors;
        }
        mBeatsLen = 0;
        mBeats[0] = '\0';
    }
    if (mDroppedBeats > 0) {
        char detail[32];
        std::snprintf(detail, sizeof(detail), "%lu raw beats dropped", static_cast<unsigned long>(mDroppedBeats));
        mDroppedBeats = 0;
        event("beats", detail);
    }
    if (mEventsLen > 0) {
        if (!append(kEventsPath, kEventsHeader, mEvents, mEventsLen)) {
            ++mWriteErrors;
        }
        mEventsLen = 0;
        mEvents[0] = '\0';
    }
}

void Service::event(const char* what, const char* detail)
{
    char line[128];
    const int n = std::snprintf(line, sizeof(line), "%lu,%lu,%s,%s\n", static_cast<unsigned long>(std::time(nullptr)),
                                static_cast<unsigned long>(mKernel.sys.getTimeMs() / 1000u), what,
                                detail != nullptr ? detail : "");
    if (n <= 0) {
        return;
    }
    const size_t len = static_cast<size_t>(n) < sizeof(line) ? static_cast<size_t>(n) : sizeof(line) - 1;
    if (!put(mEvents, kEventBuf, mEventsLen, line, len)) {
        // Full: write what there is, then this.
        if (append(kEventsPath, kEventsHeader, mEvents, mEventsLen)) {
            mEventsLen = 0;
            mEvents[0] = '\0';
            put(mEvents, kEventBuf, mEventsLen, line, len);
        } else {
            ++mWriteErrors;
        }
    }
    LOG_INFO("event %s %s\n", what, detail != nullptr ? detail : "");
}

void Service::loadState()
{
    char* text = mStateText;
    const size_t cap = sizeof(mStateText);
    const char* paths[2] = {kStatePath, kStateNew};
    for (const char* path : paths) {
        auto file = mKernel.fs.file(path);
        if (!file || !file->open(false, false)) {
            continue;
        }
        size_t got = 0;
        const bool ok = file->read(text, cap - 1, got);
        file->close();
        if (!ok) {
            continue;
        }
        text[got] = '\0';
        ProbeState s;
        if (readState(text, got, s)) {
            mState = s;
            if (s.night.open) {
                mNight.restore(s.night);
            }
            return;
        }
    }
    mState = ProbeState{};   // a new probe
}

void Service::saveState()
{
    mState.night = mNight.state();
    char* text = mStateText;
    const size_t n = writeState(text, sizeof(mStateText), mState);
    if (n == 0) {
        ++mWriteErrors;
        return;
    }
    // Write the new copy, then swap it in: a reboot mid-write leaves one good copy.
    auto file = mKernel.fs.file(kStateNew);
    if (!file || !file->open(true, true)) {
        ++mWriteErrors;
        return;
    }
    size_t w = 0;
    bool ok = file->write(text, n, w) && w == n;
    ok = file->flush() && ok;
    file->close();
    if (!ok) {
        ++mWriteErrors;
        return;
    }
    mKernel.fs.remove(kStatePath);
    if (!mKernel.fs.rename(kStateNew, kStatePath)) {
        ++mWriteErrors;
    }
}

// ---------------------------------------------------------------------------------------------
// The GUI
// ---------------------------------------------------------------------------------------------

void Service::sendStatus()
{
    auto m = SDK::make_msg<CustomMessage::ProbeStatus>(mKernel);
    if (!m) {
        return;
    }
    const Clock c = clockNow();
    const Window w = window(mState.settings.window);
    const int32_t night = tonight(c.local, w);
    Period tp;
    tp.night = true;
    tp.day = night;
    m->d.tonightDay = night;
    m->d.battPctX100 = mEpoch.batteryPctX100();
    m->d.rows = mRowsWritten + mRowsBuffered;
    m->d.kbWritten = mBytesWritten / 1024u;
    m->d.writeErrors = mWriteErrors;
    m->d.uptimeS = mKernel.sys.getTimeMs() / 1000u;
    m->d.starts = mState.starts;
    m->d.groupsOn = mGroupsOn | kGroupBattery;
    m->d.groupsFailed = mGroupsFailed;
    m->d.modeNow = static_cast<uint8_t>(mMode);
    m->d.night = mPeriod.night ? 1 : 0;
    m->d.tonightMode = static_cast<uint8_t>(modeFor(tp, mState.settings));
    m->d.overrideMode = mState.settings.overrideDay == night ? mState.settings.overrideMode : kOverrideAuto;
    m->d.window = mState.settings.window;
    m->d.dayMode = static_cast<uint8_t>(mState.settings.dayMode);
    m->d.lastBpm = mEpoch.lastBpm();
    m->d.charging = mEpoch.charging();
    m->d.touch = mEpoch.touchState();
    m->d.checkRunning = mCheckRunning ? 1 : 0;
    m->d.timeValid = c.valid ? 1 : 0;
    m->d.valid = 1;
    m.send();
}

void Service::sendNights()
{
    auto m = SDK::make_msg<CustomMessage::ProbeNights>(mKernel);
    if (!m) {
        return;
    }
    m->d.count = mState.historyCount;
    for (uint8_t i = 0; i < mState.historyCount && i < kHistory; ++i) {
        m->d.nights[i] = mState.history[i];
    }
    m.send();
}

void Service::onOverride(uint8_t value)
{
    if (value >= kOverrideCount) {
        return;
    }
    const Clock c = clockNow();
    const int32_t night = tonight(c.local, window(mState.settings.window));
    mState.settings.overrideDay = night;
    mState.settings.overrideMode = value;
    char detail[32];
    std::snprintf(detail, sizeof(detail), "tonight %lu -> %u", static_cast<unsigned long>(yyyymmdd(night)),
                  static_cast<unsigned>(value));
    event("override", detail);
    // If that night is on now, its mode changes now, and the night is flagged as mixed.
    if (mPeriod.night && mPeriod.day == night) {
        const Mode m = modeFor(mPeriod, mState.settings);
        applyMode(m);
        mNight.changeMode(static_cast<uint8_t>(m));
    }
    saveState();
    sendStatus();
}

void Service::onWindow(uint8_t value)
{
    if (value >= kWindowCount) {
        return;
    }
    mState.settings.window = value;
    char detail[24];
    std::snprintf(detail, sizeof(detail), "%02u:00-%02u:00", static_cast<unsigned>(window(value).startHour),
                  static_cast<unsigned>(window(value).endHour));
    event("window", detail);
    const Clock c = clockNow();
    const Period p = periodAt(c.local, window(value));
    if (p != mPeriod) {
        enterPeriod(p, c, false);
    }
    saveState();
    sendStatus();
}

void Service::onDayMode(uint8_t value)
{
    mState.settings.dayMode = value == static_cast<uint8_t>(Mode::Off) ? Mode::Off : Mode::Full;
    event("day_mode", modeCode(mState.settings.dayMode));
    if (!mPeriod.night) {
        applyMode(modeFor(mPeriod, mState.settings));
    }
    saveState();
    sendStatus();
}

// ---------------------------------------------------------------------------------------------
// The sensor check: every type the SDK lists, connected for two minutes, everything recorded
// ---------------------------------------------------------------------------------------------

bool Service::sharedHandle(uint32_t handle)
{
    const uint16_t h = static_cast<uint16_t>(handle);
    SDK::Sensor::Connection* mine[] = {&mBattLevel, &mBattCharge, &mBattMetrics, &mAccel, &mAccelRaw, &mHeart,
                                       &mBeat, &mTemp, &mTouch, &mMotionDet, &mActRec, &mWrist,
                                       &mHrDaily, &mSteps, &mActive, &mFloors};
    for (auto* c : mine) {
        if (c->isConnected() && c->matchesDriver(h)) {
            return true;
        }
    }
    return false;
}

void Service::startCheck()
{
    if (mCheckRunning) {
        return;
    }
    event("check", "start");
    mCheckRunning = true;
    mCheckDone = false;
    mCheckStartMs = mKernel.sys.getTimeMs();
    mCheckLastProgressMs = mCheckStartMs;
    for (uint8_t i = 0; i < kCheckCount; ++i) {
        CheckEntry& e = mCheck[i];
        e.handle = 0;
        e.handles = 0;
        e.desc[0] = '\0';
        e.connected = false;
        e.shared = false;
        e.msgs = e.samples = 0;
        e.fields = e.stride = 0;
        e.firstTsMs = e.firstNowMs = e.lastTsMs = 0;
        e.rawN = 0;

        auto def = SDK::make_msg<SDK::Message::Sensor::RequestDefault>(mKernel);
        if (!def) {
            continue;
        }
        def->id = e.type;
        if (!def.send(100) || !def.ok()) {
            continue;
        }
        e.handle = def->handle;
        if (e.handle == 0) {
            continue;
        }
        auto list = SDK::make_msg<SDK::Message::Sensor::RequestList>(mKernel);
        if (list) {
            list->id = e.type;
            if (list.send(100) && list.ok()) {
                e.handles = list->handlesCount;
            }
        }
        auto desc = SDK::make_msg<SDK::Message::Sensor::RequestGetDesc>(mKernel);
        if (desc) {
            desc->handle = e.handle;
            if (desc.send(100) && desc.ok()) {
                std::memcpy(e.desc, desc->desc, sizeof(desc->desc));
                e.desc[sizeof(e.desc) - 1] = '\0';
                // Commas would break the CSV.
                for (char& ch : e.desc) {
                    if (ch == ',' || ch == '\n' || ch == '\r') {
                        ch = ' ';
                    }
                }
            }
        }
        if (sharedHandle(e.handle)) {
            e.shared = true;      // the logger has it already: listen, but leave it alone
            e.connected = true;
            continue;
        }
        auto con = SDK::make_msg<SDK::Message::Sensor::RequestConnect>(mKernel);
        if (con) {
            con->handle = e.handle;
            con->period = e.period;
            con->latency = e.latency;
            e.connected = con.send(100) && con.ok();
        }
    }
    sendStatus();
    if (mGuiStarted) {
        auto m = SDK::make_msg<CustomMessage::ProbeCheck>(mKernel);
        if (m) {
            m->secsLeft = static_cast<uint8_t>(kCheckMs / 1000u);
            m.send();
        }
    }
}

void Service::checkData(uint16_t handle, SDK::Sensor::DataBatch& batch)
{
    const uint32_t nowMs = mKernel.sys.getTimeMs();
    for (uint8_t i = 0; i < kCheckCount; ++i) {
        CheckEntry& e = mCheck[i];
        if (e.handle == 0 || e.handle != handle) {
            continue;
        }
        ++e.msgs;
        for (uint16_t s = 0; s < batch.size(); ++s) {
            const SDK::Sensor::DataView v = batch[s];
            if (e.samples == 0) {
                e.firstTsMs = v.getTimestamp();
                e.firstNowMs = nowMs;
                e.fields = v.getFieldCount();
            }
            e.lastTsMs = v.getTimestamp();
            ++e.samples;
            if (e.rawN < 3) {
                for (uint16_t f = 0; f < 4; ++f) {
                    e.raw[e.rawN][f] = f < v.getFieldCount() ? v.u[f] : 0;
                }
                ++e.rawN;
            }
        }
        // The stride is the same for every message of a sensor; from the first.
        if (e.stride == 0) {
            e.stride = static_cast<uint16_t>(sizeof(SDK::Sensor::Data) +
                                             (e.fields > 0 ? (e.fields - 1u) * sizeof(SDK::Sensor::Data::Field) : 0));
        }
    }
}

void Service::finishCheck()
{
    mCheckRunning = false;
    mCheckDone = true;
    // Disconnect what the check connected (not what the logger holds).
    for (uint8_t i = 0; i < kCheckCount; ++i) {
        CheckEntry& e = mCheck[i];
        if (e.connected && !e.shared && e.handle != 0) {
            auto d = SDK::make_msg<SDK::Message::Sensor::RequestDisconnect>(mKernel);
            if (d) {
                d->handle = e.handle;
                d.send();
            }
        }
    }

    // The file: one row per type.
    char path[40];
    std::snprintf(path, sizeof(path), "Logs/check_%lu.csv", static_cast<unsigned long>(std::time(nullptr)));
    const char* header =
        "type_hex,name,handle,handles,desc,connected,shared,msgs,samples,fields,stride,first_ts_ms,first_now_ms,"
        "last_ts_ms,s0_f0,s0_f1,s0_f2,s0_f3,s0_u0,s1_f0,s1_f1,s1_f2,s1_f3,s1_u0,s2_f0,s2_f1,s2_f2,s2_f3,s2_u0\n";
    flush();   // empties mRows, which the check's lines borrow
    size_t len = 0;
    bool   ok = true;
    bool   first = true;
    mRows[0] = '\0';
    for (uint8_t i = 0; i < kCheckCount; ++i) {
        const CheckEntry& e = mCheck[i];
        char line[400];
        int w = std::snprintf(line, sizeof(line), "%lx,%s,%lu,%lu,%s,%u,%u,%lu,%lu,%u,%u,%lu,%lu,%lu",
                              static_cast<unsigned long>(e.type), e.name, static_cast<unsigned long>(e.handle),
                              static_cast<unsigned long>(e.handles), e.desc, e.connected ? 1u : 0u,
                              e.shared ? 1u : 0u, static_cast<unsigned long>(e.msgs),
                              static_cast<unsigned long>(e.samples), static_cast<unsigned>(e.fields),
                              static_cast<unsigned>(e.stride), static_cast<unsigned long>(e.firstTsMs),
                              static_cast<unsigned long>(e.firstNowMs), static_cast<unsigned long>(e.lastTsMs));
        size_t ll = w > 0 ? static_cast<size_t>(w) : 0;
        for (uint8_t s = 0; s < 3; ++s) {
            for (uint8_t f = 0; f < 4; ++f) {
                char num[24] = "";
                if (s < e.rawN && f < e.fields) {
                    formatFixed(num, sizeof(num), asFloat(e.raw[s][f]), 3);
                }
                w = std::snprintf(line + ll, sizeof(line) - ll, ",%s", num);
                ll += w > 0 ? static_cast<size_t>(w) : 0;
            }
            char hex[12] = "";
            if (s < e.rawN && e.fields > 0) {
                std::snprintf(hex, sizeof(hex), "%08lx", static_cast<unsigned long>(e.raw[s][0]));
            }
            w = std::snprintf(line + ll, sizeof(line) - ll, ",%s", hex);
            ll += w > 0 ? static_cast<size_t>(w) : 0;
        }
        w = std::snprintf(line + ll, sizeof(line) - ll, "\n");
        ll += w > 0 ? static_cast<size_t>(w) : 0;
        if (ll >= sizeof(line)) {
            continue;
        }
        if (!put(mRows, kRowBuf, len, line, ll)) {
            ok = append(path, first ? header : nullptr, mRows, len) && ok;
            first = false;
            len = 0;
            put(mRows, kRowBuf, len, line, ll);
        }
    }
    if (len > 0) {
        ok = append(path, first ? header : nullptr, mRows, len) && ok;
    }
    mRows[0] = '\0';
    if (!ok) {
        ++mWriteErrors;
    }
    event("check", "done");
    flush();
    sendCheckLines();
    sendStatus();
}

void Service::sendCheckLines()
{
    if (!mCheckDone || !mGuiStarted) {
        return;
    }
    constexpr uint8_t per = CustomMessage::ProbeCheck::kPerMessage;
    for (uint8_t first = 0; first < kCheckCount; first = static_cast<uint8_t>(first + per)) {
        auto m = SDK::make_msg<CustomMessage::ProbeCheck>(mKernel);
        if (!m) {
            return;
        }
        m->first = first;
        m->total = kCheckCount;
        m->secsLeft = 0;
        for (uint8_t k = 0; k < per && first + k < kCheckCount; ++k) {
            const CheckEntry& e = mCheck[first + k];
            char* t = m->text[k];
            const size_t cap = sizeof(m->text[k]);
            if (e.handle == 0) {
                std::snprintf(t, cap, "%s: absent", e.name);
            } else if (!e.connected) {
                std::snprintf(t, cap, "%s: no connect", e.name);
            } else if (e.samples == 0) {
                std::snprintf(t, cap, "%s: silent", e.name);
            } else {
                std::snprintf(t, cap, "%s: %lu, %u f", e.name, static_cast<unsigned long>(e.samples),
                              static_cast<unsigned>(e.fields));
            }
            m->count = static_cast<uint8_t>(k + 1);
        }
        m.send();
    }
}
