/**
 ******************************************************************************
 * @file    HrZones.hpp
 * @brief   Heart-rate zone from a live sample and the watch's own thresholds.
 *
 * Generalises the Workout example's Service::getHrZone
 * (una-sdk/Examples/Apps/Workout/Software/Libs/Sources/Service.cpp:904-916)
 * to an arbitrary threshold count, up to the SDK's real ceiling
 * (SDK::Message::RequestSystemSettings::skMaxHearRateTh = 7,
 * una-sdk/Libs/Header/SDK/Messages/CommandMessages.hpp:201) rather than that
 * example's own hardcoded 5-zone cap. The thresholds themselves are the
 * user's own watch settings; Core takes them as plain arguments -- sending
 * SDK::Message::RequestSystemSettings is a later, watch-integration concern.
 ******************************************************************************
 */

#ifndef INTERVALS_HR_ZONES_HPP
#define INTERVALS_HR_ZONES_HPP

#include <cstdint>

namespace Intervals
{

/// The largest threshold count HrZones will read past (skMaxHearRateTh).
constexpr uint8_t kMaxHrThresholds = 7;

/// Zone 0 if count == 0 or hr <= 0. Otherwise the zone whose threshold hr
/// exceeds, scanning low to high (hr > threshold[i] -> at least zone i+1),
/// same comparison as getHrZone. `count` above kMaxHrThresholds is clamped.
uint8_t zoneOf(float hr, const uint8_t* thresholds, uint8_t count);

} // namespace Intervals

#endif // INTERVALS_HR_ZONES_HPP
