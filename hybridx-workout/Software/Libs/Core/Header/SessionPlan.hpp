/**
 ******************************************************************************
 * @file    SessionPlan.hpp
 * @brief   The kinds of workout (W1): plain, strength sets, HIIT timers (EMOM,
 *          AMRAP, Tabata, custom intervals) and a Hyrox-style circuit, each as
 *          a list of steps worked out from a few numbers. Pure C++,
 *          header-only (the GUI uses it), no heap.
 *
 * A step is one lap of the workout: a set or its rest, a minute, a work or
 * rest interval, a run or a station. A step is either timed (it ends by
 * itself after durationMs of active time) or open (the button ends it). A
 * strength set is open with a cap: the button ends it, or the cap does.
 *
 * The rules (Jon, 1 November 2026, the defaults proposed at the end of
 * Phase A):
 * - Strength: sets of open work, each followed by a timed rest, except the
 *   last. The button ends a set (rest begins) or skips a rest. A set that
 *   reaches the cap ends by itself.
 * - EMOM: N timed minutes. A buzz at each minute, a warning 3 s before.
 * - AMRAP: one timed step, the time cap. The button counts a round, and each
 *   round is a lap.
 * - Tabata: 8 rounds of 20 s work and 10 s rest, fixed.
 * - Custom intervals: work, rest and rounds.
 * - Circuit: rounds of a run then a station, in Race's station order. The
 *   button moves to the next segment.
 * The timers (EMOM, Tabata, custom) ignore the button: their clock is the
 * point. A plan ends after its last step; there is no rest after the last
 * work interval or set.
 *
 * Bounds. Each setting has a range (k*Min / k*Max) chosen so no plan has more
 * steps than WorkoutModel's kMaxLaps (64): the largest is EMOM at 60.
 * clamp() pulls any value into range, so a corrupt settings file cannot make
 * a plan that overflows.
 ******************************************************************************
 */

#ifndef WORKOUT_SESSION_PLAN_HPP
#define WORKOUT_SESSION_PLAN_HPP

#include <cstdint>

#include "WorkoutModel.hpp"

namespace Workout {

enum class SessionType : uint8_t {
    Plain = 0,      ///< W0's workout: laps on the button, an optional auto lap
    Strength,
    Emom,
    Amrap,
    Tabata,
    Custom,         ///< Custom intervals: work, rest, rounds
    Circuit,
};
constexpr uint8_t kSessionTypeCount = 7;

enum class StepKind : uint8_t { Work = 0, Rest, Run, Station };

// --- Settings ranges ---------------------------------------------------------

constexpr uint8_t  kSetsMin = 1,          kSetsMax = 20,          kSetsDefault = 3;
constexpr uint16_t kSetRestSecMin = 10,   kSetRestSecMax = 300,   kSetRestSecDefault = 60;
constexpr uint16_t kSetCapSecMin = 60,    kSetCapSecMax = 900,    kSetCapSecDefault = 300;
constexpr uint8_t  kEmomMinMin = 1,       kEmomMinMax = 60,       kEmomMinDefault = 10;
constexpr uint8_t  kAmrapMinMin = 1,      kAmrapMinMax = 60,      kAmrapMinDefault = 12;
constexpr uint16_t kWorkSecMin = 5,       kWorkSecMax = 600,      kWorkSecDefault = 40;
constexpr uint16_t kRestSecMin = 5,       kRestSecMax = 600,      kRestSecDefault = 20;
constexpr uint8_t  kRoundsMin = 1,        kRoundsMax = 30,        kRoundsDefault = 6;
constexpr uint8_t  kCircuitRoundsMin = 1, kCircuitRoundsMax = 8,  kCircuitRoundsDefault = 8;
constexpr uint16_t kRunMMin = 100,        kRunMMax = 1000,        kRunMStep = 100, kRunMDefault = 1000;

constexpr uint8_t  kTabataRounds = 8;
constexpr uint16_t kTabataWorkSec = 20;
constexpr uint16_t kTabataRestSec = 10;

/// The warning before a timed step ends.
constexpr uint32_t kWarnMs = 3000;

// --- The circuit's stations ----------------------------------------------------

/// One circuit station: a copy of HybridX Race's table (RaceData.hpp, kStations),
/// kept here so the apps stay independent; a host test checks they match.
struct CircuitStation {
    const char *name;       ///< Upper case, as Race shows it
    const char *work;       ///< "1000 m", "100 reps"
    const char *brief;      ///< A short name for the summary's rows
    uint16_t    distanceM;  ///< Metres for the FIT lap; 0 when the work is reps
};

constexpr uint8_t kCircuitStationCount = 8;
constexpr CircuitStation kCircuitStations[kCircuitStationCount] = {
    { "SKIERG",             "1000 m",   "SKIERG",     1000u },
    { "SLED PUSH",          "50 m",     "SLED PUSH",    50u },
    { "SLED PULL",          "50 m",     "SLED PULL",    50u },
    { "BURPEE BROAD JUMPS", "80 m",     "BURPEES",      80u },
    { "ROW",                "1000 m",   "ROW",        1000u },
    { "FARMERS CARRY",      "200 m",    "CARRY",       200u },
    { "SANDBAG LUNGES",     "100 m",    "LUNGES",      100u },
    { "WALL BALLS",         "100 reps", "WALL BALLS",    0u },
};

/// Station @p n from 1; out of range gives the first.
constexpr const CircuitStation &circuitStation(uint8_t n)
{
    return kCircuitStations[(n >= 1 && n <= kCircuitStationCount) ? n - 1 : 0];
}

// --- The plan ------------------------------------------------------------------

struct PlanConfig {
    SessionType type = SessionType::Plain;
    uint8_t  sets = kSetsDefault;
    uint16_t setRestSec = kSetRestSecDefault;
    uint16_t setCapSec = kSetCapSecDefault;
    uint8_t  emomMin = kEmomMinDefault;
    uint8_t  amrapMin = kAmrapMinDefault;
    uint16_t workSec = kWorkSecDefault;
    uint16_t restSec = kRestSecDefault;
    uint8_t  rounds = kRoundsDefault;
    uint8_t  circuitRounds = kCircuitRoundsDefault;
    uint16_t runM = kRunMDefault;
};

template <typename T>
constexpr T clampTo(T v, T lo, T hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/// Every value pulled into its range; an unknown type becomes Plain.
constexpr PlanConfig clamp(PlanConfig c)
{
    if (static_cast<uint8_t>(c.type) >= kSessionTypeCount) {
        c.type = SessionType::Plain;
    }
    c.sets          = clampTo<uint8_t>(c.sets, kSetsMin, kSetsMax);
    c.setRestSec    = clampTo<uint16_t>(c.setRestSec, kSetRestSecMin, kSetRestSecMax);
    c.setCapSec     = clampTo<uint16_t>(c.setCapSec, kSetCapSecMin, kSetCapSecMax);
    c.emomMin       = clampTo<uint8_t>(c.emomMin, kEmomMinMin, kEmomMinMax);
    c.amrapMin      = clampTo<uint8_t>(c.amrapMin, kAmrapMinMin, kAmrapMinMax);
    c.workSec       = clampTo<uint16_t>(c.workSec, kWorkSecMin, kWorkSecMax);
    c.restSec       = clampTo<uint16_t>(c.restSec, kRestSecMin, kRestSecMax);
    c.rounds        = clampTo<uint8_t>(c.rounds, kRoundsMin, kRoundsMax);
    c.circuitRounds = clampTo<uint8_t>(c.circuitRounds, kCircuitRoundsMin, kCircuitRoundsMax);
    c.runM          = static_cast<uint16_t>(clampTo<uint16_t>(c.runM, kRunMMin, kRunMMax) / kRunMStep * kRunMStep);
    return c;
}

/// One step of a plan.
struct Step {
    StepKind kind = StepKind::Work;
    uint32_t durationMs = 0;   ///< Timed: its length. Open: 0
    uint32_t capMs = 0;        ///< Open with a cap (a strength set): the cap. Else 0
    uint8_t  round = 0;        ///< Set, round or minute, from 1
    uint8_t  rounds = 0;       ///< How many there are
    uint8_t  station = 0;      ///< Circuit station, from 1; 0 none
    uint16_t distanceM = 0;    ///< Planned metres (circuit runs and stations); 0 none

    constexpr bool timed() const { return durationMs > 0; }
    /// When the step ends by itself, in active ms from its start; 0 never.
    constexpr uint32_t endsAfterMs() const { return durationMs > 0 ? durationMs : capMs; }
};

/// How many steps the plan has (0 for a plain workout: it has no plan).
constexpr uint8_t stepCount(const PlanConfig &raw)
{
    const PlanConfig c = clamp(raw);
    switch (c.type) {
    case SessionType::Strength: return static_cast<uint8_t>(2 * c.sets - 1);
    case SessionType::Emom:     return c.emomMin;
    case SessionType::Amrap:    return 1;
    case SessionType::Tabata:   return static_cast<uint8_t>(2 * kTabataRounds - 1);
    case SessionType::Custom:   return static_cast<uint8_t>(2 * c.rounds - 1);
    case SessionType::Circuit:  return static_cast<uint8_t>(2 * c.circuitRounds);
    case SessionType::Plain:
    default:                    return 0;
    }
}

/// Step @p i (from 0) of the plan. Out of range gives a plain open Work step.
constexpr Step stepAt(const PlanConfig &raw, uint8_t i)
{
    const PlanConfig c = clamp(raw);
    Step s;
    if (i >= stepCount(c)) {
        return s;
    }
    const uint8_t pair = static_cast<uint8_t>(i / 2 + 1);   // the round of a work/rest pair
    const bool    odd  = (i % 2) == 1;
    switch (c.type) {
    case SessionType::Strength:
        s.round  = pair;
        s.rounds = c.sets;
        if (odd) {
            s.kind = StepKind::Rest;
            s.durationMs = c.setRestSec * 1000u;
        } else {
            s.capMs = c.setCapSec * 1000u;
        }
        break;
    case SessionType::Emom:
        s.round = static_cast<uint8_t>(i + 1);
        s.rounds = c.emomMin;
        s.durationMs = 60000u;
        break;
    case SessionType::Amrap:
        s.round = 1;
        s.rounds = 1;
        s.durationMs = c.amrapMin * 60000u;
        break;
    case SessionType::Tabata:
        s.round = pair;
        s.rounds = kTabataRounds;
        s.kind = odd ? StepKind::Rest : StepKind::Work;
        s.durationMs = (odd ? kTabataRestSec : kTabataWorkSec) * 1000u;
        break;
    case SessionType::Custom:
        s.round = pair;
        s.rounds = c.rounds;
        s.kind = odd ? StepKind::Rest : StepKind::Work;
        s.durationMs = (odd ? c.restSec : c.workSec) * 1000u;
        break;
    case SessionType::Circuit:
        s.round = pair;
        s.rounds = c.circuitRounds;
        if (odd) {
            s.kind = StepKind::Station;
            s.station = pair;
            s.distanceM = circuitStation(pair).distanceM;
        } else {
            s.kind = StepKind::Run;
            s.distanceM = c.runM;
        }
        break;
    case SessionType::Plain:
    default:
        break;
    }
    return s;
}

/// The plan's length in active time if every timed step runs its course;
/// 0 when any step is open (strength, circuit, plain).
constexpr uint32_t plannedMs(const PlanConfig &c)
{
    uint32_t total = 0;
    const uint8_t n = stepCount(c);
    for (uint8_t i = 0; i < n; ++i) {
        const Step s = stepAt(c, i);
        if (!s.timed()) {
            return 0;
        }
        total += s.durationMs;
    }
    return total;
}

static_assert(2 * kSetsMax - 1 <= kMaxLaps, "strength plan must fit the laps");
static_assert(kEmomMinMax <= kMaxLaps, "EMOM plan must fit the laps");
static_assert(2 * kRoundsMax - 1 <= kMaxLaps, "custom plan must fit the laps");
static_assert(2 * kCircuitRoundsMax <= kMaxLaps, "circuit plan must fit the laps");
static_assert(kCircuitRoundsMax <= kCircuitStationCount, "one station a round");

} // namespace Workout

#endif // WORKOUT_SESSION_PLAN_HPP
