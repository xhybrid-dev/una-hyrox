/**
 ******************************************************************************
 * @file    PlanCalendar.hpp
 * @brief   When a plan runs: its first Monday, which of its weeks are kept,
 *          and where a given day falls (brief section 7, PLANS.md 4.3).
 *
 * Weeks run Monday to Sunday. A plan set up from Monday to Wednesday starts
 * that week; later in the week it starts the next Monday. With a race date the
 * plan is counted back from the race's week, so the race week is the last;
 * if that would start before the plan can, its first weeks (the base weeks)
 * are dropped, down to the plan's minimum, and below that only the final
 * weeks are offered (TooClose).
 ******************************************************************************
 */

#ifndef XTRAINING_PLAN_CALENDAR_HPP
#define XTRAINING_PLAN_CALENDAR_HPP

#include "Date.hpp"

#include <cstdint>

namespace XTraining {

struct CalendarInput {
    Ymd     setupDay  = 0;   ///< The day the plan was set up (the watch's local date).
    Ymd     raceDate  = 0;   ///< 0 for none.
    uint8_t weekCount = 0;   ///< The plan's full length.
    uint8_t minWeeks  = 0;
};

struct Calendar {
    enum class Status : uint8_t {
        Ok,         ///< The whole plan.
        Truncated,  ///< Base weeks dropped to fit the race; at least minWeeks left.
        TooClose,   ///< Fewer than minWeeks left: only the final weeks.
        RaceGone,   ///< The race is before the plan could start, or no weeks are left.
        BadDate,    ///< No valid set-up day (the clock is not set) or race date.
    };
    Status  status    = Status::BadDate;
    Ymd     start     = 0;  ///< The first Monday run.
    uint8_t firstWeek = 0;  ///< The plan week (0-based) the first Monday runs.
    uint8_t weeks     = 0;  ///< Weeks run, from firstWeek.

    bool usable() const { return status == Status::Ok || status == Status::Truncated
                                 || status == Status::TooClose; }
};

/// Where a day falls in a calendar.
struct DayPlace {
    enum class Where : uint8_t { Before, In, After, None } where = Where::None;
    uint8_t planWeek = 0;  ///< The plan week (0-based, into PlanDef::weeks).
    uint8_t runWeek  = 0;  ///< The week as the runner counts it, 0-based.
    uint8_t weekday  = 0;  ///< 0 Monday .. 6 Sunday.
};

namespace PlanCalendar {

/// The first Monday a plan set up on @p setupDay can start.
Ymd firstStart(Ymd setupDay);

Calendar make(const CalendarInput& in);

DayPlace locate(const Calendar& c, Ymd day);

/// The date of @p weekday (0 Monday) in run week @p runWeek.
Ymd dateOf(const Calendar& c, uint8_t runWeek, uint8_t weekday);

}  // namespace PlanCalendar
}  // namespace XTraining

#endif  // XTRAINING_PLAN_CALENDAR_HPP
