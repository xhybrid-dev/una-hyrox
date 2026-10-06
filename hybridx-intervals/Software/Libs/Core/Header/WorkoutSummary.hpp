/**
 ******************************************************************************
 * @file    WorkoutSummary.hpp
 * @brief   A workout's totals with its repeats expanded, for the list and the
 *          preview ("6 x 400 m, about 34 min").
 ******************************************************************************
 */

#ifndef INTERVALS_WORKOUT_SUMMARY_HPP
#define INTERVALS_WORKOUT_SUMMARY_HPP

#include <cstdint>

#include "WorkoutTypes.hpp"

namespace Intervals
{

struct WorkoutSummary {
    uint32_t timeS      = 0;      ///< total of the timed steps, every pass counted
    uint32_t distanceM  = 0;      ///< total of the distance steps, every pass counted
    uint16_t stepsRun   = 0;      ///< steps the athlete will run, every pass counted
    uint16_t openSteps  = 0;      ///< of those, how many end on a press
    bool     hasTargets = false;  ///< any step with a pace or heart-rate target
};

/// Precondition: `workout` passed validate() (one repeat block at a time).
WorkoutSummary summarise(const Workout& workout);

} // namespace Intervals

#endif // INTERVALS_WORKOUT_SUMMARY_HPP
