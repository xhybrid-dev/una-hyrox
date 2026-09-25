/**
 ******************************************************************************
 * @file    ProbeResult.hpp
 * @brief   What one run of the Intervals Probe found, in a fixed-size struct.
 *
 * Header-only and trivially copyable: the service fills it, sends it to the
 * GUI inside a message, and formats one line of probe-history.txt from it.
 ******************************************************************************
 */

#ifndef INTERVALS_PROBE_RESULT_HPP
#define INTERVALS_PROBE_RESULT_HPP

#include <cstdint>
#include <ctime>
#include <type_traits>

namespace Probe
{

/// The P0 question (hybridx-intervals PLAN P0): did a file written by an
/// external BLE client land where the app can read it?
enum class Verdict : uint8_t {
    Running,       ///< not finished yet
    Go,            ///< a file was found in Plans/ and read back
    Empty,         ///< the Plans/ folder exists but is empty: no write arrived (yet)
    FolderFailed,  ///< the Plans/ folder itself could not be created or opened
};

/// Not tried, worked, or refused.
enum class Check : uint8_t { NotRun, Ok, Failed };

struct Result {
    Verdict verdict    = Verdict::Running;
    Check   folderOk   = Check::NotRun;   ///< Plans/ created or already existed, and opened

    uint16_t fileCount  = 0;              ///< entries in Plans/ (files only)
    char     newestName[64] = {};
    uint32_t newestSize = 0;
    time_t   newestUtc  = 0;
    char     preview[96] = {};            ///< the newest file's first bytes, sanitised to print safely

    uint32_t utc          = 0;            ///< the watch's clock, for lining up with the sender's timestamp
    int32_t  utcOffsetMin = 0;
    uint16_t run          = 0;            ///< this run's number, from probe-history.txt
};

static_assert(std::is_trivially_copyable<Result>::value, "Result travels in a message");

inline const char* verdictName(Verdict v)
{
    switch (v) {
        case Verdict::Running:      return "running";
        case Verdict::Go:           return "GO";
        case Verdict::Empty:        return "EMPTY";
        case Verdict::FolderFailed: return "FOLDER FAILED";
    }
    return "?";
}

inline const char* checkName(Check c)
{
    return c == Check::Ok ? "ok" : (c == Check::Failed ? "FAIL" : "-");
}

} // namespace Probe

#endif // INTERVALS_PROBE_RESULT_HPP
