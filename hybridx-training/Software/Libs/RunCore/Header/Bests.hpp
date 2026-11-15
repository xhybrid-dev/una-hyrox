/**
 ******************************************************************************
 * @file    Bests.hpp
 * @brief   HybridX Run: the athlete's personal bests over 1 km, 1 mile and
 *          5 km, kept across runs (docs/NOTES.md, "Personal bests").
 *
 * The run detail page (RunStats) finds the fastest of each distance inside a
 * run; this keeps the fastest ever, with the run it came from, as bests.json
 * in the app's folder:
 *   {"v":1,"b":[[ms,utc],[ms,utc],[ms,utc]]}
 * in StatsConfig::kSplitM's order, [0,0] = none yet. Only outdoor runs offer
 * bests (a treadmill run's distance is an estimate): the service decides.
 *
 * A time faster than kMaxSpeedMs or slower than kMinSpeedMs over the distance
 * is not a best but a measuring fault (a GPS jump the split ring missed), and
 * is never kept; a file holding one does not read. Written and read with the
 * SDK's JSON classes, as Vo2History is; integers only. Pure C++.
 ******************************************************************************
 */

#ifndef RUN_BESTS_HPP
#define RUN_BESTS_HPP

#include <cstddef>
#include <cstdint>

#include "RunStats.hpp"

namespace RunVo2
{

class Bests
{
public:
    static constexpr uint8_t kCount = StatsConfig::kSplits;

    /// Faster than this over any distance is a fault, not a run: above the world records.
    static constexpr float kMaxSpeedMs = 7.0f;
    /// Slower than this is a walk with stops, not a best worth keeping.
    static constexpr float kMinSpeedMs = 0.5f;

    struct Best {
        uint32_t ms  = 0;   ///< 0 = none yet
        uint32_t utc = 0;   ///< the run it came from (its start), UTC seconds
    };

    void clear() { *this = Bests{}; }

    /// Offer a run's fastest splits (RunStatsResult::bestMs; 0 = none). Returns a bit per
    /// distance (bit i = kSplitM[i]) that is a new best; a first best counts as new.
    uint8_t update(const uint32_t bestMs[kCount], uint32_t runUtc);

    const Best& at(uint8_t i) const;

    /// True if @p ms is a believable time over distance @p i.
    static bool plausible(uint8_t i, uint32_t ms);

    /// Write bests.json into @p buf; returns the length, or 0 if it did not fit.
    size_t toJson(char* buf, size_t cap) const;
    /// Read bests.json. On any error the bests are left empty and false returned.
    bool fromJson(const char* text, size_t len);

    static constexpr size_t kMaxJsonBytes = 128;

private:
    Best mBest[kCount] {};
};

}  // namespace RunVo2

#endif  // RUN_BESTS_HPP
