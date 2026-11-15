/**
 ******************************************************************************
 * @file    PlanSchedule.hpp
 * @brief   Which session falls on a day: the plan, its calendar and the
 *          runner's days put together (brief section 7).
 *
 *   - Run week 0 is the plan's time-trial week when the 5K is not recent
 *     (REVIEW.md C5), or its "recent" week 1 when it is and the plan has one,
 *     unless run week 0 is already the race week.
 *   - In the race week, with a race date, the week is placed round the race
 *     day, which joins the runner's days if it was not one.
 *   - The session is then made into the runner's level (PlanBuilder::forLevel).
 ******************************************************************************
 */

#ifndef XTRAINING_PLAN_SCHEDULE_HPP
#define XTRAINING_PLAN_SCHEDULE_HPP

#include "PaceModel.hpp"
#include "PlanBuilder.hpp"
#include "PlanCalendar.hpp"
#include "PlanState.hpp"
#include "PlanTypes.hpp"
#include "WeekLayout.hpp"

#include <cstdint>

namespace XTraining {

struct DaySession {
    Ymd     date     = 0;
    uint8_t runWeek  = 0;
    uint8_t planWeek = 0;
    uint8_t weekday  = 0;
    Phase   phase    = Phase::Base;
    Role    role     = Role::Rest;
    bool    key      = false;  ///< The week's key session.
    Session written;           ///< As the plan file has it (level 2).
    Session asRun;             ///< At the runner's level, capped.
};

namespace PlanSchedule {

/// The week a run week uses (the time-trial and recent-week substitutions).
const Week& weekFor(const PlanDef& plan, const Calendar& cal, uint8_t runWeek, bool fiveKRecent);

/// The layout of a run week (the race week round the race day).
Layout layoutFor(const PlanSetup& setup, const Calendar& cal, uint8_t runWeek);

/// The seven days of run week @p runWeek. False if the plan or week is not usable.
bool week(const PlanSetup& setup, const PlanDef& plan, const Calendar& cal, const Paces& paces,
          uint8_t runWeek, DaySession out[7]);

/// The session on @p day; false if the day is not in the plan.
bool day(const PlanSetup& setup, const PlanDef& plan, const Calendar& cal, const Paces& paces, Ymd day,
         DaySession& out);

}  // namespace PlanSchedule
}  // namespace XTraining

#endif  // XTRAINING_PLAN_SCHEDULE_HPP
