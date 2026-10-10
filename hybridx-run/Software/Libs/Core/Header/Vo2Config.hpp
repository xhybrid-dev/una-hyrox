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

namespace RunVo2::Config
{

/// One window is this many seconds of running.
inline constexpr uint16_t kWindowSec = 60;

/// Seconds in a window that must be usable (good HR, speed and grade).
inline constexpr uint16_t kMinGoodSecPerWindow = 55;

/// Active seconds ignored at the start of a run, while heart rate catches up.
inline constexpr uint32_t kWarmUpSec = 300;

/// Slowest usable speed. ACSM gives its running equation for speeds above
/// 134 m/min (about 7:28 per km); slower is the walking equation's range.
inline constexpr float kMinSpeedMs = 134.0f / 60.0f;

/// Fastest believable speed (a 2:00 per km pace).
inline constexpr float kMaxSpeedMs = 8.33f;

/// Steady speed: the window's spread (standard deviation over mean), percent.
inline constexpr float kMaxSpeedSpreadPct = 8.0f;

/// Steady heart rate: highest minus lowest in the window, bpm.
inline constexpr uint8_t kMaxHrRangeBpm = 10;

/// Gradient limits, percent. ACSM's equation covers level and uphill running,
/// so steeper downhill windows are dropped and gentle downhill counts as 0.
inline constexpr float kMaxGradePct = 10.0f;
inline constexpr float kMinGradePct = -3.0f;

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

/// HR trust levels the kernel calls good (RunLVGL's own test, 1 to 3).
inline constexpr uint8_t kMinHrTrust = 1;
inline constexpr uint8_t kMaxHrTrust = 3;

/// Believable heart-rate range, bpm.
inline constexpr uint8_t kMinHrBpm = 30;
inline constexpr uint8_t kMaxHrBpm = 230;

/// Auto max HR: seconds a reading must be held to count.
inline constexpr uint8_t kAutoMaxHoldSec = 5;

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

}  // namespace RunVo2::Config

#endif  // RUN_VO2_CONFIG_HPP
