/**
 ******************************************************************************
 * @file    Vo2TrailIngest.hpp
 * @brief   Run's side of the Trail link: judge a window file from Trail.
 *
 * HybridX Trail leaves its last three runs in ../SharedData/HybridX/
 * (Vo2WindowFile.hpp). At start-up the service reads the headers, takes the
 * runs newer than the history's Trail mark oldest first, and for each one
 * loads its windows into a Vo2Run through addWindow(): the same judging as a
 * run Run recorded itself, with this app's profile. The service does the file
 * I/O and the history update; this is the part the host tests can reach.
 ******************************************************************************
 */

#ifndef RUN_VO2_TRAIL_INGEST_HPP
#define RUN_VO2_TRAIL_INGEST_HPP

#include <cstddef>
#include <cstdint>

#include "Vo2Run.hpp"
#include "Vo2WindowFile.hpp"

namespace RunVo2
{

enum class IngestStatus : uint8_t {
    NotAFile,        ///< empty, malformed or from a newer version: nothing to count
    AlreadyCounted,  ///< a valid file whose run is at or before the mark
    Corrupt,         ///< a valid header but a window out of range: not counted
    Judged,          ///< the run holds the file's windows; estimate() it
};

/// Judge one file. On Judged the run has been reset and holds the file's
/// windows and sustained max HR. @p info is filled whenever the header read
/// (so a Corrupt run's ID can still be marked, and never retried).
/// @p keptOnly: the file holds only the minutes that were kept (Trail's, and a stored Trail run),
/// so the HR-lag guard flags only a known change of pace (Vo2Run::setLagKnownOnly). A stored run
/// of Run's own holds every minute it built, and is judged as it was live.
IngestStatus judgeWindowFile(const char* text, size_t len, uint32_t markUtc,
                             Vo2Run& run, WindowFileInfo& info, bool keptOnly = true);

/// Which slot to read next: the one with the smallest run ID above the mark.
/// ids[i] = 0 means slot i holds no readable file. -1 when there is none.
int nextWindowFileSlot(const uint32_t* ids, uint8_t count, uint32_t markUtc);

}  // namespace RunVo2

#endif  // RUN_VO2_TRAIL_INGEST_HPP
