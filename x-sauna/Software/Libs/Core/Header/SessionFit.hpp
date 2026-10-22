/**
 ******************************************************************************
 * @file    SessionFit.hpp
 * @brief   A finished SessionModel as the values the FIT file's lap and
 *          session messages need. Pure C++, header-only, no SDK.
 *
 * Times are whole seconds, worked out with lapSeconds() (LapTiming.hpp) so the
 * laps chain (each starts where the last ended) and add up to the session
 * exactly. The session's timer time is the sum of the laps' timer times, and
 * its elapsed time their sum, so the file never disagrees with itself.
 *
 * Heart rate 0 means "none"; the writer turns it into FIT's invalid value.
 ******************************************************************************
 */

#ifndef SAUNA_SESSION_FIT_HPP
#define SAUNA_SESSION_FIT_HPP

#include <cstdint>

#include "LapTiming.hpp"
#include "SessionModel.hpp"

namespace Sauna {

struct FitLap {
    uint32_t startSec = 0;      ///< From the session start
    uint32_t elapsedSec = 0;    ///< Wall time, pauses included
    uint32_t activeSec = 0;     ///< Timer time
    uint8_t  hrAvg = 0;         ///< bpm, 0 none
    uint8_t  hrMax = 0;
    uint8_t  phase = 0;         ///< Phase
    uint8_t  round = 1;
    bool     hasRecovery = false;
    int16_t  recoveryBpm = 0;   ///< Fall in heart rate a minute after the lap
};

struct FitSession {
    uint32_t elapsedSec = 0;
    uint32_t timerSec = 0;
    uint8_t  hrAvg = 0;
    uint8_t  hrMax = 0;
    uint8_t  lapCount = 0;
    uint8_t  mode = 0;          ///< Mode
    uint8_t  rest = 0;          ///< Rest phase in use (Just sauna always rests)
};

namespace detail {
inline uint8_t clampBpm(uint32_t v)
{
    return v > 255u ? 255u : static_cast<uint8_t>(v);
}
} // namespace detail

/**
 * @brief   Fill @p out with the model's laps and @p session with its totals.
 * @return  Laps written (at most @p cap).
 */
inline uint8_t buildFit(const SessionModel &m, FitLap *out, uint8_t cap, FitSession &session)
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
        f.phase       = static_cast<uint8_t>(l.phase);
        f.round       = l.round;
        f.hasRecovery = l.recovery.state == Recovery::State::Done;
        f.recoveryBpm = f.hasRecovery ? l.recovery.drop : 0;

        session.elapsedSec += s.elapsedSec;
        session.timerSec   += s.activeSec;
    }

    const SessionSummary sum = m.summary();
    session.hrAvg    = detail::clampBpm(sum.hrAvg);
    session.hrMax    = detail::clampBpm(sum.hrMax);
    session.lapCount = n;
    session.mode     = static_cast<uint8_t>(m.config().mode);
    session.rest     = (m.config().mode == Mode::SaunaOnly || m.config().rest) ? 1u : 0u;
    return n;
}

} // namespace Sauna

#endif // SAUNA_SESSION_FIT_HPP
