/**
 ******************************************************************************
 * @file    TodayBuild.hpp
 * @brief   The app's side of training_today.json (TP6): fill the file's data
 *          from the plan, as the plan has it now. Pure; the service writes the
 *          bytes (TodayFile::write) with SafeFile.
 *
 * The rows come from PlanRules::sessionOn, so a moved, eased or rested day is
 * already in them; the status of each from the plan's log; the words from
 * PlanText::glanceWhat and the pace in the runner's units. The window is the
 * Monday of today's week through 14 days after today (TodayFile.hpp), cut to
 * the plan's own days.
 ******************************************************************************
 */

#ifndef XTRAINING_TODAY_BUILD_HPP
#define XTRAINING_TODAY_BUILD_HPP

#include "PaceModel.hpp"
#include "PlanState.hpp"
#include "TodayFile.hpp"

#include "SDK/Workout/WorkoutProgram.hpp"

#include <cstddef>
#include <cstdint>

namespace XTraining {
namespace TodayBuild {

/// "4:18/km", or "6:55/mi", from a pace in seconds per kilometre (as the app's paces are).
void paceText(uint16_t secPerKm, bool imperial, char* buf, size_t cap);

/// Fill @p out for @p today (yyyymmdd; 0 = the clock is not set) from @p st. A plan that is not
/// set, not usable, or without paces (@p paces null) gives "no plan" (plan 0): the glance then
/// says to set one up. @p scratch is a Program to build each session into for its pace; null
/// leaves the pace out (no memory for it). @p utc is written as given.
void fill(const PlanState& st, const Paces* paces, bool imperial, Ymd today, uint32_t utc,
          SDK::Workout::Program* scratch, TodayData& out);

}  // namespace TodayBuild
}  // namespace XTraining

#endif  // XTRAINING_TODAY_BUILD_HPP
