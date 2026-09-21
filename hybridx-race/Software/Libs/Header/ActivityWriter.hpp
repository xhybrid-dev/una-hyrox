/**
 ******************************************************************************
 * @file    ActivityWriter.hpp
 * @brief   Serializes activity data to a FIT file (native SDK::Fit encoder).
 ******************************************************************************
 */

#ifndef ACTIVITY_WRITER_HPP
#define ACTIVITY_WRITER_HPP

#include <cstdint>
#include <ctime>
#include <memory>
#include <string>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Fit/FitWriter.hpp"
#include "SDK/Fit/RecordingMarker.hpp"

/**
 * @class ActivityWriter
 * @brief Serializes activity data to a FIT file.
 */
class ActivityWriter {
public:
    struct AppInfo {
        std::time_t timestamp  = 0;  // UTC
        uint32_t    appVersion = 0;  // Application version 4 bytes LE [patch, minor, major, 0]
        std::string devID;           // Developer ID (max len 16)
        std::string appID;           // Application ID (max len 16)
    };

    struct RecordData {
        enum class Field : uint8_t {
            HEART_RATE = 1u << 0,
            BATTERY    = 1u << 1,
        };

        void set(Field f)                 { mFlags |= mask(f); }
        void clear(Field f)               { mFlags &= static_cast<uint8_t>(~mask(f)); }
        void set(Field f, bool state)     { state ? set(f) : clear(f); }
        bool has(Field f) const           { return (mFlags & mask(f)) != 0; }
        void clearAll()                   { mFlags = 0; }

        std::time_t timestamp      = 0;   // UTC
        float       heartRate      = 0.0f; // bpm (arbitrated)
        uint8_t     hrSource       = 0;   // HeartRateEx::Source (0=none,1=optical,2=external)
        uint8_t     hrOpticalBpm   = 0;   // raw wrist-optical (PPG) bpm (0 = none)
        uint8_t     hrExternalBpm  = 0;   // raw external strap bpm (0 = none)
        uint8_t     batteryLevel   = 0;   // %
        uint16_t    batteryVoltage = 0;   // mV

    private:
        static constexpr uint8_t mask(Field f)
        {
            return static_cast<uint8_t>(f);
        }

        uint8_t mFlags = 0;
    };

    struct LapData {
        std::time_t timestamp        = 0;     // UTC
        std::time_t timeStart        = 0;     // UTC
        std::time_t duration         = 0;     // seconds
        std::time_t elapsed          = 0;     // seconds
        float       hrAvg            = 0.0f;  // bpm
        float       hrMax            = 0.0f;  // bpm
        // Which HYROX segment this lap was (brief 10.1). Written as developer
        // fields because the FIT profile has nothing that means "sled push".
        uint8_t     segmentType      = 0;     // 0 run, 1 roxzone in, 2 station, 3 roxzone out
        uint8_t     round            = 0;     // 1 to 8
        uint8_t     stationId        = 0;     // 1 to 8, 0 when not a station
    };

    struct TrackData {
        std::time_t timestamp          = 0;    // UTC
        std::time_t timeStart          = 0;    // UTC
        std::time_t duration           = 0;    // seconds
        std::time_t elapsed            = 0;    // seconds
        float       hrAvg              = 0.0f; // bpm
        float       hrMax              = 0.0f; // bpm
        uint8_t     raceFormat         = 0;    // 0 full, 1 half A, 2 half B
        uint8_t     roxzoneMode        = 0;    // 1 when Roxzone splitting was on
        uint8_t     completed          = 0;    // 1 finished normally, 0 ended early
        // Decision D2 is still open, so the session sport is a parameter rather
        // than a constant: Phase 3 can emit candidate files for Jon to upload.
        uint8_t     sport              = 10;   // FIT sport, default training
        uint8_t     subSport           = 0;    // FIT sub_sport, default generic
    };

    ActivityWriter(const SDK::Kernel& kernel, const char* pathToDir);

    void start(const AppInfo& info);
    void pause(std::time_t timestamp);
    void resume(std::time_t timestamp);
    void addRecord(const RecordData& record);
    void addLap(const LapData& lap);
    /// Finalize the current activity. The return value is the FIT-durability
    /// contract: true iff the FIT stream + its finish()/flush/close succeeded, so
    /// the .fit is safely on disk (the kernel auto-registers it on close, and
    /// recoverInterrupted() re-registers after a crash). The auxiliary .json
    /// summary is best-effort — a summary-only failure is logged but does NOT
    /// flip the result, so it can never suppress the activity's registration.
    bool stop(const TrackData& track);
    void discard();

    /// Finalize an activity that a previous boot left unfinished (power loss /
    /// crash mid-recording). If the recovery marker exists it names the torn
    /// .fit and the last record-complete data-end offset; the file is finalized
    /// via SDK::Fit::FitWriter::recover() and the marker is removed. Returns true
    /// only when an interrupted activity was recovered into a valid FIT file.
    /// Safe (returns false, no side effects) when no marker is present. Must run
    /// before any new activity is started.
    bool recoverInterrupted();

private:
    /// Local message types (FIT record header, 0-15).
    enum Local : uint8_t {
        L_FILE_ID = 0,
        L_DEV_ID,
        L_FIELD_DESC,   // reused for each field_description (redefined per string size)
        L_EVENT,
        L_RECORD,       // no battery
        L_RECORD_B,     // + battery
        L_LAP,
        L_SESSION,
        L_ACTIVITY,
    };

    /// Developer field definition numbers (UNA-assigned).
    enum DevField : uint8_t {
        DF_BATTERY_LEVEL    = 2,
        DF_BATTERY_VOLTAGE  = 3,
        DF_HR_SOURCE        = 4,
        DF_HR_OPTICAL       = 5,
        DF_HR_EXTERNAL      = 6,
        // Per-lap segment identity (brief 10.1). These are what lets HybridX,
        // or anything else, map a lap back to a HYROX segment.
        DF_SEGMENT_TYPE     = 7,
        DF_ROUND            = 8,
        DF_STATION_ID       = 9,
        // Per-session race identity (brief 10.1).
        DF_RACE_FORMAT      = 10,
        DF_ROXZONE_MODE     = 11,
        DF_COMPLETED        = 12,
    };

    /// Flush + marker-refresh cadence during recording (seconds of record time).
    static constexpr std::time_t skFlushIntervalSec = 30;

    const SDK::Kernel& mKernel;
    const char*        mPath = nullptr;

    std::unique_ptr<SDK::Interface::IFile> mFile = nullptr;
    std::unique_ptr<SDK::Fit::FitWriter>   mFit  = nullptr;
    SDK::Fit::RecordingMarker              mMarker;   ///< Shared crash-recovery marker.
    uint16_t    mLapCounter   = 0;
    std::time_t mLastFlushUtc = 0;   ///< Record timestamp of the last durability flush.

    void defineRecordMessages();
    void writeFieldDescription(uint8_t devFieldNum, const char* name,
                               const char* units, SDK::Fit::BaseType baseType);
    void addMessageEvent(std::time_t t, SDK::Fit::EventType type);

    bool createAndOpenFile(std::time_t utc);
    bool saveSummary(const TrackData& track);

    static std::time_t tm2epoch(const struct tm* tm);
    static std::time_t epochToLocal(std::time_t utc);
    static uint32_t unixToFitTimestamp(std::time_t unixTimestamp);
};

#endif // ACTIVITY_WRITER_HPP
