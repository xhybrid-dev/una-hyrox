/**
 ******************************************************************************
 * @file    RideFit.hpp
 * @brief   A finished RideModel as the values the FIT file's lap and
 *          session messages need. Pure C++, header-only, no SDK. (HybridX
 *          Workout's WorkoutFit, plus distance, speed and climb.)
 *
 * Times are whole seconds, worked out with lapSeconds() (LapTiming.hpp) so the
 * laps chain (each starts where the last ended) and add up to the session
 * exactly. The session's timer time is the sum of the laps' timer times, and
 * its elapsed time their sum, so the file never disagrees with itself.
 *
 * Heart rate 0 means "none"; the writer turns it into FIT's invalid value.
 *
 * Distance is FIT's total_distance scale (centimetres), speed is enhanced speed's
 * (mm/s), and climb is whole metres (FIT's total_ascent and total_descent). An
 * indoor ride has all of them zero; the writer then leaves them out.
 ******************************************************************************
 */

#ifndef RIDE_SESSION_FIT_HPP
#define RIDE_SESSION_FIT_HPP

#include <cstdint>

#include "LapTiming.hpp"
#include "RideModel.hpp"

namespace Ride {

struct FitLap {
    uint32_t startSec = 0;      ///< From the session start
    uint32_t elapsedSec = 0;    ///< Wall time, pauses included
    uint32_t activeSec = 0;     ///< Timer time
    uint8_t  hrAvg = 0;         ///< bpm, 0 none
    uint8_t  hrMax = 0;
    uint16_t kcal = 0;          ///< Calories, whole kcal
    uint16_t restingKcal = 0;   ///< Their resting part
    bool     autoEnded = false; ///< Ended by the auto lap (lap_trigger time), else manual
    uint32_t distCm = 0;
    uint32_t avgSpeedMmS = 0;
    uint32_t maxSpeedMmS = 0;
    uint16_t ascentM = 0;
    uint16_t descentM = 0;
};

struct FitSession {
    uint32_t elapsedSec = 0;
    uint32_t timerSec = 0;
    uint8_t  hrAvg = 0;
    uint8_t  hrMax = 0;
    uint8_t  lapCount = 0;
    uint16_t kcal = 0;          ///< Calories, whole kcal
    uint16_t restingKcal = 0;   ///< Their resting part (FIT metabolic_calories)
    uint32_t distCm = 0;
    uint32_t avgSpeedMmS = 0;
    uint32_t maxSpeedMmS = 0;
    uint16_t ascentM = 0;
    uint16_t descentM = 0;
};

namespace detail {
inline uint8_t clampBpm(uint32_t v)
{
    return v > 255u ? 255u : static_cast<uint8_t>(v);
}
/// Tenths of a kcal to whole kcal, rounded, capped at FIT's uint16 range less its invalid value.
inline uint16_t kcalOf(uint32_t x10)
{
    const uint32_t k = x10 / 10u + (x10 % 10u >= 5u ? 1u : 0u);
    return k > 65534u ? 65534u : static_cast<uint16_t>(k);
}
/// Centimetres of climb to whole metres, rounded, capped at FIT's uint16 range less its invalid value.
inline uint16_t metresOf(uint32_t cm)
{
    const uint32_t m = cm / 100u + (cm % 100u >= 50u ? 1u : 0u);
    return m > 65534u ? 65534u : static_cast<uint16_t>(m);
}
} // namespace detail

/**
 * @brief   Fill @p out with the model's laps and @p session with its totals.
 * @return  Laps written (at most @p cap).
 */
inline uint8_t buildFit(const RideModel &m, FitLap *out, uint8_t cap, FitSession &session)
{
    session = FitSession();
    const uint8_t n = m.lapCount() < cap ? m.lapCount() : cap;

    uint32_t cursorMs = 0;
    for (uint8_t i = 0; i < n; ++i) {
        const LapStats &l = m.lapAt(i);
        const LapSeconds s = lapSeconds(cursorMs, l.activeMs, l.pausedMs);
        cursorMs = (l.activeMs > UINT32_MAX - l.pausedMs || cursorMs > UINT32_MAX - l.activeMs - l.pausedMs)
                       ? UINT32_MAX
                       : cursorMs + l.activeMs + l.pausedMs;

        FitLap &f = out[i];
        f = FitLap();
        f.startSec    = s.startSec;
        f.elapsedSec  = s.elapsedSec;
        f.activeSec   = s.activeSec;
        f.hrAvg       = detail::clampBpm(l.hrAvg());
        f.hrMax       = detail::clampBpm(l.hrMax);
        f.kcal        = detail::kcalOf(m.lapKcalX10(i));
        f.restingKcal = detail::kcalOf(m.lapRestingKcalX10(i));
        f.autoEnded   = l.autoEnded;
        f.distCm      = l.distCm;
        f.avgSpeedMmS = l.avgSpeedMmS();
        f.maxSpeedMmS = l.maxSpeedMmS;
        f.ascentM     = detail::metresOf(l.ascentCm);
        f.descentM    = detail::metresOf(l.descentCm);

        session.elapsedSec += s.elapsedSec;
        session.timerSec   += s.activeSec;
    }

    const SessionSummary sum = m.summary();
    session.hrAvg    = detail::clampBpm(sum.hrAvg);
    session.hrMax    = detail::clampBpm(sum.hrMax);
    session.lapCount    = n;
    session.kcal        = detail::kcalOf(sum.kcalX10);
    session.restingKcal = detail::kcalOf(sum.restingKcalX10);
    session.distCm      = sum.distCm;
    session.avgSpeedMmS = sum.avgSpeedMmS;
    session.maxSpeedMmS = sum.maxSpeedMmS;
    session.ascentM     = detail::metresOf(sum.ascentCm);
    session.descentM    = detail::metresOf(sum.descentCm);
    return n;
}

} // namespace Ride

#endif // RIDE_SESSION_FIT_HPP
