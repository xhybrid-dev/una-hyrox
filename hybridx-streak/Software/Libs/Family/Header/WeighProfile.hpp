/**
 ******************************************************************************
 * @file    WeighProfile.hpp
 * @brief   The athlete's fixed numbers: age, and the profile's weight as an
 *          integer the GUI can format without floating point.
 *
 * Pure C++, no SDK: host-tested in Tests/Host. Used by the service only (the
 * GUI ELF links no Libs sources).
 ******************************************************************************
 */

#ifndef XWEIGHIN_WEIGH_PROFILE_HPP
#define XWEIGHIN_WEIGH_PROFILE_HPP

#include <cstdint>
#include <ctime>

namespace XWeighIn
{

/// Before this UTC (1 January 2026) the watch's clock is taken as not set:
/// after a flat battery, and before the phone syncs, time() can read 1970.
/// The same floor as HybridX Streak's.
constexpr int64_t kClockFloorUtc = 1767225600;

/// The heaviest weight the app accepts, kg. Anything above is treated as not
/// set rather than shown.
constexpr int32_t kMaxKg = 500;

/// True once the clock reads a plausible date (see kClockFloorUtc).
bool clockIsSet(std::time_t nowUtc);

/**
 * @brief   Whole years of age at nowUtc.
 *
 * Copied from HybridX Run (hybridx-run/Software/Libs/Core/Sources/
 * Vo2Profile.cpp) so the apps stay independent. A birth month of 0 counts as
 * January.
 *
 * @return  0 if the birth year is unset, the clock is not set, or the age
 *          comes out at 0 or above 120.
 */
uint8_t ageAt(uint16_t birthYear, uint8_t birthMonth, std::time_t nowUtc);

/**
 * @brief   A weight in kg as whole hundredths of a kg, rounded to nearest.
 *
 * @return  0 ("not set") for NaN, zero, negative values and anything above
 *          kMaxKg; otherwise 1..kMaxKg * 100.
 */
int32_t kgToHundredths(float kg);

}  // namespace XWeighIn

#endif  // XWEIGHIN_WEIGH_PROFILE_HPP
