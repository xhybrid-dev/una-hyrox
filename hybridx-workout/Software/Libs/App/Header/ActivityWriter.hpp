/**
 ******************************************************************************
 * @file    ActivityWriter.hpp
 * @brief   Serializes a workout to a FIT file (native SDK::Fit encoder).
 *
 * X Sauna's trimmed copy of HybridX Race's ActivityWriter (the apps stay
 * independent): the same file layout, crash-recovery marker and registration
 * contract, with no distance, speed or temperature. What it writes follows the
 * SDK's Workout example (Examples/Apps/Workout, ActivityWriter.cpp):
 *   - record: heart rate;
 *   - lap: times, average and maximum heart rate, total_calories (field 11),
 *     lap_trigger (field 24: manual 0, time 1 for an auto lap; public profile,
 *     checked in fitdecode), and the lap's resting calories as the example's
 *     developer field "resting_calories" (kcal; the public profile has no lap
 *     field for it);
 *   - session: total_calories and metabolic_calories (FitProfile.hpp's 196,
 *     as the example writes it), sport and sub_sport (generic in W0, as the
 *     example), and sport_profile_name.
 *
 * Laps and the session are written at stop(), not as they happen (X Sauna's
 * design, kept). A crash therefore recovers the per-second records but not the
 * laps or the session.
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

class ActivityWriter {
public:
    struct AppInfo {
        std::time_t timestamp  = 0;  // UTC
        uint32_t    appVersion = 0;  // Application version 4 bytes LE [patch, minor, major, 0]
        std::string devID;           // Developer ID (max len 16)
        std::string appID;           // Application ID (max len 16)
        /// session.sport_profile_name (field 110, a name the file gives its own
        /// activity profile; the number is the public FIT profile's, as Race
        /// NOTES 5.28). Not written when empty; truncated to kProfileNameMax.
        std::string sportProfileName;
        /// file_id.product_name. Empty keeps the SDK's "UNA Watch".
        std::string productName;
    };

    static constexpr uint8_t kProfileNameMax = 15;

    struct RecordData {
        std::time_t timestamp    = 0;   // UTC
        uint8_t     heartRate    = 0;   // bpm, 0 = none
    };

    struct LapData {
        std::time_t timestamp = 0;      // UTC, end
        std::time_t timeStart = 0;      // UTC
        std::time_t duration  = 0;      // timer seconds
        std::time_t elapsed   = 0;      // wall seconds
        uint8_t     hrAvg = 0;          // bpm, 0 = none
        uint8_t     hrMax = 0;
        uint16_t    kcal = 0;           // whole kcal
        uint16_t    restingKcal = 0;
        bool        autoEnded = false;  // lap_trigger time, else manual
    };

    struct TrackData {
        std::time_t timestamp = 0;      // UTC, end
        std::time_t timeStart = 0;      // UTC
        std::time_t duration  = 0;      // timer seconds
        std::time_t elapsed   = 0;      // wall seconds
        uint8_t     hrAvg = 0;
        uint8_t     hrMax = 0;
        uint16_t    kcal = 0;
        uint16_t    metabolicKcal = 0;  // the resting part
        // Generic / generic, as the SDK's Workout example writes it. W2 sets the
        // sport per workout type, from numbers checked in the public profile.
        uint8_t     sport = 0;          // FIT sport: generic
        uint8_t     subSport = 0;       // FIT sub_sport: generic
    };

    ActivityWriter(const SDK::Kernel& kernel, const char* pathToDir);

    void start(const AppInfo& info);
    void pause(std::time_t timestamp);
    void resume(std::time_t timestamp);
    /// False when the record could not be written (the screen says so).
    bool addRecord(const RecordData& record);
    void addLap(const LapData& lap);
    /// Finalize the current activity. True iff the FIT stream and its
    /// finish/flush/close succeeded, so the .fit is safely on disk (the kernel
    /// registers it when it closes, and recoverInterrupted() re-registers after
    /// a crash). The .json summary is best-effort and cannot flip the result.
    bool stop(const TrackData& track);
    void discard();

    /// Finalize an activity a previous boot left unfinished. Returns true only
    /// when one was recovered. Safe (false, no side effects) when there is none.
    /// Must run before a new activity is started.
    bool recoverInterrupted();

private:
    /// Local message types (FIT record header, 0-15).
    enum Local : uint8_t {
        L_FILE_ID = 0,
        L_DEV_ID,
        L_FIELD_DESC,   // reused for each field_description (redefined per string size)
        L_EVENT,
        L_RECORD,
        L_LAP,
        L_SESSION,
        L_ACTIVITY,
    };

    /// Developer field definition numbers (this app's own).
    enum DevField : uint8_t {
        DF_LAP_RESTING_CAL = 0,   // lap, uint16 kcal (the SDK's Workout example's)
    };

    /// Flush + marker-refresh cadence during recording (seconds of record time).
    static constexpr std::time_t skFlushIntervalSec = 30;

    const SDK::Kernel& mKernel;
    const char*        mPath = nullptr;

    std::unique_ptr<SDK::Interface::IFile> mFile = nullptr;
    std::unique_ptr<SDK::Fit::FitWriter>   mFit  = nullptr;
    SDK::Fit::RecordingMarker              mMarker;
    uint16_t    mLapCounter   = 0;
    std::string mProfileName;
    std::time_t mLastFlushUtc = 0;

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
