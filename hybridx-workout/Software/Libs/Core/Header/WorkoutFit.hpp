/**
 ******************************************************************************
 * @file    WorkoutFit.hpp
 * @brief   A finished WorkoutModel as the values the FIT file's lap and
 *          session messages need. Pure C++, header-only, no SDK. (X Sauna's
 *          SessionFit, with calories in place of phases and recovery.)
 *
 * Times are whole seconds, worked out with lapSeconds() (LapTiming.hpp) so the
 * laps chain (each starts where the last ended) and add up to the session
 * exactly. The session's timer time is the sum of the laps' timer times, and
 * its elapsed time their sum, so the file never disagrees with itself.
 *
 * Heart rate 0 means "none"; the writer turns it into FIT's invalid value.
 *
 * Sport per kind of workout (W2). SDK/Fit/FitProfile.hpp declares only
 * Generic (0) and Training (10) of these. The rest are read from the public
 * FIT profile as fitdecode 0.11.0 ships it (tables generated from Garmin's FIT
 * SDK), the way HybridX Race sourced its sub_sport values (Race NOTES 5.11):
 *   sport      training 10, hiit 62;
 *   sub_sport  strength_training 20, cardio_training 26, hiit 70, amrap 73,
 *              emom 74, tabata 75.
 * docs/experiments/fit_check.py decodes each back by name. Whether the UNA app
 * and Strava honour them is for the watch test.
 ******************************************************************************
 */

#ifndef WORKOUT_SESSION_FIT_HPP
#define WORKOUT_SESSION_FIT_HPP

#include <cstdint>

#include "LapTiming.hpp"
#include "SessionPlan.hpp"
#include "WorkoutModel.hpp"

namespace Workout {

struct FitLap {
    uint32_t startSec = 0;      ///< From the session start
    uint32_t elapsedSec = 0;    ///< Wall time, pauses included
    uint32_t activeSec = 0;     ///< Timer time
    uint8_t  hrAvg = 0;         ///< bpm, 0 none
    uint8_t  hrMax = 0;
    uint16_t kcal = 0;          ///< Calories, whole kcal
    uint16_t restingKcal = 0;   ///< Their resting part
    bool     autoEnded = false; ///< Ended by time (lap_trigger time), else manual
    uint8_t  kind = 0;          ///< StepKind: a Rest lap is FIT intensity rest, the rest active
};

struct FitSession {
    uint32_t elapsedSec = 0;
    uint32_t timerSec = 0;
    uint8_t  hrAvg = 0;
    uint8_t  hrMax = 0;
    uint8_t  lapCount = 0;
    uint16_t kcal = 0;          ///< Calories, whole kcal
    uint16_t restingKcal = 0;   ///< Their resting part (FIT metabolic_calories)
};

/// The public FIT profile's numbers this app writes (see the file comment).
namespace fitnum {
constexpr uint8_t kSportGeneric  = 0;
constexpr uint8_t kSportTraining = 10;
constexpr uint8_t kSportHiit     = 62;
constexpr uint8_t kSubGeneric          = 0;
constexpr uint8_t kSubStrengthTraining = 20;
constexpr uint8_t kSubCardioTraining   = 26;
constexpr uint8_t kSubHiit             = 70;
constexpr uint8_t kSubAmrap            = 73;
constexpr uint8_t kSubEmom             = 74;
constexpr uint8_t kSubTabata           = 75;
// The set message's numbers are the writer's (ActivityWriter.cpp).
} // namespace fitnum

/// What a workout of each kind is called in its file.
struct FitSport {
    uint8_t     sport;
    uint8_t     subSport;
    const char *profileName;    ///< session.sport_profile_name, at most 15 characters
};

constexpr FitSport fitSportOf(SessionType t)
{
    switch (t) {
    case SessionType::Strength: return {fitnum::kSportTraining, fitnum::kSubStrengthTraining, "Strength"};
    case SessionType::Emom:     return {fitnum::kSportHiit, fitnum::kSubEmom, "EMOM"};
    case SessionType::Amrap:    return {fitnum::kSportHiit, fitnum::kSubAmrap, "AMRAP"};
    case SessionType::Tabata:   return {fitnum::kSportHiit, fitnum::kSubTabata, "Tabata"};
    case SessionType::Custom:   return {fitnum::kSportHiit, fitnum::kSubHiit, "Intervals"};
    case SessionType::Circuit:  return {fitnum::kSportTraining, fitnum::kSubCardioTraining, "Circuit"};
    case SessionType::Plain:
    default:                    return {fitnum::kSportGeneric, fitnum::kSubGeneric, "Workout"};
    }
}

/// Only strength writes FIT set messages (one a set and one a rest).
constexpr bool writesSets(SessionType t) { return t == SessionType::Strength; }

/// The HybridX activity link's sport code (hybridx-link ActivityLink.hpp,
/// HybridXLink::Sport): Strength 4, Workout 5, Hybrid 6. A host test ties the
/// numbers to the link's enum.
constexpr uint8_t linkSportCodeOf(SessionType t)
{
    switch (t) {
    case SessionType::Strength: return 4;
    case SessionType::Circuit:  return 6;
    default:                    return 5;
    }
}

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
} // namespace detail

/**
 * @brief   Fill @p out with the model's laps and @p session with its totals.
 * @return  Laps written (at most @p cap).
 */
inline uint8_t buildFit(const WorkoutModel &m, FitLap *out, uint8_t cap, FitSession &session)
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
        f.kind        = l.kind;

        session.elapsedSec += s.elapsedSec;
        session.timerSec   += s.activeSec;
    }

    const SessionSummary sum = m.summary();
    session.hrAvg    = detail::clampBpm(sum.hrAvg);
    session.hrMax    = detail::clampBpm(sum.hrMax);
    session.lapCount    = n;
    session.kcal        = detail::kcalOf(sum.kcalX10);
    session.restingKcal = detail::kcalOf(sum.restingKcalX10);
    return n;
}

} // namespace Workout

#endif // WORKOUT_SESSION_FIT_HPP
