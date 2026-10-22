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
 * Developer fields are not shown by either, but they are readable from the file,
 * which is what tuning the heat hint needs.
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
        /// NOTES 5.28). Not written when empty; truncated to kProfileNameMax.
        std::string sportProfileName;
        /// file_id.product_name. Empty keeps the SDK's "UNA Watch".
        std::string productName;
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
        uint8_t     round = 1;
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
        uint8_t     mode = 0;           // Sauna::Mode
        uint8_t     restEnabled = 0;
        uint8_t     completed = 1;      // 1 saved normally, 0 banked when the app was stopped
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

    /// Developer field definition numbers (this app's own).
    enum DevField : uint8_t {
        DF_WATCH_TEMP_X100 = 0,   // record, sint16
        DF_PHASE           = 1,   // lap, uint8: 0 sauna, 1 plunge, 2 rest
        DF_ROUND           = 2,   // lap, uint8
        DF_HR_RECOVERY     = 3,   // lap, sint16: bpm fallen a minute after the lap
        DF_MODE            = 4,   // session, uint8: 0 sauna + plunge, 1 just sauna
        DF_REST_ENABLED    = 5,   // session, uint8
        DF_COMPLETED       = 6,   // session, uint8
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
