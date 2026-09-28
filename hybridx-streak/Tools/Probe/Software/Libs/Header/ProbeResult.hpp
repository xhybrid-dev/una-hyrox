/**
 ******************************************************************************
 * @file    ProbeResult.hpp
 * @brief   What one run of the Streak Probe found, in a fixed-size struct.
 *
 * Header-only and trivially copyable: the service fills it, sends it to the
 * GUI inside a message, and formats one line of probe-history.txt from it.
 ******************************************************************************
 */

#ifndef STREAK_PROBE_RESULT_HPP
#define STREAK_PROBE_RESULT_HPP

#include <cstdint>
#include <type_traits>

namespace Probe
{

/// The Gate 0 answer (hybridx-streak PLAN section 3, question 1).
enum class Verdict : uint8_t {
    Running,    ///< not finished yet
    Go,         ///< another app's activity data was read: a summary by name [7], or a listed .fit
    NoFiles,    ///< other apps can be reached but hold nothing yet: record one and re-run
    NoOpen,     ///< another app's file is there but could not be opened or was not what it should be
    Blocked,    ///< no other app's folder or file could be reached at all
};

/// Result of trying one thing: not tried, worked, or refused.
enum class Check : uint8_t { NotRun, Ok, Failed };

struct Result {
    Verdict  verdict        = Verdict::Running;

    // [1] Routes out of the sandbox. "base" is the first that listed.
    Check    listParent     = Check::NotRun;   ///< ".."
    Check    listApps       = Check::NotRun;   ///< "/Apps"
    Check    listDriveApps  = Check::NotRun;   ///< "2:/Apps"
    Check    listRoot       = Check::NotRun;   ///< "/"
    char     base[8]        = {};              ///< the route the scan used

    // [2] Apps and activity files.
    uint8_t  apps           = 0;               ///< folders under the base (SharedData excluded)
    uint8_t  appsWithFit    = 0;
    uint8_t  recordingMarks = 0;               ///< Activity/.recording files seen (in-progress recordings)
    uint16_t fitFiles       = 0;               ///< Activity/YYYYMM/activity_*.fit
    uint16_t otherFit       = 0;               ///< any other .fit under Activity/
    char     newest[64]     = {};              ///< "<App>/<YYYYMM>/<file>", by the time in the name

    // [3] The newest .fit, opened.
    Check    fitOpen        = Check::NotRun;
    Check    fitSignature   = Check::NotRun;   ///< ".FIT" at byte 8

    // [3] Read speed, on the largest .fit (capped, see ProbeRunner).
    uint32_t readBytes      = 0;
    uint32_t readMs         = 0;

    // [5] and [6], and the SharedData control.
    Check    sharedData     = Check::NotRun;   ///< write, read back and remove ../SharedData/<tmp>
    Check    renameRefused  = Check::NotRun;   ///< Ok = rename onto an existing file refused, as FatFs does

    // [7] Exact paths: <route>/<App>/Activity/summary.json opened by name,
    // with no folder listing (Gate 0's first run: listing ".." went elsewhere,
    // but "../SharedData/..." files landed in /Apps/SharedData).
    uint8_t  namesTried     = 0;               ///< app names tried (built-in list + apps.txt)
    uint8_t  appsSeen       = 0;               ///< names where exist("../<App>") said yes
    uint8_t  summariesFound = 0;               ///< summary.json that exist() reported, by any route
    uint8_t  summariesRead  = 0;               ///< summary.json opened and parsed
    uint8_t  fitByName      = 0;               ///< .fit files seen in a summary's month folder
    Check    fitByNameOpen  = Check::NotRun;   ///< the first of them opened, ".FIT" at byte 8
    Check    sharedByName   = Check::NotRun;   ///< ../SharedData/HybridX/streak.json, if present
    char     summaryApp[16] = {};              ///< the most recent summary read: its app,
    char     summaryRoute[8] = {};             ///< the route that worked ("..", "/Apps", "2:/Apps"),
    uint32_t summaryUtc     = 0;               ///< and its "utc", "time" (s) and "distance" (m)
    uint32_t summarySecs    = 0;
    uint32_t summaryMetres  = 0;

    // Filled by the service, not the runner.
    Check    glance         = Check::NotRun;
    int16_t  glanceWidth    = 0;
    int16_t  glanceHeight   = 0;
    uint16_t glanceControls = 0;
    int32_t  utcOffsetMin   = 0;
    uint32_t utc            = 0;
    uint16_t run            = 0;               ///< this run's number, from probe-history.txt
};

static_assert(std::is_trivially_copyable<Result>::value, "Result travels in a message");

inline const char* verdictName(Verdict v)
{
    switch (v) {
        case Verdict::Running: return "running";
        case Verdict::Go:      return "GO";
        case Verdict::NoFiles: return "NO FILES";
        case Verdict::NoOpen:  return "NO READ";
        case Verdict::Blocked: return "BLOCKED";
    }
    return "?";
}

inline const char* checkName(Check c)
{
    return c == Check::Ok ? "ok" : (c == Check::Failed ? "FAIL" : "-");
}

/// What rename onto an existing file did (Result::renameRefused).
inline const char* renameName(Check c)
{
    return c == Check::Ok ? "refused" : (c == Check::Failed ? "replaces" : "-");
}

} // namespace Probe

#endif // STREAK_PROBE_RESULT_HPP
