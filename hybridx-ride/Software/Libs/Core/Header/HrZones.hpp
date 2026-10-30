/**
 ******************************************************************************
 * @file    HrZones.hpp
 * @brief   Heart-rate zones on the watch's own thresholds, and calories from
 *          them, both as the SDK's Workout example works them out
 *          (Examples/Apps/Ride, Service::getHrZone and
 *          updateHrDerivedMetrics). Pure C++, header-only, integer only.
 *
 * Zones. The kernel gives up to six thresholds (RequestSystemSettings,
 * heartRateTh); its defaults are 95, 114, 133, 152, 171, 190. A heart rate
 * above threshold i is in zone i + 1, capped at 5; at or below the first, or
 * none, is zone 0 ("below zone 1").
 *
 * Calories. Each active second burns MET x weight / 3600 kcal, with the
 * example's MET for the zone: 1.0 (resting) in zone 0, then 2.5, 4.5, 7, 10 and
 * 12.5 for zones 1 to 5. The resting part (1.0 MET every active second) is
 * also kept on its own: the FIT file's metabolic and resting calories. Here
 * MET is in tenths, the weight in grams and the time in ms, so a total is
 * sum(ms x MET10) x grams / 3.6e10 kcal, worked in 64 bits.
 ******************************************************************************
 */

#ifndef RIDE_HR_ZONES_HPP
#define RIDE_HR_ZONES_HPP

#include <cstdint>

namespace Ride {

constexpr uint8_t kZoneCount = 6;       ///< Zone 0 (below zone 1) and zones 1 to 5
constexpr uint8_t kThresholdCount = 6;  ///< As the kernel sends them

/// The example's MET for each zone, in tenths.
constexpr uint16_t kZoneMet10[kZoneCount] = {10, 25, 45, 70, 100, 125};
constexpr uint16_t kRestingMet10 = 10;

/// The example's fallback weight, when the profile has none: 75 kg.
constexpr uint32_t kDefaultWeightG = 75000;

struct HrZones {
    uint8_t th[kThresholdCount] = {95, 114, 133, 152, 171, 190};
    uint8_t count = kThresholdCount;

    /// The zone of @p hr bpm: 0 below zone 1 (or none), else 1 to 5.
    uint8_t zoneOf(uint16_t hr) const
    {
        if (hr == 0) {
            return 0;
        }
        uint8_t zone = 0;
        const uint8_t n = count < kThresholdCount ? count : kThresholdCount;
        for (uint8_t i = 0; i < n; ++i) {
            if (hr > th[i]) {
                zone = static_cast<uint8_t>(i + 1u);
            }
        }
        return zone > 5 ? 5 : zone;
    }
};

/// Calories in tenths of a kcal for time in each zone (ms) at @p weightG grams, rounded.
inline uint32_t caloriesX10(const uint32_t (&zoneMs)[kZoneCount], uint32_t weightG)
{
    uint64_t metMs = 0;   // at most 6 x 4.3e9 x 125: fits easily
    for (uint8_t z = 0; z < kZoneCount; ++z) {
        metMs += static_cast<uint64_t>(zoneMs[z]) * kZoneMet10[z];
    }
    // kcal x 10 = metMs / 10 x grams / 1000 / 3.6e6 x 10 = metMs x grams / 3.6e9
    const uint64_t num = metMs * weightG;   // at most 3.2e12 x 3e5: under 2^64 for any real weight
    const uint64_t v   = (num + 1800000000ull) / 3600000000ull;
    return v > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(v);
}

/// The resting part of @p activeMs at @p weightG, in tenths of a kcal.
inline uint32_t restingCaloriesX10(uint32_t activeMs, uint32_t weightG)
{
    const uint64_t num = static_cast<uint64_t>(activeMs) * kRestingMet10 * weightG;
    const uint64_t v   = (num + 1800000000ull) / 3600000000ull;
    return v > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(v);
}

} // namespace Ride

#endif // RIDE_HR_ZONES_HPP
