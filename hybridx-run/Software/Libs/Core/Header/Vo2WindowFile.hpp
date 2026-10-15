/**
 ******************************************************************************
 * @file    Vo2WindowFile.hpp
 * @brief   One run's window records as a JSON file, for the shared folder.
 *
 * HybridX Trail writes one of these per saved run (kWindowFilePaths: three
 * slots in ../SharedData/HybridX/, the oldest overwritten); HybridX Run reads them, judges each window with its
 * own profile and rules, and adds the run to its history.
 *
 *   {"v":1,"id":1791980037,"max":179,
 *    "w":[[420,60,1870,18,2997,0,1623,160,165],...]}
 *
 * id = the run's start (UTC seconds), max = the run's sustained max HR (0 =
 * none), and each window is [activeSec, goodSec, speedMMinX10, spreadX10,
 * gradeX10 + 3000, steepSec, hrX10, hrMin, hrMax]. Integers only, none
 * negative (grade is offset by 3000, so -3.0% to +3.0% is stored 0 to 6000). The reader bounds
 * every value (the SDK's reader does no range check) and a file that is
 * missing, malformed or from a newer version reads as nothing.
 *
 * Copied byte for byte into Trail (with Vo2Window.*). The SDK's JSON reader
 * searches from the start of the text for each value, so it is read one
 * window at a time by index and Run skips runs it has already counted.
 ******************************************************************************
 */

#ifndef RUN_VO2_WINDOW_FILE_HPP
#define RUN_VO2_WINDOW_FILE_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/JSON/JsonStreamReader.hpp"

#include "Vo2Window.hpp"

namespace RunVo2
{

/// Grade is stored offset by this, so the file has no negative numbers.
inline constexpr int16_t kGradeOffsetX10 = 3000;

/// Windows written per run (two hours of steady minutes). Longer runs: the
/// writer keeps the first ones.
inline constexpr uint16_t kWindowFileMaxWindows = 120;

/// Trail keeps its last few runs in this many files; Run reads them all. Fixed
/// names: nothing depends on listing the shared folder (not shown to work).
inline constexpr uint8_t kWindowFileSlots = 3;
inline constexpr const char* kWindowFilePaths[kWindowFileSlots] = {
    "../SharedData/HybridX/trail_vo2_0.json",
    "../SharedData/HybridX/trail_vo2_1.json",
    "../SharedData/HybridX/trail_vo2_2.json",
};

/// A buffer big enough for any file the writer makes.
inline constexpr size_t kWindowFileBytes = 8192;

struct WindowFileInfo {
    uint32_t runId     = 0;   ///< the run's start, UTC seconds
    uint8_t  maxHr     = 0;   ///< the run's sustained max HR; 0 = none
    uint16_t count     = 0;   ///< windows in the file
};

/// Write a run. Returns the length, or 0 if it did not fit or the arguments
/// are bad. Writes at most kWindowFileMaxWindows windows.
size_t writeWindowFile(char* buf, size_t cap, uint32_t runId, uint8_t maxHr,
                       const WindowRecord* windows, uint16_t count);

class WindowFileReader
{
public:
    WindowFileReader(const char* text, size_t len) : mReader(text, len) {}

    /// Validate the file and read its header. False if it is not a version 1
    /// window file, or any header value is out of range.
    bool open(WindowFileInfo& info);

    /// Window i (0 .. count-1), every value bounded. activeSec, goodSec,
    /// speed, spread, grade, steepSec, hrX10, hrMin, hrMax are filled;
    /// index = i + 1; the speed correction is 0 (not in the file). False
    /// if the window is missing or any value is out of range.
    bool window(uint16_t i, WindowRecord& out) const;

private:
    SDK::JsonStreamReader mReader;
};

}  // namespace RunVo2

#endif  // RUN_VO2_WINDOW_FILE_HPP
