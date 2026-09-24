/**
 ******************************************************************************
 * @file    ActivityScanner.hpp
 * @brief   Finds new activities in every app's Activity folder (PLAN 5.1).
 *
 * Every SDK activity app, and HybridX Race, writes
 * /Apps/<App>/Activity/YYYYMM/activity_YYYYMMDDTHHMMSS.fit, named with the
 * LOCAL start time (NOTES E.3, E.9). So the scanner:
 *   1. lists ".." (that is /Apps) for app folders, skipping our own and
 *      SharedData;
 *   2. in each, lists only the month folders that the window touches (one,
 *      or two at a month end);
 *   3. takes activity_*.fit files whose local date falls in the window and
 *      that the caller has not seen, skipping the one named in that app's
 *      Activity/.recording marker (still being written);
 *   4. reads each, oldest first, up to kMaxNew per scan; the rest wait for
 *      the next scan.
 *
 * The window is the previous and current week. The caller's dedup ring, not a
 * single high-water mark, decides what is new: an activity skipped while it
 * was recording, or recovered after a crash, can carry an older start time
 * than one already counted and would otherwise never be seen (PLAN 5.5).
 *
 * A file that fails to read is not reported, so it is looked at again next
 * time (it may have been mid-write). Its session start (UTC) must agree with
 * the local time in its name to within a day, or it is skipped.
 *
 * Bounded throughout: fixed tables, named limits, no heap of its own (the
 * SDK's file objects are the only allocations, as in every SDK app).
 ******************************************************************************
 */

#ifndef STREAK_ACTIVITY_SCANNER_HPP
#define STREAK_ACTIVITY_SCANNER_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "FitSessionReader.hpp"
#include "StreakTypes.hpp"

namespace Streak
{

constexpr size_t kAppNameChars = 16;   ///< app folder names are kept to 15 characters

/// One activity found.
struct Found {
    uint32_t appKey     = 0;   ///< FNV-1a-32 of the full app folder name
    uint32_t localStart = 0;   ///< local start, seconds since 1970-01-01 (local calendar)
    int32_t  localDay   = 0;   ///< localStart / 86400
    Kind     kind       = Kind::Other;
    uint16_t minutes    = 0;   ///< total timer time, whole minutes
    char     app[kAppNameChars] {};   ///< the app folder, truncated for display
};

/// The caller's memory of what it has already counted (the model's ring).
class SeenSet
{
public:
    virtual bool seen(uint32_t appKey, uint32_t localStart) const = 0;

protected:
    ~SeenSet() = default;
};

/// What a scan did, for the log and the tests.
struct ScanStats {
    uint16_t apps       = 0;   ///< app folders looked in
    uint16_t candidates = 0;   ///< new files in the window
    uint16_t read       = 0;   ///< reported in the output
    uint16_t rejected   = 0;   ///< failed to read (retried next scan)
    uint16_t mismatched = 0;   ///< name and session start disagree
    uint16_t recording  = 0;   ///< skipped: still being recorded
    uint16_t deferred   = 0;   ///< over kMaxNew; left for the next scan
    bool     listed     = false;   ///< ".." could be listed at all
};

class ActivityScanner
{
public:
    static constexpr uint8_t  kMaxApps       = 32;
    static constexpr uint16_t kMaxEntries    = 500;   ///< read from any one directory
    static constexpr uint8_t  kMaxCandidates = 64;
    static constexpr uint8_t  kMaxNew        = 32;    ///< files read per scan
    static constexpr size_t   kMaxFolderChars = 40;   ///< longer app folders are not scanned

    /// Scan for activities with local dates in [fromDay, toDay] that @p seen
    /// does not know. Writes up to @p cap into @p out, oldest first; returns
    /// how many. @p ownFolder is this app's folder name, never scanned.
    size_t scan(SDK::Interface::IFileSystem& fs, const char* ownFolder, int32_t fromDay, int32_t toDay,
                const SeenSet& seen, Found* out, size_t cap);

    const ScanStats& stats() const { return mStats; }

    /// FNV-1a-32 of a folder name: the app half of a dedup key.
    static uint32_t appKey(const char* folder);

    /// Parse "activity_YYYYMMDDTHHMMSS.fit" into local seconds since
    /// 1970-01-01. False for any other name.
    static bool parseName(const char* name, uint32_t& localStart);

private:
    struct Candidate {
        uint8_t  app = 0;
        char     month[7] {};
        char     name[32] {};
        uint32_t localStart = 0;
    };

    void listApps(SDK::Interface::IFileSystem& fs, const char* ownFolder);
    void readMarker(SDK::Interface::IFileSystem& fs, uint8_t app, char* out, size_t n);
    void addCandidate(const Candidate& c);

    SDK::Interface::IFileSystem::ObjectInfo mInfo {};
    char             mApps[kMaxApps][kMaxFolderChars] {};
    uint8_t          mAppCount = 0;
    Candidate        mCandidates[kMaxCandidates] {};
    uint8_t          mCandidateCount = 0;
    FitSessionReader mReader;
    ScanStats        mStats {};
};

} // namespace Streak

#endif // STREAK_ACTIVITY_SCANNER_HPP
