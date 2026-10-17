/**
 ******************************************************************************
 * @file    Vo2History.hpp
 * @brief   The last runs' estimates, the value shown, and their files.
 *
 * The phone deletes activity files after sync, so the app keeps its own small
 * history, saved as vo2.json in the app's folder:
 *   - the last kHistoryRuns estimates; the value shown is the mean of the
 *     latest kRollingRuns, each weighted by its window count (capped);
 *   - the last kHistoryRuns runs' sustained max HR; the auto max HR is the
 *     second-highest of them (reached on two runs, kAutoMaxConfirmRuns);
 *   - the last plausible resting HR the watch reported, for a run that ends
 *     before today's figure arrives.
 *
 * Written and read with the SDK's JSON writer and reader (coreJSON), as HybridX
 * Streak's state file is, so the host tests cover it; the service does the file
 * I/O. Integers only:
 *   {"v":2,"restHr":52,"maxes":[186,184],"runs":[[1791000000,523,24],...]}
 * oldest first. Version 1 files (R1-R3: "autoMaxHr" instead of "maxes" and
 * "restHr") are read. Unknown keys are ignored, so a newer file still reads;
 * a known key with a bad value fails the whole file (the service then tries
 * the backup), never a crash.
 ******************************************************************************
 */

#ifndef RUN_VO2_HISTORY_HPP
#define RUN_VO2_HISTORY_HPP

#include <cstddef>
#include <cstdint>

#include "Vo2Config.hpp"

namespace RunVo2
{

struct RunRecord {
    uint32_t utc     = 0;   ///< run end
    uint16_t vo2x10  = 0;
    uint16_t windows = 0;
};

class Vo2History
{
public:
    void clear() { *this = Vo2History{}; }

    /// Add a run's estimate, dropping the oldest when full.
    void add(const RunRecord& r);

    uint8_t size() const { return mCount; }

    /// i = 0 is the oldest kept.
    const RunRecord& at(uint8_t i) const;

    /// The value to show, x 10; 0 when there are no runs.
    uint16_t rollingX10() const { return rollingOver(mCount); }

    /// The value as it was before the latest run (for the trend); 0 when
    /// there was none.
    uint16_t rollingX10Before() const { return mCount ? rollingOver(static_cast<uint8_t>(mCount - 1)) : 0; }

    /// Record a saved run's sustained max HR (0 or implausible: ignored),
    /// dropping the oldest when full.
    void addRunMax(uint8_t bpm);

    uint8_t runMaxCount() const { return mMaxCount; }

    /// The auto max HR: the kAutoMaxConfirmRuns-th highest run max kept; 0
    /// until that many runs have one.
    uint8_t autoMaxHr() const;

    /// The watch's resting HR, kept only if plausible (Vo2Profile's range).
    void    setWatchRestHr(uint8_t bpm);
    uint8_t watchRestHr() const { return mWatchRestHr; }

    /// Write vo2.json into buf; returns the length, or 0 if it did not fit.
    size_t toJson(char* buf, size_t cap) const;

    /// Read vo2.json (version 1 or 2). On any error the history is left
    /// empty and false returned.
    bool fromJson(const char* text, size_t len);

    /// The shown figure as it stood after each kept run, oldest first, x 10
    /// (the same weighting as rollingX10()). Writes at most max values;
    /// returns how many.
    uint8_t trendX10(uint16_t* out, uint8_t max) const;

    /// The shared file other HybridX apps (and the VO2max glance) read
    /// (../SharedData/HybridX/vo2max.json), version 2: version 1's keys plus
    /// the figure before the latest run and the trend, oldest first:
    ///   {"v":2,"vo2maxX10":523,"runs":5,"utc":1791000000,"beforeX10":519,
    ///    "trend":[505,512,519,523]}
    size_t toSharedJson(char* buf, size_t cap) const;

    /// Largest toSharedJson() output, for sizing buffers.
    static constexpr size_t kMaxSharedJsonBytes = 96 + Config::kHistoryRuns * 6;

    /// Largest toJson() output, for sizing buffers.
    static constexpr size_t kMaxJsonBytes = 64 + Config::kHistoryRuns * (32 + 4);

private:
    /// The weighted mean of the latest kRollingRuns of the first n runs kept.
    uint16_t rollingOver(uint8_t n) const;

    RunRecord mRuns[Config::kHistoryRuns] = {};
    uint8_t   mFirst = 0;
    uint8_t   mCount = 0;

    uint8_t   mMaxes[Config::kHistoryRuns] = {};
    uint8_t   mMaxFirst = 0;
    uint8_t   mMaxCount = 0;

    uint8_t   mWatchRestHr = 0;
};

}  // namespace RunVo2

#endif  // RUN_VO2_HISTORY_HPP
