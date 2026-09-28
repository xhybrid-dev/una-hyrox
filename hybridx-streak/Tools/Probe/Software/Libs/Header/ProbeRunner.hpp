/**
 ******************************************************************************
 * @file    ProbeRunner.hpp
 * @brief   The Streak Probe's checks, over IFileSystem, with no kernel.
 *
 * Answers hybridx-streak PLAN section 3 from inside a real watch:
 *   [1] which routes out of the app's own folder can be listed:
 *       "..", "/Apps", "2:/Apps" and "/";
 *   [2] which apps keep Activity/YYYYMM/activity_*.fit, and how many;
 *   [3] whether the newest one opens and is a FIT file (".FIT" at byte 8),
 *       and how fast the largest one reads;
 *   [5] whether ../SharedData/ can be written, read back and cleaned up;
 *   [6] whether rename onto an existing file is refused, as FatFs's is;
 *   [7] whether another app's Activity/summary.json opens by its exact path,
 *       with no folder listing, through "..", "/Apps" or "2:/Apps"; and if
 *       one does, whether that month's folder shows and opens a .fit. The
 *       app names come from a built-in list plus an optional apps.txt in the
 *       probe's own folder (one name per line). [7] decides the verdict when
 *       it reads anything, because on the watch listing ".." went elsewhere
 *       while "../SharedData/..." files landed in /Apps/SharedData.
 * (Question 4, files after a phone sync, is answered by comparing runs in
 * probe-history.txt. Question 5, the glance area, is the service's.)
 *
 * Every check is bounded: at most kMaxEntries entries are read from any one
 * directory, kMaxApps apps are scanned, kMaxReadBytes are read for the speed
 * test, and every path is built into a fixed buffer and skipped if it would
 * not fit. The only writes are two temporary files in the app's own folder
 * and one in SharedData, all removed again.
 *
 * Pure C++ over SDK::Interface::IFileSystem, so the host tests run it against
 * TreeFileSystem, including a tree where the firmware forbids leaving the
 * sandbox.
 ******************************************************************************
 */

#ifndef STREAK_PROBE_RUNNER_HPP
#define STREAK_PROBE_RUNNER_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "ProbeResult.hpp"

namespace Probe
{

/// Where the report goes, and the millisecond clock for the read test.
class Host
{
public:
    /// One line of the report, without its newline.
    virtual void line(const char* text) = 0;
    /// A millisecond clock that may wrap; only differences are used.
    virtual uint32_t nowMs() = 0;

protected:
    ~Host() = default;
};

class Runner
{
public:
    static constexpr uint16_t kMaxEntries   = 500;    ///< per directory read
    static constexpr uint8_t  kMaxApps      = 32;
    static constexpr uint8_t  kMaxMonths    = 48;     ///< per app: four years of YYYYMM folders
    static constexpr size_t   kMaxAppName   = 32;     ///< longer names are listed, not scanned
    static constexpr uint32_t kMaxReadBytes = 2u * 1024u * 1024u;
    static constexpr size_t   kLineChars    = 160;

    static constexpr const char* kSharedDir  = "../SharedData";
    static constexpr const char* kSharedTmp  = "../SharedData/hxstreak-probe.tmp";
    static constexpr const char* kRenameFrom = "probe-a.tmp";
    static constexpr const char* kRenameTo   = "probe-b.tmp";
    static constexpr const char* kAppsFile   = "apps.txt";
    static constexpr const char* kOwnApp     = "HXStreakProbe";
    static constexpr const char* kSharedFile = "../SharedData/HybridX/streak.json";
    static constexpr uint8_t     kMaxNames   = 32;

    /// The apps [7] tries without being told: UNA's built-in activity apps
    /// seen on Jon's watch or in the SDK's examples, and HybridX Race.
    static constexpr const char* kKnownApps[] = { "Walking", "Running", "Cycling", "Hiking",
                                                  "Treadmill", "Workout", "HybridXRace" };

    Runner(SDK::Interface::IFileSystem& fs, Host& host);

    /// Run every check, reporting through the host; fills everything in
    /// @p r except the service's fields (glance, clock, run number).
    void run(Result& r);

    /// One line for probe-history.txt (no newline). @p stamp is the local
    /// date and time. Returns the length written, always < @p n.
    static size_t historyLine(const Result& r, const char* stamp, char* out, size_t n);

    /// The unsigned integer part of "key": <number> in a small JSON text, e.g.
    /// "time":346 -> 346, "distance":461.431 -> 461. False if absent or negative.
    static bool jsonNumber(const char* text, const char* key, uint32_t& out);

    /// "YYYYMM" for a UTC time: the month folder an activity is filed under.
    static void monthOf(uint32_t utc, char out[7]);

private:
    void   say(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
    int    listCount(const char* path);
    void   collectApps(const char* base);
    void   scanApp(const char* base, const char* app, Result& r);
    void   consider(const char* path, const char* app, const char* month, const char* name, size_t size,
                    bool standardName, Result& r);
    void   openCandidate(Result& r);
    void   readLargest(Result& r);
    void   sharedData(Result& r);
    void   renameOver(Result& r);
    bool   writeText(const char* path, const char* text);
    bool   readText(const char* path, char* out, size_t n);
    void   loadNames();
    void   addName(const char* name, size_t len);
    void   byName(Result& r);
    void   trySummary(const char* app, Result& r);
    void   tryMonth(const char* route, const char* app, uint32_t utc, Result& r);

    SDK::Interface::IFileSystem& mFs;
    Host&                        mHost;

    // Fixed storage: nothing here grows, and none of it sits on the
    // service's 10 KB stack when the runner is a static.
    SDK::Interface::IFileSystem::ObjectInfo mInfo {};
    char     mApps[kMaxApps][kMaxAppName] {};
    uint8_t  mAppCount = 0;
    char     mCandidate[SDK::Interface::IFileSystem::skMaxPathLen] {};
    char     mCandidateName[64] {};
    char     mLargest[SDK::Interface::IFileSystem::skMaxPathLen] {};
    size_t   mLargestSize = 0;
    char     mPath[SDK::Interface::IFileSystem::skMaxPathLen] {};
    char     mMonths[kMaxMonths][8] {};    ///< one app's month folders
    char     mLine[kLineChars] {};
    char     mBuf[512] {};
    char     mNames[kMaxNames][kMaxAppName] {};
    uint8_t  mNameCount = 0;
};

/// Does @p name end with ".fit" (any case)?
bool isFit(const char* name);
/// Is @p name "activity_<anything>.fit", the SDK template's naming?
bool isActivityFit(const char* name);
/// Is @p name six digits (a YYYYMM month folder)?
bool isMonth(const char* name);

} // namespace Probe

#endif // STREAK_PROBE_RUNNER_HPP
