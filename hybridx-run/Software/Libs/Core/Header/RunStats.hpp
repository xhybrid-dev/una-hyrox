/**
 ******************************************************************************
 * @file    RunStats.hpp
 * @brief   The run detail page's numbers, worked out as the run goes: time in
 *          heart-rate zones, the fastest 1 km / 1 mile / 5 km, the first and
 *          second half, and aerobic drift (decoupling).
 *
 * Fed once per active second by the service, from the same Sample the VO2max
 * windows take, plus the run's distance so far. A paused second is ignored, so
 * every time here is active time. Bounded memory (about 4 KB) for a run of any
 * length; no heap, no floating point in the formatting. Pure C++.
 *
 * - **Zones.** Seconds with a good heart rate (the VO2max core's own test), by the
 *   watch's zone thresholds, as the arc on the lap face reads them: threshold i is
 *   the entry to zone i + 1, and a heart rate at or below the first threshold is in
 *   "below zone 1" (bucket 0).
 * - **Best splits.** A ring holds the active time at which each 10 m was crossed, the
 *   crossing placed within its second by interpolation. The best of a distance L is
 *   the shortest time between a boundary and the point L metres earlier (interpolated
 *   between two boundaries), taken at every boundary. So a best ends on a 10 m
 *   boundary: it can miss the true best by the pace change over under 10 m (well under
 *   a second). A one-second jump in distance of more than kMaxJumpM (a GPS catch-up or
 *   a counter reset) restarts the ring, so no split spans it; bests found earlier stay.
 * - **Halves.** Cumulative totals are kept at up to 64 checkpoints, thinned by
 *   doubling the spacing when full, so a half is read by interpolation at any run
 *   length to within the spacing (at most 3% of the run). A sharp change that falls
 *   between two checkpoints is smoothed: drift can be about 0.2 of a percentage point
 *   off for it (the host tests show 4.6 for a true 4.8).
 * - **Drift** (aerobic decoupling, Pa:HR as published by Friel): after the warm-up,
 *   the efficiency factor EF = level-equivalent speed (m/min) / average heart rate of
 *   each half; drift = (EF1 - EF2) / EF1. The level-equivalent speed is the run's
 *   grade-adjusted speed, worked out as WindowBuilder does (uphill only, ACSM's factor).
 *   Only given for a run that is long and steady enough (see DriftStatus). It does not
 *   depend on the athlete's max heart rate.
 ******************************************************************************
 */

#ifndef RUN_RUN_STATS_HPP
#define RUN_RUN_STATS_HPP

#include <cstdint>

#include "Vo2Window.hpp"

namespace RunVo2
{

namespace StatsConfig
{
/// Zones the watch's arc shows (the first five of its six thresholds).
inline constexpr uint8_t kZones = 5;

/// The distances whose fastest time is kept, metres: 1 km, 1 mile, 5 km.
inline constexpr uint8_t kSplits = 3;
inline constexpr float   kSplitM[kSplits] = {1000.0f, 1609.344f, 5000.0f};

/// One boundary every this many metres; the ring holds more than the longest split needs.
inline constexpr float    kRingStepM  = 10.0f;
inline constexpr uint16_t kRingCount  = 512;   ///< above 5000 / 10 + 1

/// A second in which the distance moved by more than this, or went backwards, is not trusted.
inline constexpr float kMaxJumpM = 20.0f;

/// Checkpoints for the halves, and the spacing the first ones start at (seconds).
inline constexpr uint8_t  kCheckpoints   = 64;
inline constexpr uint16_t kFirstSpacingS = 30;

// Drift. PROVISIONAL: for Jon to tune against real runs (NOTES, P2).
/// Active seconds after the warm-up that a run needs before drift is given.
inline constexpr uint32_t kMinDriftSec = 1800;
/// The two halves' level speeds may differ by at most this much (percent of the first).
inline constexpr float kMaxHalfSpeedDiffPct = 10.0f;
/// Each half needs a good heart rate and a moving speed for at least this share of its seconds.
inline constexpr uint8_t kMinShareSecPct = 80;
}  // namespace StatsConfig

enum class DriftStatus : uint8_t {
    Ok = 0,
    TooShort,      ///< under kMinDriftSec after the warm-up
    NoHeartRate,   ///< too few seconds with a good heart rate or a moving speed
    NotSteady,     ///< the halves were run at speeds more than kMaxHalfSpeedDiffPct apart
};

/// What the screens show. Plain data so it can travel in a message.
struct RunStatsResult {
    uint32_t    activeSec = 0;
    uint32_t    zoneSec[StatsConfig::kZones + 1] {};   ///< [0] below zone 1, [1..5] zones 1..5
    uint8_t     zoneCount = 0;                         ///< 0: no thresholds were given, the zones are not counted
    uint32_t    bestMs[StatsConfig::kSplits] {};       ///< fastest 1 km, 1 mile, 5 km; 0 = none yet
    uint16_t    gapSpeedMmps = 0;                      ///< the run's average grade-adjusted speed, mm/s; 0 = none
    float       halfPace[2] {};                        ///< s/m of the first and second half of the run; 0 = none
    uint8_t     halfHr[2] {};                          ///< their average heart rates; 0 = none
    DriftStatus driftStatus = DriftStatus::TooShort;
    int16_t     driftX10 = 0;                          ///< (EF1 - EF2) / EF1 in % x 10; positive = heart rate rose for the speed
    uint16_t    efX1000[2] {};                         ///< EF of each half, (m/min per bpm) x 1000
};

class RunStats
{
public:
    RunStats() { reset(); }

    /// A new run. @p thresholds are the watch's zone thresholds (bpm), @p count of them;
    /// the first five are used (threshold i is the entry to zone i + 1).
    void reset(const uint8_t* thresholds = nullptr, uint8_t count = 0);

    /// One second. @p distanceM is the run's distance so far (the active distance counter).
    void addSecond(const Sample& s, float distanceM);

    /// The numbers so far (a few microseconds of arithmetic; call it when sending).
    RunStatsResult result() const;

    uint32_t activeSec() const { return mNow.sec; }
    /// The run's average grade-adjusted speed, m/s; 0 when nothing moved. The same sum as
    /// WindowBuilder::avgGradeAdjustedSpeedMs() (the host tests check they agree).
    float avgGradeAdjustedSpeedMs() const;

private:
    /// Everything that accumulates, so a checkpoint is one copy.
    struct Totals {
        uint32_t sec        = 0;       ///< active seconds
        float    distM      = 0.0f;    ///< distance so far
        uint32_t hrSec      = 0;       ///< seconds with a good heart rate
        uint32_t hrSum      = 0;       ///< sum of those heart rates
        uint32_t gapSec     = 0;       ///< moving seconds
        uint32_t gapSumMmps = 0;       ///< sum of their grade-adjusted speeds, mm/s
    };

    static Totals lerp(const Totals& a, const Totals& b, double f);
    Totals totalsAt(uint32_t sec) const;
    void   addBoundary(uint32_t n, float distBeforeM, float stepM, uint32_t secBefore);
    void   checkSplits(uint32_t n);
    void   checkpoint();

    Totals   mNow;
    Totals   mCp[StatsConfig::kCheckpoints];
    uint8_t  mCpCount   = 0;
    uint16_t mCpSpacing = StatsConfig::kFirstSpacingS;

    uint32_t mZoneSec[StatsConfig::kZones + 1] {};
    uint8_t  mThreshold[StatsConfig::kZones] {};
    uint8_t  mZoneCount = 0;

    // Splits
    uint32_t mCross[StatsConfig::kRingCount] {};   ///< ms at which boundary n was crossed (n % kRingCount)
    uint32_t mLatest    = 0;                       ///< the newest boundary since the ring's origin
    float    mOriginM   = 0.0f;                    ///< the distance at the ring's origin
    float    mPrevDistM = 0.0f;
    uint32_t mBestMs[StatsConfig::kSplits] {};
};

}  // namespace RunVo2

#endif  // RUN_RUN_STATS_HPP
