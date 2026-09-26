/**
 ******************************************************************************
 * @file    GeoPoint.cpp
 * @brief   Distances between GeoPoints (see the header).
 ******************************************************************************
 */

#include "GeoPoint.hpp"

#include <cmath>

namespace Trail
{
namespace Geo
{

namespace
{
constexpr float kPi           = 3.14159265358979f;
constexpr float kRadPerE7     = kPi / 180.0f / 1.0e7f;
constexpr float kMetresPerE7  = kEarthRadiusM * kRadPerE7;   ///< along a meridian

/// Longitude difference in E7, the short way round the antimeridian. Taken in
/// 64 bits: two valid longitudes can differ by up to 3.6e9, past int32.
int64_t lonDelta(int32_t from, int32_t to)
{
    int64_t d = static_cast<int64_t>(to) - from;
    if (d > kMaxLonE7) {
        d -= 2LL * kMaxLonE7;
    } else if (d < -static_cast<int64_t>(kMaxLonE7)) {
        d += 2LL * kMaxLonE7;
    }
    return d;
}

/// Metres per E7 of longitude at the given latitude.
float lonScale(int32_t latE7)
{
    return kMetresPerE7 * std::cos(static_cast<float>(latE7) * kRadPerE7);
}
} // namespace

GeoPoint fromDegrees(float lat, float lon)
{
    GeoPoint p;
    p.latE7 = static_cast<int32_t>(std::lround(static_cast<double>(lat) * 1.0e7));
    p.lonE7 = static_cast<int32_t>(std::lround(static_cast<double>(lon) * 1.0e7));
    return p;
}

float distanceM(const GeoPoint& a, const GeoPoint& b)
{
    const int32_t midLat = static_cast<int32_t>((static_cast<int64_t>(a.latE7) + b.latE7) / 2);
    const float   dy     = static_cast<float>(static_cast<int64_t>(b.latE7) - a.latE7) * kMetresPerE7;
    const float   dx     = static_cast<float>(lonDelta(a.lonE7, b.lonE7)) * lonScale(midLat);
    return std::sqrt(dx * dx + dy * dy);
}

float distanceToSegmentM(const GeoPoint& p, const GeoPoint& a, const GeoPoint& b)
{
    // Flatten around p: p is the origin, a and b are metres east/north of it.
    const float sx = lonScale(p.latE7);
    const float ax = static_cast<float>(lonDelta(p.lonE7, a.lonE7)) * sx;
    const float ay = static_cast<float>(static_cast<int64_t>(a.latE7) - p.latE7) * kMetresPerE7;
    const float bx = static_cast<float>(lonDelta(p.lonE7, b.lonE7)) * sx;
    const float by = static_cast<float>(static_cast<int64_t>(b.latE7) - p.latE7) * kMetresPerE7;

    const float vx  = bx - ax;
    const float vy  = by - ay;
    const float len = vx * vx + vy * vy;
    float       t   = 0.0f;
    if (len > 0.0f) {
        t = -(ax * vx + ay * vy) / len;   // projection of the origin onto a-b
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    }
    const float cx = ax + t * vx;
    const float cy = ay + t * vy;
    return std::sqrt(cx * cx + cy * cy);
}

} // namespace Geo
} // namespace Trail
