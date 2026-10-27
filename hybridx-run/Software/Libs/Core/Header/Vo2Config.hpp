/**
 ******************************************************************************
 * @file    Vo2Config.hpp
 * @brief   The VO2max estimate's tunable limits, in one place.
 *
 * PROVISIONAL. These are starting values chosen for HybridX Run's test, not
 * published constants: Jon tunes them against a Garmin (docs/NOTES.md, R1).
 * The two that come from a source say so.
 ******************************************************************************
 */

#ifndef RUN_VO2_CONFIG_HPP
#define RUN_VO2_CONFIG_HPP

#include <cstdint>

#include "Vo2WindowConfig.hpp"   // the builder's limits (also in Trail)

namespace RunVo2::Config
{

/// Seconds in a window that must be usable (good HR, speed and grade).
inline constexpr uint16_t kMinGoodSecPerWindow = 55;

/// Steady speed: the window's spread (standard deviation over mean), percent.
inline constexpr float kMaxSpeedSpreadPct = 8.0f;

/// Steady heart rate: highest minus lowest in the window, bpm. 12 since L1
/// (15 October 2026): on Jon's 14 October run eight windows missed 10 by 1-3 bpm.
inline constexpr uint8_t kMaxHrRangeBpm = 12;

/// Gradient (limits in Vo2WindowConfig.hpp). A window is dropped when its mean
/// grade is outside them, or when more than this many of its seconds were: one
/// barometric spike no longer costs a level minute (L1, 15 October 2026).
inline constexpr uint8_t kMaxSteepSecPerWindow = 10;

/// Lowest intensity used, as percent of heart-rate reserve: the HR to oxygen
/// link is loosest at easy effort.
inline constexpr float kMinHrrPct = 50.0f;

/// Window estimates outside this range are taken as bad data, ml/kg/min.
inline constexpr float kMinPlausibleVo2 = 15.0f;
inline constexpr float kMaxPlausibleVo2 = 95.0f;

/// A run needs this many accepted windows for an estimate.
inline constexpr uint16_t kMinWindows = 5;

/// Accepted windows kept per run (4 hours of running).
inline constexpr uint16_t kMaxWindows = 240;

/// Auto max HR is the second-highest of the last kHistoryRuns runs' sustained
/// maxima: a value must be reached on two runs, so one artefact (an optical
/// sensor locking on to cadence) cannot raise it, and an old value leaves as
/// its run rolls out of the history.
inline constexpr uint8_t kAutoMaxConfirmRuns = 2;

/// Resting HR range accepted from the watch or the athlete, bpm.
inline constexpr uint8_t kMinRestingHr = 30;
inline constexpr uint8_t kMaxRestingHr = 100;

/// Max HR must exceed resting HR by at least this much, bpm.
inline constexpr uint8_t kMinHrReserve = 40;

/// Runs kept in the history, and how many of the latest make the shown value.
inline constexpr uint8_t kHistoryRuns = 10;
inline constexpr uint8_t kRollingRuns = 5;

/// A run's weight in the shown value is its window count, capped here.
inline constexpr uint16_t kMaxRunWeight = 30;

// ---- P3 (27 October 2026): reliability, MEASURED FIRST --------------------------------------
// These are worked out and written to the tuning log, but change nothing in the estimate until Jon
// has compared the app with his Garmin (NOTES, P3). All PROVISIONAL.

/// HR lag: in the minute or two after the pace changes, or after a pause, heart rate has not caught
/// up with the pace, which makes the minute read as a higher VO2max. A window is "after a change"
/// when the window before it was at a pace more than this far from its own (percent), or was not
/// the minute just before (a pause or a dropped part in between), or had no speed.
inline constexpr float kLagMaxSpeedChangePct = 10.0f;
/// When true, a window after a change is not used (outcome Lagged). While false it is used and
/// flagged, and RunResult::withoutLagX10 says what the estimate would have been without it.
inline constexpr bool kLagGuardEnabled = false;

/// Drift: the halves of a run's windows each need this many for their own median.
inline constexpr uint16_t kMinHalfWindows = 3;

/// The shown value's age limit, measured only (Vo2History::rollingX10Within): runs older than this
/// would not count. Not switched on: doing that also changes the glance's file and the trend.
inline constexpr uint16_t kMaxRunAgeDays = 42;

}  // namespace RunVo2::Config

#endif  // RUN_VO2_CONFIG_HPP
