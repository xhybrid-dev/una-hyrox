/**
 ******************************************************************************
 * @file    PlanProgress.hpp
 * @brief   How the plan is going, for the This week and Plan screens (TP3):
 *          each day's mark, and the counts of sessions done.
 *
 * A past day with nothing logged reads "not logged" until PlanRules::settle() logs it
 * missed (TP5, brief section 9). DayMark is in PlanTypes.hpp, for the GUI; the functions are
 * the service's.
 ******************************************************************************
 */

#ifndef XTRAINING_PLAN_PROGRESS_HPP
#define XTRAINING_PLAN_PROGRESS_HPP

#include "PlanSchedule.hpp"
#include "PlanState.hpp"

#include <cstdint>

namespace XTraining {

namespace PlanProgress {

/// The mark for @p day, given @p today and the plan's log.
DayMark mark(const DaySession& day, Ymd today, const PlanState& state);

struct Counts {
    uint16_t total        = 0;  ///< Sessions in the plan as run (rest days not counted).
    uint16_t soFar        = 0;  ///< Sessions before today, and today's if it is done.
    uint16_t done         = 0;  ///< Of all, logged done or part done.
    uint8_t  weekSessions = 0;  ///< In the week shown (weekToShow).
    uint8_t  weekDone     = 0;
};

/// The run week to show on @p today: today's week in the plan, the first before it
/// starts, the last once it is over.
uint8_t weekToShow(const Calendar& cal, Ymd today);

/// The counts for @p state's plan on @p today. All zero if the plan is not usable.
Counts counts(const PlanState& state, const PlanDef& plan, const Paces& paces, Ymd today);

}  // namespace PlanProgress
}  // namespace XTraining

#endif  // XTRAINING_PLAN_PROGRESS_HPP
