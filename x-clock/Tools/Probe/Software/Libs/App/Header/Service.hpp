/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   The Clock Probe's service (x-clock/docs/PROBE.md, Gate F0).
 *
 * A face's service, as the SDK's faces have it: it lives as long as its GUI
 * does (it leaves on COMMAND_APP_NOTIF_GUI_STOP, or after a five second grace
 * if no GUI ever comes up), reads the clock once a turn round the loop and
 * sizes its wait from that reading (Docs/writing-a-clockface.md, step 3).
 *
 * What it measures, for the questions in x-clock/docs/NOTES.md:
 *   - the shared folder: listed, and every known file asked after and parsed,
 *     at start, on each GUI resume (30 s apart at most), and every hour;
 *   - wrist raises (and gestures), when today's mode has the sensors on;
 *   - the GUI's own ticking, from its reports;
 *   - the battery gauge every minute, tagged with the wrist mode;
 *   - the watch's day figures, once an hour: the history the ring needs.
 * Everything goes to Logs/ in the probe's own folder, over USB, and the screen
 * carries the same answers, so one photograph is enough if the log cannot be written.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <cstddef>
#include <cstdint>

#include "Commands.hpp"
#include "FrameWatch.hpp"
#include "ProbeState.hpp"
#include "ProbeText.hpp"
#include "Scan.hpp"
#include "WeekView.hpp"
#include "WristLog.hpp"
#include "WristMode.hpp"

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
    struct Clock {
        uint32_t utc      = 0;
        int32_t  day      = 0;     ///< local day number (days since 1970-01-01)
        uint8_t  hour     = 0;
        uint8_t  minute   = 0;
        bool     valid    = false; ///< the watch has a real date
        char     text[20] = {};    ///< "2026-11-26T18:42"
    };
    Clock clockNow() const;

    // --- start and stop ---
    void start();
    void stop();

    // --- time-driven work ---
    void onMinute(const Clock& c);
    void onNewDay(const Clock& c);
    void onNewHour(const Clock& c);
    void closeRow(const Clock& c);

    // --- sensors ---
    void connectSensors();
    void applyWristMode(bool on);
    void onSensor(uint16_t handle, SDK::Sensor::DataBatch& batch);
    void readModeFile();

    // --- the scan ---
    void doScan(const char* why, uint32_t minGapMs);
    static void onScanListed(void* ctx, const char* dir, const ClockProbe::ListEntry& e);
    static void onScanListDone(void* ctx, const char* dir, const ClockProbe::ListResult& r);
    static void onScanFile(void* ctx, uint8_t index, const ClockProbe::FileResult& r, ClockProbe::Verdict v,
                           const char* text);
    void dump(const char* text, size_t len);

    // --- the watch's settings ---
    void refreshSettings();

    // --- files ---
    bool append(const char* path, const char* header, const char* data, size_t len);
    void flush();
    void event(const char* what, const char* detail, bool flushNow = false);
    void loadState();
    void saveState();

    // --- the GUI ---
    void sendClock();
    void sendView(bool force);
    void buildView(ClockProbe::ViewData& v) const;

    SDK::Kernel& mKernel;

    // Sensors.
    SDK::Sensor::Connection mBattLevel;
    SDK::Sensor::Connection mBattCharge;
    SDK::Sensor::Connection mBattMetrics;
    SDK::Sensor::Connection mWrist;
    SDK::Sensor::Connection mGesture;
    SDK::Sensor::Connection mSteps;
    SDK::Sensor::Connection mActive;
    SDK::Sensor::Connection mFloors;

    // What the sensors last said (kUnknown until then).
    int32_t mBattX100 = ClockProbe::kUnknown;
    int32_t mMv       = ClockProbe::kUnknown;
    int32_t mMa       = ClockProbe::kUnknown;
    int32_t mAvgMa    = ClockProbe::kUnknown;
    int32_t mMah      = ClockProbe::kUnknown;
    int8_t  mCharging = -1;
    int32_t mStepsDay   = ClockProbe::kUnknown;
    int32_t mActiveDay  = ClockProbe::kUnknown;
    int32_t mFloorsDay  = ClockProbe::kUnknown;

    // The wrist, and the GUI's ticking.
    ClockProbe::ModeSetting mModeSetting = ClockProbe::ModeSetting::Auto;
    bool                    mWristOn     = false;
    ClockProbe::WristLog    mRaises;
    uint32_t                mGestureToday = 0;
    uint16_t                mWristLogged  = 0;    ///< raises written to events.csv today (capped)
    uint16_t                mGestureLogged = 0;
    ClockProbe::FrameWatch  mFrames;
    uint32_t                mLastResumeMs = 0;
    bool                    mHaveResume   = false;

    // State.
    ClockProbe::ProbeState  mState;
    bool                    mGuiStarted   = false;
    bool                    mIs12h        = false;
    bool                    mMonthFirst   = false;
    int32_t                 mDay          = -1;    ///< the local day the loop is in
    int32_t                 mHourKey      = -1;    ///< day * 24 + hour of the last hour gate
    uint32_t                mMinuteKey    = 0;     ///< utc / 60 of the last minute handled
    int8_t                  mWriteOk      = -1;    ///< -1 not tried, 0 the last write failed, 1 worked
    char                    mSettingsText[160] = {};

    // The scan and what the screen takes from it.
    ClockProbe::ScanData    mScan;
    ClockProbe::WeekView    mWeek;
    uint32_t                mScanSig       = 0;
    bool                    mScanned       = false;
    uint32_t                mLastScanMs    = 0;
    ClockProbe::ViewData    mLastView;
    bool                    mSentView      = false;

    // Write buffers (fixed: no allocation in the loop).
    static constexpr size_t   kRowBuf      = 2048;
    static constexpr size_t   kEventBuf    = 1536;
    static constexpr size_t   kDumpBuf     = 12288;
    static constexpr uint32_t kFlushRows   = 10;
    char     mRows[kRowBuf]     = {};
    size_t   mRowsLen           = 0;
    uint32_t mRowsBuffered      = 0;
    int32_t  mBufferDay         = 0;   ///< the local day the buffered rows belong to
    char     mEvents[kEventBuf] = {};
    size_t   mEventsLen         = 0;
    char     mDump[kDumpBuf]    = {};
    size_t   mDumpLen           = 0;
    bool     mDumpCut           = false;
    char     mListBuf[2304]     = {};   ///< the folder being listed: its entry lines, until its header is written
    size_t   mListLen           = 0;
    char     mScratch[ClockProbe::kScratchBytes] = {};
    char     mStateText[96]     = {};
    uint32_t mWriteErrors       = 0;
};

#endif // SERVICE_HPP
