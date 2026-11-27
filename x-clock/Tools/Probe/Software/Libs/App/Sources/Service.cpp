/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   The Clock Probe's service (see the header and x-clock/docs/PROBE.md).
 *
 * The loop reads the clock, handles the minute if it has turned, then waits for
 * a message or the next minute, whichever comes first (Docs/service-lifecycle.md
 * 4.2: nothing paces a service). Elapsed time comes from the clock or the OS
 * tick, never from counting wake-ups. Rows are buffered and appended every ten;
 * a wrist raise is written at once, because it is the thing being measured.
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
#include "SDK/SensorLayer/DataParsers/SensorDataParserActivity.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryCharging.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryMetrics.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserFloorCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserStepCounter.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserWristMotion.hpp"

#define LOG_MODULE_PRX      "ClockProbe"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

using namespace ClockProbe;
using SDK::Sensor::Type;

static_assert(sizeof(CustomMessage::ProbeView) <= 256, "a message must fit the kernel's 256 bytes");
static_assert(sizeof(CustomMessage::ProbeClock) <= 256, "a message must fit the kernel's 256 bytes");
static_assert(sizeof(CustomMessage::GuiReport) <= 256, "a message must fit the kernel's 256 bytes");
static_assert(sizeof(CustomMessage::GuiLife) <= 256, "a message must fit the kernel's 256 bytes");

namespace
{
constexpr uint32_t kBattPeriodMs     = 60000;
constexpr uint32_t kSaneUtc          = 1577836800u;   ///< 2020-01-01: before this the clock is unset
constexpr uint32_t kStartupGraceMs   = 5000;          ///< a GUI that has not come up by then is not coming
constexpr uint32_t kScanGapMs        = 30000;         ///< a resume asks for a scan at most this often
constexpr uint16_t kMaxWristLogged   = 400;           ///< raises written to events.csv a day; the count goes on
constexpr uint16_t kMaxGestureLogged = 100;

const char* kLogDir       = "Logs";
const char* kStatePath    = "Logs/state.txt";
const char* kStateNew     = "Logs/state.new";
const char* kEventsPath   = "Logs/events.csv";
const char* kDaysPath     = "Logs/days.csv";
const char* kModePath     = "mode.txt";

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

uint32_t fnv(uint32_t h, uint32_t v)
{
    for (int i = 0; i < 4; ++i) {
        h ^= (v >> (8 * i)) & 0xFFu;
        h *= 16777619u;
    }
    return h;
}

int32_t toInt(uint32_t v)
{
    return v > 0x7FFFFFFFu ? 0x7FFFFFFF : static_cast<int32_t>(v);
}

/// A float as a scaled integer for a column, or kUnknown if it has no value or is out of range.
int32_t scaledOrUnknown(float v, int32_t scale, int64_t lo, int64_t hi)
{
    const int64_t s = scaled(v, scale);
    if (s == kNoValue || s < lo || s > hi) {
        return kUnknown;
    }
    return static_cast<int32_t>(s);
}
} // namespace

// ---------------------------------------------------------------------------------------------
// Construction and the loop
// ---------------------------------------------------------------------------------------------

Service::Service(SDK::Kernel& kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mBattLevel(Type::BATTERY_LEVEL, kBattPeriodMs, 0)
    , mBattCharge(Type::BATTERY_CHARGING, kBattPeriodMs, 0)
    , mBattMetrics(Type::BATTERY_METRICS, kBattPeriodMs, 0)
    , mWrist(Type::WRIST_MOTION, 0, 0)
    , mGesture(Type::GESTURE_RECOGNITION, 0, 0)
    , mSteps(Type::STEP_COUNTER_DAILY, 0, 0)
    , mActive(Type::ACTIVITY_TIME_DAILY, 0, 0)
    , mFloors(Type::FLOOR_COUNTER_DAILY, 0, 0)
{
    (void)kernel;
}

void Service::run()
{
    LOG_INFO("Clock Probe service started\n");
    const uint32_t startMs = mKernel.sys.getTimeMs();
    start();

    while (true) {
        const Clock c = clockNow();
        if (c.utc / 60u != mMinuteKey) {
            onMinute(c);
        }

        // Wait for a message, or for the next minute: sized from this reading, so a late wake-up
        // cannot add up to drift.
        uint32_t wait = (60u - c.utc % 60u) * 1000u;
        if (!mGuiStarted) {
            const uint32_t elapsed = static_cast<uint32_t>(mKernel.sys.getTimeMs() - startMs);
            if (elapsed >= kStartupGraceMs) {
                LOG_INFO("no GUI came up: leaving\n");
                event("stop", "no GUI", true);
                stop();
                return;
            }
            if (kStartupGraceMs - elapsed < wait) {
                wait = kStartupGraceMs - elapsed;
            }
        }

        SDK::MessageBase* msg = nullptr;
        if (!mKernel.comm.getMessage(msg, wait)) {
            continue;
        }
        bool leave = false;
        switch (msg->getType()) {
            case SDK::MessageType::COMMAND_APP_STOP:
                event("stop", "COMMAND_APP_STOP", true);
                leave = true;
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                mGuiStarted = true;
                event("gui", "run");
                refreshSettings();
                doScan("gui run", kScanGapMs);
                sendClock();
                sendView(true);
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                // A face's service is its GUI's: when the GUI goes (another face chosen), so does it.
                event("stop", "GUI stopped", true);
                leave = true;
                break;

            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto* e = static_cast<SDK::Message::Sensor::EventData*>(msg);
                SDK::Sensor::DataBatch batch(e->data, static_cast<uint16_t>(e->count), static_cast<uint16_t>(e->stride));
                onSensor(static_cast<uint16_t>(e->handle), batch);
            } break;

            case CustomMessage::GUI_REFRESH:
                refreshSettings();
                doScan("resume", kScanGapMs);
                sendClock();
                sendView(true);
                break;

            case CustomMessage::GUI_REPORT: {
                const auto* m = static_cast<CustomMessage::GuiReport*>(msg);
                mFrames.onReport(mKernel.sys.getTimeMs(), m->frames, m->resumes, m->suspends);
            } break;

            case CustomMessage::GUI_LIFE: {
                const auto* m = static_cast<CustomMessage::GuiLife*>(msg);
                char d[40];
                std::snprintf(d, sizeof(d), "frames=%lu", static_cast<unsigned long>(m->frames));
                if (m->kind == 1) {
                    mLastResumeMs = mKernel.sys.getTimeMs();
                    mHaveResume   = true;
                    event("gui_resume", d);
                } else if (m->kind == 2) {
                    event("gui_suspend", d);
                }
            } break;

            default:
                break;   // not ours: ignore, and release below
        }
        mKernel.comm.releaseMessage(msg);
        if (leave) {
            stop();
            return;
        }
    }
}

Service::Clock Service::clockNow() const
{
    Clock c;
    c.utc   = static_cast<uint32_t>(std::time(nullptr));
    c.valid = c.utc >= kSaneUtc;
    const time_t t = static_cast<time_t>(c.utc);
    struct tm tm {};
    if (localtime_r(&t, &tm) != nullptr) {
        c.day    = daysFromCivil(tm.tm_year + 1900, static_cast<uint32_t>(tm.tm_mon + 1),
                                 static_cast<uint32_t>(tm.tm_mday));
        c.hour   = static_cast<uint8_t>(tm.tm_hour % 24);
        c.minute = static_cast<uint8_t>(tm.tm_min % 60);
        const CivilDate d = civilFromDays(c.day);
        const unsigned  y = d.year < 0 ? 0u : (d.year > 9999 ? 9999u : static_cast<unsigned>(d.year));
        std::snprintf(c.text, sizeof(c.text), "%04u-%02u-%02uT%02u:%02u", y, static_cast<unsigned>(d.month),
                      static_cast<unsigned>(d.day), static_cast<unsigned>(c.hour), static_cast<unsigned>(c.minute));
    } else {
        c.day    = static_cast<int32_t>(c.utc / 86400u);
        c.hour   = static_cast<uint8_t>((c.utc / 3600u) % 24u);
        c.minute = static_cast<uint8_t>((c.utc / 60u) % 60u);
        std::snprintf(c.text, sizeof(c.text), "utc");
    }
    return c;
}

// ---------------------------------------------------------------------------------------------
// Start and stop
// ---------------------------------------------------------------------------------------------

void Service::start()
{
    // Can a face write its own folder? The answer is the first thing the screen shows.
    const bool dirOk = mKernel.fs.mkdir(kLogDir);
    loadState();
    ++mState.starts;
    const Clock c = clockNow();
    if (mState.firstUtc == 0 && c.valid) {
        mState.firstUtc = c.utc;
    }
    saveState();   // sets mWriteOk
    if (!dirOk) {
        mWriteOk = 0;
        ++mWriteErrors;
    }

    char detail[96];
    std::snprintf(detail, sizeof(detail), "start %lu, uptime %lu s, clock %s, logs dir %s",
                  static_cast<unsigned long>(mState.starts),
                  static_cast<unsigned long>(mKernel.sys.getTimeMs() / 1000u), c.valid ? "set" : "unset",
                  dirOk ? "ok" : "FAILED");
    event("start", detail);

    mDay      = c.day;
    mHourKey  = c.day * 24 + c.hour;
    mBufferDay = c.day;
    mMinuteKey = c.utc / 60u;

    readModeFile();
    connectSensors();
    refreshSettings();
    doScan("start", 0);
    flush();
}

void Service::stop()
{
    flush();
    saveState();
    applyWristMode(false);
    mBattLevel.disconnect();
    mBattCharge.disconnect();
    mBattMetrics.disconnect();
    mSteps.disconnect();
    mActive.disconnect();
    mFloors.disconnect();
}

// ---------------------------------------------------------------------------------------------
// Sensors
// ---------------------------------------------------------------------------------------------

void Service::connectSensors()
{
    auto tryConnect = [&](SDK::Sensor::Connection& c, const char* name) {
        if (!c.connect()) {
            event("connect_failed", name);
        }
    };
    // Cheap and event driven: the battery, and the watch's own day figures.
    tryConnect(mBattLevel, "BATTERY_LEVEL");
    tryConnect(mBattCharge, "BATTERY_CHARGING");
    tryConnect(mBattMetrics, "BATTERY_METRICS");
    tryConnect(mSteps, "STEP_COUNTER_DAILY");
    tryConnect(mActive, "ACTIVITY_TIME_DAILY");
    tryConnect(mFloors, "FLOOR_COUNTER_DAILY");
    // The wrist sensors are the thing whose cost is being measured: on or off by today's mode.
    const Clock c = clockNow();
    applyWristMode(wristOn(mModeSetting, c.day));
}

void Service::applyWristMode(bool on)
{
    if (on) {
        if (!mWrist.isConnected() && !mWrist.connect()) {
            event("connect_failed", "WRIST_MOTION");
        }
        if (!mGesture.isConnected() && !mGesture.connect()) {
            event("connect_failed", "GESTURE_RECOGNITION");
        }
    } else {
        if (mWrist.isConnected()) {
            mWrist.disconnect();
        }
        if (mGesture.isConnected()) {
            mGesture.disconnect();
        }
    }
    // Honest: "on" only if the sensor really connected.
    mWristOn = on && mWrist.isConnected();
}

void Service::readModeFile()
{
    mModeSetting = ModeSetting::Auto;
    if (!mKernel.fs.exist(kModePath)) {
        return;
    }
    auto file = mKernel.fs.file(kModePath);
    if (!file || !file->open(false, false)) {
        return;
    }
    char text[32] = {};
    size_t got = 0;
    const bool ok = file->read(text, sizeof(text) - 1, got);
    file->close();
    if (ok) {
        mModeSetting = parseModeText(text, got);
        event("mode", modeName(mModeSetting));
    }
}

void Service::onSensor(uint16_t handle, SDK::Sensor::DataBatch& batch)
{
    const uint16_t n = batch.size();
    if (n == 0) {
        return;
    }
    if (mBattLevel.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryLevel p(batch[n - 1]);
        if (p.isDataValid()) {
            mBattX100 = scaledOrUnknown(p.getCharge(), 100, 0, 10000);
        }
    } else if (mBattCharge.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryCharging p(batch[n - 1]);
        if (p.isDataValid()) {
            const int8_t now = p.isCharging() ? 1 : 0;
            if (now != mCharging) {
                mCharging = now;
                event("charging", now ? "on" : "off");
            }
        }
    } else if (mBattMetrics.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryMetrics p(batch[n - 1]);
        if (p.isDataValid()) {
            mMv    = scaledOrUnknown(p.getVoltage(), 1000, 0, 10000);
            mMa    = scaledOrUnknown(p.getCurrent(), 100, -10000000, 10000000);
            mAvgMa = scaledOrUnknown(p.getAverageCurrent(), 100, -10000000, 10000000);
            mMah   = scaledOrUnknown(p.getCapacity(), 10, 0, 1000000);
        }
    } else if (mWrist.matchesDriver(handle)) {
        for (uint16_t i = 0; i < n; ++i) {
            SDK::SensorDataParser::WristMotion p(batch[i]);
            if (!p.isDataValid() || !p.isWristMotion()) {
                continue;
            }
            const uint32_t nowMs = mKernel.sys.getTimeMs();
            mRaises.record(nowMs);
            if (mWristLogged < kMaxWristLogged) {
                ++mWristLogged;
                // When the GUI last woke and last reported, against the raise: did the face wake first?
                char resume[16] = "none", report[16] = "none";
                if (mHaveResume) {
                    std::snprintf(resume, sizeof(resume), "%lu", static_cast<unsigned long>(nowMs - mLastResumeMs));
                }
                const uint32_t since = mFrames.sinceReportMs(nowMs);
                if (since != 0xFFFFFFFFu) {
                    std::snprintf(report, sizeof(report), "%lu", static_cast<unsigned long>(since));
                }
                char d[80];
                std::snprintf(d, sizeof(d), "n=%lu resume_ago_ms=%s report_ago_ms=%s",
                              static_cast<unsigned long>(mRaises.today()), resume, report);
                event("wrist", d, true);
            }
            sendView(false);
        }
    } else if (mGesture.matchesDriver(handle)) {
        for (uint16_t i = 0; i < n; ++i) {
            const SDK::Sensor::DataView v = batch[i];
            ++mGestureToday;
            if (mGestureLogged < kMaxGestureLogged) {
                ++mGestureLogged;
                const uint16_t fc = v.getFieldCount();
                char d[88];
                std::snprintf(d, sizeof(d), "fields=%u u0=%lu u1=%lu u2=%lu", static_cast<unsigned>(fc),
                              static_cast<unsigned long>(fc > 0 ? v.u[0] : 0), static_cast<unsigned long>(fc > 1 ? v.u[1] : 0),
                              static_cast<unsigned long>(fc > 2 ? v.u[2] : 0));
                event("gesture", d, true);
            }
        }
    } else if (mSteps.matchesDriver(handle)) {
        SDK::SensorDataParser::StepCounter p(batch[n - 1]);
        if (p.isDataValid()) {
            mStepsDay = toInt(p.getStepCount());
        }
    } else if (mActive.matchesDriver(handle)) {
        SDK::SensorDataParser::Activity p(batch[n - 1]);
        if (p.isDataValid()) {
            mActiveDay = toInt(p.getDuration());
        }
    } else if (mFloors.matchesDriver(handle)) {
        SDK::SensorDataParser::FloorCounter p(batch[n - 1]);
        if (p.isDataValid()) {
            mFloorsDay = p.getFloorsUp();
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Time-driven work
// ---------------------------------------------------------------------------------------------

void Service::onMinute(const Clock& c)
{
    mMinuteKey = c.utc / 60u;
    if (c.valid && c.day != mDay) {
        onNewDay(c);
    }
    if (c.valid) {
        const int32_t hourKey = c.day * 24 + c.hour;
        if (hourKey != mHourKey) {
            mHourKey = hourKey;
            onNewHour(c);
        }
    }
    closeRow(c);
    sendClock();
    sendView(false);
}

void Service::onNewDay(const Clock& c)
{
    flush();   // the rows buffered so far belong to the day that ended
    mDay       = c.day;
    mBufferDay = c.day;
    mRaises.newDay();
    mGestureToday  = 0;
    mWristLogged   = 0;
    mGestureLogged = 0;
    readModeFile();
    applyWristMode(wristOn(mModeSetting, c.day));
    char d[48];
    std::snprintf(d, sizeof(d), "%s wrist=%s", modeName(mModeSetting), mWristOn ? "on" : "off");
    event("day", d);
    mWeek = makeWeek(mScan, c.day);
}

void Service::onNewHour(const Clock& c)
{
    // The watch's own day figures, once an hour: the history the ring would need (it keeps none).
    char row[64];
    const size_t n = formatDaysRow(row, sizeof(row), yyyymmdd(c.day), c.hour, mStepsDay, mActiveDay, mFloorsDay, mWristOn);
    if (!append(kDaysPath, daysHeader(), row, n)) {
        ++mWriteErrors;
    }
    refreshSettings();
    doScan("hourly", 0);
}

void Service::closeRow(const Clock& c)
{
    MinuteRow r;
    r.utc          = c.utc - c.utc % 60u;
    r.hour         = c.hour;
    r.minute       = c.minute;
    r.wristOn      = mWristOn;
    r.battX100     = mBattX100;
    r.mv           = mMv;
    r.ma           = mMa;
    r.avgMa        = mAvgMa;
    r.mah          = mMah;
    r.charging     = mCharging;
    r.steps        = mStepsDay;
    r.active       = mActiveDay;
    r.floors       = mFloorsDay;
    r.wristToday   = mRaises.today();
    r.gestureToday = mGestureToday;
    r.reports      = mFrames.reportsThisMinute();
    r.fpsX10       = mFrames.haveRate() ? static_cast<int32_t>(mFrames.fpsX10()) : kUnknown;
    r.maxGapMs     = mFrames.maxGapMs();
    r.resumes      = mFrames.resumes();
    r.suspends     = mFrames.suspends();
    r.starts       = mState.starts;
    mFrames.startMinute();

    char line[192];
    const size_t n = formatMinuteRow(line, sizeof(line), r);
    if (n == 0) {
        return;
    }
    if (!put(mRows, kRowBuf, mRowsLen, line, n)) {
        flush();
        put(mRows, kRowBuf, mRowsLen, line, n);
    }
    ++mRowsBuffered;
    if (mRowsBuffered >= kFlushRows) {
        flush();
    }
}

// ---------------------------------------------------------------------------------------------
// The scan
// ---------------------------------------------------------------------------------------------

void Service::dump(const char* text, size_t len)
{
    if (!put(mDump, kDumpBuf, mDumpLen, text, len)) {
        mDumpCut = true;
    }
}

void Service::onScanListed(void* ctx, const char*, const ListEntry& e)
{
    Service* self = static_cast<Service*>(ctx);
    if (self->mListLen + 1 < sizeof(self->mListBuf)) {
        self->mListLen += formatListEntry(self->mListBuf + self->mListLen, sizeof(self->mListBuf) - self->mListLen, e);
    }
}

void Service::onScanListDone(void* ctx, const char* dir, const ListResult& r)
{
    Service* self = static_cast<Service*>(ctx);
    char head[192];
    const size_t n = formatListHeader(head, sizeof(head), dir, r);
    self->dump(head, n);
    self->dump(self->mListBuf, self->mListLen);
    self->mListLen = 0;
    self->mListBuf[0] = '\0';
}

void Service::onScanFile(void* ctx, uint8_t index, const FileResult& r, Verdict v, const char* text)
{
    Service* self = static_cast<Service*>(ctx);
    char line[192];
    size_t n = formatFileLine(line, sizeof(line), kKnown[index], r, v);
    self->dump(line, n);
    if (text != nullptr && text[0] != '\0' && self->mDumpLen + 1 < kDumpBuf) {
        // Straight into the dump: the content is up to a kilobyte, which the stack could not spare.
        const size_t len = formatFileContent(self->mDump + self->mDumpLen, kDumpBuf - self->mDumpLen, text, kScratchBytes);
        self->mDumpLen += len;
    }
}

void Service::doScan(const char* why, uint32_t minGapMs)
{
    const uint32_t nowMs = mKernel.sys.getTimeMs();
    if (mScanned && minGapMs > 0 && static_cast<uint32_t>(nowMs - mLastScanMs) < minGapMs) {
        return;   // a resume that follows another closely: nothing new to learn
    }
    mLastScanMs = nowMs;
    const Clock c = clockNow();

    mDumpLen = 0;
    mDump[0] = '\0';
    mDumpCut = false;
    mListLen = 0;
    mListBuf[0] = '\0';
    char head[96];
    dump(head, formatScanHeader(head, sizeof(head), c.text, why));

    ScanSink sink;
    sink.listed   = &Service::onScanListed;
    sink.listDone = &Service::onScanListDone;
    sink.file     = &Service::onScanFile;
    sink.ctx      = this;
    runScan(mKernel.fs, mScratch, sizeof(mScratch), mScan, sink);
    mScanned = true;

    // A signature of what is there: the full dump is kept only when something changed.
    uint32_t sig = 2166136261u;
    for (const ScanFile& f : mScan.files) {
        sig = fnv(sig, (f.r.exists ? 1u : 0u) | (static_cast<uint32_t>(f.verdict) << 1));
        sig = fnv(sig, f.r.size);
        sig = fnv(sig, f.r.utc);
    }
    sig = fnv(sig, (static_cast<uint32_t>(mScan.shared.entries) << 16) | mScan.hybridx.entries);
    const bool changed = sig != mScanSig;
    mScanSig = sig;

    char detail[96];
    std::snprintf(detail, sizeof(detail), "%s: %u of %u found, %u bad, list %s/%s, %s", why,
                  static_cast<unsigned>(mScan.found()), static_cast<unsigned>(kKnownCount),
                  static_cast<unsigned>(mScan.problems()), mScan.shared.opened ? "ok" : "no",
                  mScan.hybridx.opened ? "ok" : "no", changed ? "changed" : "same");
    event("scan", detail);

    if (changed) {
        if (mDumpCut) {
            dump("(dump cut: out of room)\n", 25);
        }
        char path[40];
        std::snprintf(path, sizeof(path), "Logs/scan_%08lu.txt", static_cast<unsigned long>(yyyymmdd(c.day)));
        if (!append(path, nullptr, mDump, mDumpLen)) {
            ++mWriteErrors;
        }
    }
    mWeek = makeWeek(mScan, c.valid ? c.day : -1);
    sendView(false);
}

// ---------------------------------------------------------------------------------------------
// The watch's settings
// ---------------------------------------------------------------------------------------------

void Service::refreshSettings()
{
    auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel);
    if (!msg) {
        return;
    }
    if (!(msg.send(100) && msg.ok())) {
        event("settings_failed", "RequestSystemSettings");
        return;
    }
    mIs12h      = msg->timeFormat;
    mMonthFirst = msg->dateMonthFirst;
    // What a face would use. Height and weight are the wearer's own and not needed: left out.
    char hr[32] = {};
    size_t hl = 0;
    for (uint8_t i = 0; i < msg->heartRateCount && i < SDK::Message::RequestSystemSettings::skMaxHearRateTh; ++i) {
        hl += static_cast<size_t>(std::snprintf(hr + hl, sizeof(hr) - hl, "%s%u", i ? "/" : "", static_cast<unsigned>(msg->heartRateTh[i])));
        if (hl >= sizeof(hr)) {
            hl = sizeof(hr) - 1;
            break;
        }
    }
    char text[sizeof(mSettingsText)];
    std::snprintf(text, sizeof(text), "lang=%u imperial=%u h12=%u month_first=%u hr=%s steps=%lu active=%lu floors=%lu",
                  static_cast<unsigned>(msg->languageId), msg->imperialUnits ? 1u : 0u, msg->timeFormat ? 1u : 0u,
                  msg->dateMonthFirst ? 1u : 0u, hr, static_cast<unsigned long>(msg->steps),
                  static_cast<unsigned long>(msg->activityMin), static_cast<unsigned long>(msg->floors));
    if (std::strcmp(text, mSettingsText) != 0) {
        std::memcpy(mSettingsText, text, sizeof(mSettingsText));
        event("settings", text);
    }
}

// ---------------------------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------------------------

bool Service::append(const char* path, const char* header, const char* data, size_t len)
{
    // Open for writing without truncating, then write at the end (as X Weigh-in's LogStore).
    auto file = mKernel.fs.file(path);
    bool ok = file && file->open(true, false);
    if (ok) {
        const size_t size = file->size();
        ok = file->seek(size);
        size_t w = 0;
        if (ok && size == 0 && header != nullptr) {
            const size_t hl = std::strlen(header);
            ok = file->write(header, hl, w) && w == hl;
        }
        if (ok && len > 0) {
            ok = file->write(data, len, w) && w == len;
        }
        ok = file->flush() && ok;
        file->close();
    }
    mWriteOk = ok ? 1 : 0;
    return ok;
}

void Service::flush()
{
    if (mRowsLen > 0) {
        char path[40];
        std::snprintf(path, sizeof(path), "Logs/min_%08lu.csv", static_cast<unsigned long>(yyyymmdd(mBufferDay)));
        if (!append(path, minuteHeader(), mRows, mRowsLen)) {
            ++mWriteErrors;
            LOG_WARNING("could not write %s\n", path);
        }
        mRowsLen      = 0;
        mRowsBuffered = 0;
        mRows[0]      = '\0';
    }
    if (mEventsLen > 0) {
        if (!append(kEventsPath, eventsHeader(), mEvents, mEventsLen)) {
            ++mWriteErrors;
        }
        mEventsLen = 0;
        mEvents[0] = '\0';
    }
}

void Service::event(const char* what, const char* detail, bool flushNow)
{
    char line[200];
    const size_t n = formatEventLine(line, sizeof(line), static_cast<uint32_t>(std::time(nullptr)),
                                     mKernel.sys.getTimeMs() / 1000u, what, detail);
    if (n == 0) {
        return;
    }
    if (!put(mEvents, kEventBuf, mEventsLen, line, n)) {
        // Full: write what there is, then this.
        if (append(kEventsPath, eventsHeader(), mEvents, mEventsLen)) {
            mEventsLen = 0;
            mEvents[0] = '\0';
            put(mEvents, kEventBuf, mEventsLen, line, n);
        } else {
            ++mWriteErrors;
        }
    }
    LOG_INFO("event %s %s\n", what, detail != nullptr ? detail : "");
    if (flushNow) {
        flush();
    }
}

void Service::loadState()
{
    const char* paths[2] = {kStatePath, kStateNew};
    for (const char* path : paths) {
        auto file = mKernel.fs.file(path);
        if (!file || !file->open(false, false)) {
            continue;
        }
        size_t got = 0;
        const bool ok = file->read(mStateText, sizeof(mStateText) - 1, got);
        file->close();
        if (!ok) {
            continue;
        }
        mStateText[got] = '\0';
        ProbeState s;
        if (parseState(mStateText, got, s)) {
            mState = s;
            return;
        }
    }
    mState = ProbeState {};   // a new probe
}

void Service::saveState()
{
    const size_t n = formatState(mState, mStateText, sizeof(mStateText));
    if (n == 0) {
        ++mWriteErrors;
        return;
    }
    // Write the new copy, then swap it in: a reboot mid-write leaves one good copy.
    auto file = mKernel.fs.file(kStateNew);
    if (!file || !file->open(true, true)) {
        ++mWriteErrors;
        mWriteOk = 0;
        return;
    }
    size_t w = 0;
    bool ok = file->write(mStateText, n, w) && w == n;
    ok = file->flush() && ok;
    file->close();
    if (ok) {
        mKernel.fs.remove(kStatePath);
        ok = mKernel.fs.rename(kStateNew, kStatePath);
    }
    mWriteOk = ok ? 1 : 0;
    if (!ok) {
        ++mWriteErrors;
    }
}

// ---------------------------------------------------------------------------------------------
// The GUI
// ---------------------------------------------------------------------------------------------

void Service::sendClock()
{
    if (!mGuiStarted) {
        return;
    }
    auto m = SDK::make_msg<CustomMessage::ProbeClock>(mKernel);
    if (!m) {
        return;
    }
    const Clock c = clockNow();
    const CivilDate d = civilFromDays(c.day);
    m->d.valid  = c.valid ? 1 : 0;
    m->d.hour   = c.hour;
    m->d.minute = c.minute;
    m->d.wday   = static_cast<uint8_t>(weekdayOf(c.day));
    m->d.mday   = d.day;
    m->d.month  = d.month;
    m->d.is12h  = mIs12h ? 1 : 0;
    m.send();
}

void Service::buildView(ViewData& v) const
{
    const Clock    c   = clockNow();
    const uint32_t now = mKernel.sys.getTimeMs();
    rowFiles(v.rows[0], mScan);
    rowList(v.rows[1], mScan);
    rowLog(v.rows[2], mWriteOk);
    rowWrist(v.rows[3], mWristOn, mRaises.today(), mRaises.any(), mRaises.lastAgeMs(now));
    rowFrames(v.rows[4], mFrames.haveRate(), mFrames.fpsX10(), mFrames.resumes());
    rowPlan(v.rows[5], mScan, c.valid ? yyyymmdd(c.day) : 0u);
    rowVo2(v.rows[6], mScan);
    ringFromWeek(v, mWeek);
    trendFromVo2(v, mScan);
}

void Service::sendView(bool force)
{
    if (!mGuiStarted) {
        return;
    }
    ViewData v;
    buildView(v);
    if (!force && mSentView && std::memcmp(&v, &mLastView, sizeof(v)) == 0) {
        return;   // nothing the screen shows has changed
    }
    auto m = SDK::make_msg<CustomMessage::ProbeView>(mKernel);
    if (!m) {
        return;
    }
    m->d = v;
    if (m.send()) {
        mLastView = v;
        mSentView = true;
    }
}
