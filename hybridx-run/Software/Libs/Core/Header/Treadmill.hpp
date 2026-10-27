/**
 ******************************************************************************
 * @file    Treadmill.hpp
 * @brief   HybridX Run's treadmill mode: the arithmetic around the distance
 *          the athlete enters after a run.
 *
 * Indoors the distance is integrated from cadence by the SDK's
 * TreadmillSpeedEstimator (SDK::Calibration). After Save the athlete may enter
 * the distance the treadmill's console shows; the SDK's CadenceStrideModel
 * decides which distance goes in the file (it rejects absurd entries) and
 * learns from it. What is left is here:
 *
 * - **correct()**: the session totals once that distance is known, as the SDK's
 *   Treadmill example does: the average speed from the distance and the timer
 *   time, the maximum scaled by k = final / estimate. Per-record and per-lap
 *   figures stay as recorded (the industry convention).
 * - **The entry**: the distance in hundredths of a km or mile, as the picker
 *   steps it, between kMinHundredths and kMaxHundredths, and its text ("5.03").
 *   range() narrows that to what the stride model will accept: it rejects an
 *   entry outside a band around the estimate (SDK::Calibration::Config's
 *   kDActualRatioMin and Max, passed in so this file needs no SDK), so the
 *   picker never offers one it would drop.
 *
 * Header-only, because the GUI links none of the Libs sources. No floating
 * point in the formatting.
 ******************************************************************************
 */

#ifndef RUN_TREADMILL_HPP
#define RUN_TREADMILL_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace RunVo2::Treadmill
{

inline constexpr float kMetresPerMile = 1609.344f;

/// The picker's range, in hundredths of the unit: 0.10 to 99.99.
inline constexpr uint16_t kMinHundredths = 10;
inline constexpr uint16_t kMaxHundredths = 9999;

/// The session totals after the distance is settled.
struct Totals {
    float distanceM = 0.0f;
    float speedAvg  = 0.0f;   ///< m/s, distance / timer time
    float speedMax  = 0.0f;   ///< m/s, the recorded maximum times k
    float k         = 1.0f;   ///< final / estimate; 1 when nothing changed
};

/// @p estimateM the integrated distance; @p finalM the one the stride model chose
/// (the entry if it accepted it, else the estimate); @p timerSec the active time.
inline Totals correct(float estimateM, float finalM, float timerSec, float speedMaxMs)
{
    Totals t;
    const bool okFinal = std::isfinite(finalM) && finalM > 0.0f;
    const bool okEst   = std::isfinite(estimateM) && estimateM > 1e-3f;
    t.distanceM = okFinal ? finalM : (okEst ? estimateM : 0.0f);
    t.k         = (okFinal && okEst) ? finalM / estimateM : 1.0f;
    t.speedAvg  = (std::isfinite(timerSec) && timerSec > 1e-3f) ? t.distanceM / timerSec : 0.0f;
    t.speedMax  = std::isfinite(speedMaxMs) && speedMaxMs > 0.0f ? speedMaxMs * t.k : 0.0f;
    return t;
}

/// Metres to the picker's hundredths of a km (or mile), rounded and held in range.
inline uint16_t toHundredths(float metres, bool imperial)
{
    if (!std::isfinite(metres) || metres <= 0.0f) {
        return kMinHundredths;
    }
    const float unit = imperial ? kMetresPerMile : 1000.0f;
    const float h    = metres / unit * 100.0f + 0.5f;
    if (h <= static_cast<float>(kMinHundredths)) {
        return kMinHundredths;
    }
    if (h >= static_cast<float>(kMaxHundredths)) {
        return kMaxHundredths;
    }
    return static_cast<uint16_t>(h);
}

/// The picker's hundredths back to metres.
inline float toMetres(uint16_t hundredths, bool imperial)
{
    return static_cast<float>(hundredths) / 100.0f * (imperial ? kMetresPerMile : 1000.0f);
}

/// The picker's range.
struct Range {
    uint16_t lo = kMinHundredths;
    uint16_t hi = kMaxHundredths;
};

/// The entries the stride model will accept for @p estimateM: ratioMin to ratioMax times it,
/// rounded inwards to hundredths and held inside the picker's own range. With no estimate
/// (nothing was counted) the whole range, though the model will keep the estimate anyway.
inline Range range(float estimateM, bool imperial, float ratioMin, float ratioMax)
{
    Range r;
    if (!std::isfinite(estimateM) || estimateM <= 0.0f || !(ratioMin > 0.0f) || !(ratioMax >= ratioMin)) {
        return r;
    }
    const float unit = imperial ? kMetresPerMile : 1000.0f;
    const float lo   = std::ceil(estimateM * ratioMin / unit * 100.0f - 1e-3f);
    const float hi   = std::floor(estimateM * ratioMax / unit * 100.0f + 1e-3f);
    if (lo > static_cast<float>(kMinHundredths)) {
        r.lo = lo >= static_cast<float>(kMaxHundredths) ? kMaxHundredths : static_cast<uint16_t>(lo);
    }
    if (hi < static_cast<float>(kMaxHundredths)) {
        r.hi = hi <= static_cast<float>(r.lo) ? r.lo : static_cast<uint16_t>(hi);
    }
    return r;
}

/// One step of the picker: @p delta hundredths (negative to go down), held in @p r.
inline uint16_t step(uint16_t hundredths, int delta, Range r = Range{})
{
    const int v = static_cast<int>(hundredths) + delta;
    if (v < static_cast<int>(r.lo)) {
        return r.lo;
    }
    if (v > static_cast<int>(r.hi)) {
        return r.hi;
    }
    return static_cast<uint16_t>(v);
}

/// "5.03". Returns the length (0 if @p out is unusable).
inline size_t text(char* out, size_t cap, uint16_t hundredths)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    const int n = std::snprintf(out, cap, "%u.%02u", static_cast<unsigned>(hundredths / 100u),
                                static_cast<unsigned>(hundredths % 100u));
    return n < 0 ? 0 : (static_cast<size_t>(n) < cap ? static_cast<size_t>(n) : cap - 1);
}

}  // namespace RunVo2::Treadmill

#endif  // RUN_TREADMILL_HPP
