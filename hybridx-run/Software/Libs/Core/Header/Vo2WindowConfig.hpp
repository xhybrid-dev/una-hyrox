/**
 ******************************************************************************
 * @file    Vo2WindowConfig.hpp
 * @brief   The limits the window builder needs: what makes a second usable and
 *          what a window is.
 *
 * This file and Vo2Window.hpp/.cpp are copied byte for byte into HybridX Trail
 * (CI checks they agree). Anything that only Run's judging or estimate uses
 * lives in Vo2Config.hpp, so Run can be tuned without touching Trail.
 ******************************************************************************
 */

#ifndef RUN_VO2_WINDOW_CONFIG_HPP
#define RUN_VO2_WINDOW_CONFIG_HPP

#include <cstdint>

namespace RunVo2::Config
{

/// One window is this many seconds of running.
inline constexpr uint16_t kWindowSec = 60;

/// Active seconds ignored at the start of a run, while heart rate catches up.
inline constexpr uint32_t kWarmUpSec = 300;

/// Slowest usable speed. ACSM gives its running equation for speeds above
/// 134 m/min (about 7:28 per km); slower is the walking equation's range.
inline constexpr float kMinSpeedMs = 134.0f / 60.0f;

/// Fastest believable speed (a 2:00 per km pace).
inline constexpr float kMaxSpeedMs = 8.33f;

/// Gradient limits, percent. ACSM's equation covers level and uphill running,
/// so steeper downhill windows are dropped and gentle downhill counts as 0.
/// The builder counts the seconds outside them; Run judges the count.
inline constexpr float kMaxGradePct = 10.0f;
inline constexpr float kMinGradePct = -3.0f;

/// Average grade-adjusted speed: seconds at or above this speed count as
/// moving; the grade used is clamped to 0..kGapMaxGradePct (ACSM covers uphill
/// only, and a barometric spike must not run away); the uphill gain is ACSM's
/// own, VO2 = 3.5 + 0.2 v (1 + 4.5 grade), so the speed is as if level.
inline constexpr float kMinMovingMs        = 0.5f;
inline constexpr float kGapMaxGradePct     = 20.0f;
inline constexpr float kAcsmGradeFactor    = 0.9f / 0.2f;

/// HR trust levels the kernel calls good: RunLVGL's own test on the raw
/// float, 1 to 3 inclusive (its Service.cpp, the FIT heart-rate gate).
inline constexpr float kMinHrTrust = 1.0f;
inline constexpr float kMaxHrTrust = 3.0f;

/// Believable heart-rate range, bpm.
inline constexpr uint8_t kMinHrBpm = 30;
inline constexpr uint8_t kMaxHrBpm = 230;

/// Auto max HR: seconds a reading must be held to count. Readings in the
/// warm-up never count (optical sensors are least reliable there).
inline constexpr uint8_t kAutoMaxHoldSec = 5;

}  // namespace RunVo2::Config

#endif  // RUN_VO2_WINDOW_CONFIG_HPP
