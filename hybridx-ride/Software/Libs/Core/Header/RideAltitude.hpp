/**
 ******************************************************************************
 * @file    RideAltitude.hpp
 * @brief   Which barometer readings are fit to use. Pure C++, header-only.
 *
 * The altitude is worked out from the station pressure and the sea-level
 * pressure the kernel sends with it (SDK Pressure::getAltitude). If either is
 * missing the SDK returns NaN, and one NaN fed to the smoothing filter or the
 * climb counter would spoil them for the rest of the ride, so a reading is used
 * only when both are real numbers. The SDK's own rule for the pressures is
 * "greater than zero"; the altitude limits are the physical range with a wide
 * margin (the Dead Sea shore to above Everest's summit), not a sensor spec.
 ******************************************************************************
 */

#ifndef RIDE_RIDE_ALTITUDE_HPP
#define RIDE_RIDE_ALTITUDE_HPP

#include <cmath>

namespace Ride {

constexpr float kMinAltitudeM = -1000.0f;
constexpr float kMaxAltitudeM = 10000.0f;

/// A pressure in Pa the SDK can use: a finite number above zero (NaN fails).
inline bool usablePressurePa(float pa)
{
    return std::isfinite(pa) && pa > 0.0f;
}

/// An altitude in metres worth keeping: finite and within the physical range.
inline bool usableAltitudeM(float m)
{
    return std::isfinite(m) && m >= kMinAltitudeM && m <= kMaxAltitudeM;
}

} // namespace Ride

#endif // RIDE_RIDE_ALTITUDE_HPP
