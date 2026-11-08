/**
 ******************************************************************************
 * @file    ActivityWriter.hpp
 * @brief   Serializes a sauna session to a FIT file (native SDK::Fit encoder).
 *
 * A trimmed copy of HybridX Race's ActivityWriter (the apps stay independent):
 * the same file layout, crash-recovery marker and registration contract, but
 * no distance, speed or workout, and this app's own developer fields.
 *
 * The watch's temperature is a DEVELOPER field, not the standard record
 * temperature (field 13): Gate SA0 found it is a lagging temperature inside the
 * watch, so telling Garmin or Strava it is the air temperature would be wrong.
 * Strava shows no developer fields. The UNA app shows every one as a card
 * (Jon's first session, 22 October 2026), titled from the field's name and
 * with its raw number, so the name is plain ("watch_temp") and the value is a
 * decimal (30.28), not hundredths.
 *
 * So the developer fields are chosen for those cards (Gate SA3, 8 November
 * 2026, Jon): the session carries sauna_minutes, plunge_seconds (Sauna + plunge
 * only) and rounds; the laps carry phase and hr_recovery_bpm. Whole numbers,
 * not decimals: the UNA app is seen to show an integer card as it is ("14"),
 * and a float32 0.8 is 0.800000011920929 to a reader that does not round. The
 * plunge is in seconds because a plunge is a minute or two. The old round,
 * mode, rest_enabled and completed fields made cards that meant nothing to the
 * person reading them, and are gone (their numbers are not reused).
 *
 * Laps and the session are written at stop(), not as they happen, because a
 * lap's heart-rate recovery is only known a minute after it ends. A crash
 * therefore recovers the per-second records but not the laps or the session.
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
        /// NOTES 5.28). Empty writes an empty string; truncated to kProfileNameMax.
        /// The UNA app and Strava ignore it today (NOTES, Gate SA3); it stays
        /// as the standard place for the name UNA is asked to honour.
        std::string sportProfileName;
        /// file_id.product_name. Empty keeps the SDK's "UNA Watch".
        std::string productName;
        /// A Sauna + plunge session: the session carries plunge_seconds. Just
        /// sauna has no plunge, so the field is not described at all (a card
        /// reading 0 would mean nothing).
        bool withPlunge = true;
    };

    static constexpr uint8_t kProfileNameMax = 15;

    /// record.watchTempX100 when the watch gave no usable temperature.
    static constexpr int16_t kNoTemp = INT16_MIN;

    struct RecordData {
        std::time_t timestamp    = 0;   // UTC
        uint8_t     heartRate    = 0;   // bpm, 0 = none
        int32_t     watchTempX100 = INT32_MIN;  // hundredths of the sensor's unit; INT32_MIN = none
    };

    struct LapData {
        std::time_t timestamp = 0;      // UTC, end
        std::time_t timeStart = 0;      // UTC
        std::time_t duration  = 0;      // timer seconds
        std::time_t elapsed   = 0;      // wall seconds
        uint8_t     hrAvg = 0;          // bpm, 0 = none
        uint8_t     hrMax = 0;
        uint8_t     phase = 0;          // Sauna::Phase
        bool        hasRecovery = false;
        int16_t     recoveryBpm = 0;
    };

    struct TrackData {
        std::time_t timestamp = 0;      // UTC, end
        std::time_t timeStart = 0;      // UTC
        std::time_t duration  = 0;      // timer seconds
        std::time_t elapsed   = 0;      // wall seconds
        uint8_t     hrAvg = 0;
        uint8_t     hrMax = 0;
        uint16_t    saunaMinutes = 0;     // sauna timer time, whole minutes
        uint16_t    plungeSeconds = 0;    // plunge timer time (written only withPlunge)
        uint8_t     rounds = 0;           // Sauna laps
        // Training / generic, as HybridX Race settled (its NOTES 5.12): the
        // profile has no sauna sport, and a value is only used if
        // SDK/Fit/FitProfile.hpp declares it or the public profile tables list
        // it. Whether the UNA app or Strava titles from these is Race 5.28's
        // open question.
        uint8_t     sport = 10;         // FIT sport: training
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

    /// Developer field definition numbers (this app's own). 2 (round), 4
    /// (mode), 5 (rest_enabled) and 6 (completed) were written until Gate SA3
    /// and are not reused, so an old file and a new one never disagree on a number.
    enum DevField : uint8_t {
        DF_WATCH_TEMP      = 0,   // record, float32
        DF_PHASE           = 1,   // lap, uint8: 0 sauna, 1 plunge, 2 rest
        DF_HR_RECOVERY     = 3,   // lap, sint16: bpm fallen a minute after the lap
        DF_SAUNA_MINUTES   = 7,   // session, uint16: sauna timer time, whole minutes
        DF_PLUNGE_SECONDS  = 8,   // session, uint16: plunge timer time (Sauna + plunge only)
        DF_ROUNDS          = 9,   // session, uint8: Sauna laps
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
    bool        mWithPlunge   = true;
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
