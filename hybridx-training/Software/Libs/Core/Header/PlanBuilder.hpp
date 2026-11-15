/**
 ******************************************************************************
 * @file    PlanBuilder.hpp
 * @brief   A plan session, at the runner's level and paces, as the
 *          SDK::Workout::Program the watch runs (PLANS.md sections 3 and 4.1).
 *
 * Two steps:
 *   - forLevel(): the plan file's level 2 session made into the runner's
 *     level (1, 2 or 3), with the caps of PLANS.md 3.3 applied whatever the
 *     file says, and the half's long-run distance floor;
 *   - build(): that session as program steps, with targets from the paces.
 *
 * Every step is filled twice, as the SDK's own buildIntervals() does: the
 * interpreted fields the engine runs, and the raw FIT fields FitWorkoutWriter
 * writes into the activity file (NOTES.md, "What was read in the SDK"). The
 * host tests write every session to FIT and read it back.
 *
 * Alongside the program, SessionMeta says how each step alerts (easy steps
 * only for "too fast": REVIEW.md C3) and what the session adds up to.
 ******************************************************************************
 */

#ifndef XTRAINING_PLAN_BUILDER_HPP
#define XTRAINING_PLAN_BUILDER_HPP

#include "PaceModel.hpp"
#include "PlanTypes.hpp"

#include "SDK/Workout/WorkoutProgram.hpp"

#include <cstdint>

namespace XTraining {

/// How a step's target zone may alert (the service filters ZoneAlerts by it).
enum class AlertMode : uint8_t {
    Both = 0,  ///< Too fast and too slow.
    FastOnly,  ///< Easy steps: only too fast (or heart rate too high).
    None,      ///< No target, or a step too short to judge live.
    AtEnd,     ///< Short reps: judged by the rep's average when it ends.
};

/// The limits per session at a level (PLANS.md 3.3). Times in seconds.
struct LevelCaps {
    uint16_t intervalSec;        ///< Interval work in a session, 30-30s' fast halves included.
    uint16_t intervalRepMaxSec;  ///< The longest interval rep.
    uint16_t tempoMin;           ///< Continuous tempo.
    uint16_t cruiseSec;          ///< Cruise intervals in all.
    uint16_t repsM;              ///< Repetitions in all; 0: none at this level (hills instead).
    uint16_t pace10kSec;
    uint16_t paceHalfSec;
    uint16_t finishMaxMin;       ///< The fast finish of a long run.
    uint16_t longMaxMin;
    uint16_t warmupMin;
};

struct BuildOptions {
    uint8_t level          = 2;      ///< 1, 2 or 3.
    bool    easyByHr       = false;  ///< Easy steps target the watch's zone 2.
    bool    repsByDistance = false;  ///< Paced reps by distance (rounded to 100 m), not time.
};

/// What forLevel() needs to know beyond the session.
struct Scaling {
    const PlanDef* plan = nullptr;
    uint8_t  level = 2;
    uint16_t longPermille = 1000;  ///< The half's distance floor: long runs x this / 1000.
};

struct SessionMeta {
    Session   session;              ///< As run, after forLevel().
    AlertMode alert[SDK::Workout::kMaxSteps] = {};
    uint32_t  plannedSec = 0;       ///< Estimated, open steps included as a typical length.
    uint32_t  plannedM   = 0;       ///< Estimated from each step's pace.
    uint32_t  qualitySec = 0;       ///< Time on steps faster than easy.
    uint32_t  repM       = 0;       ///< About how far one main rep is, to 10 m; 0 if none.
    uint16_t  targetPace = 0;       ///< The main steps' pace, s/km; 0 if none.
    uint16_t  targetPlusMinus = 0;  ///< Its band, +/- s/km.
};

namespace PlanBuilder {

/// The jog after an interval rep, a cruise rep or a race-pace rep, when the
/// plan file gives none (PLANS.md 3.1).
uint16_t defaultJogSec(Kind k, uint16_t repSec);

const LevelCaps& caps(uint8_t level);

/// The scaling for a plan at a level; @p paces sets the half's distance floor.
Scaling scaling(const PlanDef& plan, uint8_t level, const Paces& paces);

/// The plan file's session at the runner's level, capped.
Session forLevel(const Session& s, const Scaling& sc);

/// @p s (already forLevel()) as a program, into @p out. @p raceCm is the plan's
/// race distance (for Kind::Race). False for Kind::Rest or bad input; @p out
/// is then empty.
bool build(const Session& s, const Paces& paces, const BuildOptions& opt, uint32_t raceCm,
           SDK::Workout::Program& out, SessionMeta& meta);

}  // namespace PlanBuilder
}  // namespace XTraining

#endif  // XTRAINING_PLAN_BUILDER_HPP
