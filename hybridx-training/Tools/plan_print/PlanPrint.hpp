/**
 * @file    PlanPrint.hpp
 * @brief   A whole plan for one runner, as Markdown: the paces, then every
 *          week day by day with each session's steps, targets and totals.
 *
 * Host-only (std::string). Used by Tools/plan_print and by the golden-file
 * test, so what Jon reviews is exactly what the watch would build.
 */

#ifndef XTRAINING_PLAN_PRINT_HPP
#define XTRAINING_PLAN_PRINT_HPP

#include "Date.hpp"
#include "PlanTypes.hpp"
#include "WeekLayout.hpp"

#include <cstdint>
#include <string>

namespace XTraining {

struct PrintInput {
    PlanId   plan     = PlanId::TenK;
    uint32_t fiveKSec = 0;
    uint32_t halfSec  = 0;
    bool     fiveKRecent = true;
    uint8_t  level    = 2;
    DayMask  days     = WeekLayout::kDefaultDays;
    uint8_t  longDay  = WeekLayout::kDefaultLongDay;
    bool     easyByHr = false;
    bool     repsByDistance = false;
    Ymd      setupDay = 20270104u;  ///< A Monday, so the golden files start on the 4th.
    Ymd      raceDate = 0;
};

/// The Markdown, or a line saying why there is none.
std::string printPlan(const PrintInput& in);

/// "Tue Wed Fri Sun".
std::string dayList(DayMask days);

}  // namespace XTraining

#endif  // XTRAINING_PLAN_PRINT_HPP
