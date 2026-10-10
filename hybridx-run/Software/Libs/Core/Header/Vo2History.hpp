/**
 ******************************************************************************
 * @file    Vo2History.hpp
 * @brief   The last runs' estimates, the value shown, and their files.
 *
 * The phone deletes activity files after sync, so the app keeps its own small
 * history: a fixed ring of the last kHistoryRuns estimates and the auto max
 * HR, saved as vo2.json in the app's folder. The value shown is the mean of
 * the latest kRollingRuns, each weighted by its window count (capped).
 *
 * The JSON is written and read here as plain text, so the host tests cover it;
 * the service does the file I/O. Integers only (no %f):
 *   {"v":1,"autoMaxHr":188,"runs":[[1791000000,523,24],...]}   oldest first
 * Anything malformed reads as an empty history, never a crash.
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

    /// Add a run, dropping the oldest when full.
    void add(const RunRecord& r);

    uint8_t size() const { return mCount; }

    /// i = 0 is the oldest kept.
    const RunRecord& at(uint8_t i) const;

    /// The value to show, x 10; 0 when there are no runs.
    uint16_t rollingX10() const;

    uint8_t autoMaxHr() const { return mAutoMaxHr; }
    /// Raise (never lower) the auto max HR.
    void raiseAutoMaxHr(uint8_t bpm);

    /// Write vo2.json into buf; returns the length, or 0 if it did not fit.
    size_t toJson(char* buf, size_t cap) const;

    /// Read vo2.json. On any error the history is left empty and false returned.
    bool fromJson(const char* text, size_t len);

    /// The shared file other HybridX apps may read
    /// (../SharedData/HybridX/vo2max.json):
    ///   {"v":1,"vo2maxX10":523,"runs":5,"utc":1791000000}
    size_t toSharedJson(char* buf, size_t cap) const;

    /// Largest toJson() output, for sizing buffers.
    static constexpr size_t kMaxJsonBytes = 64 + Config::kHistoryRuns * 32;

private:
    RunRecord mRuns[Config::kHistoryRuns] = {};
    uint8_t   mFirst = 0;
    uint8_t   mCount = 0;
    uint8_t   mAutoMaxHr = 0;
};

}  // namespace RunVo2

#endif  // RUN_VO2_HISTORY_HPP
