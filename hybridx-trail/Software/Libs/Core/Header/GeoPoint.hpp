/**
 ******************************************************************************
 * @file    GeoPoint.hpp
 * @brief   A position on the ground, and the distances HybridX Trail needs.
 *
 * Coordinates are stored as integer degrees x 10^7 (the same fixed point FIT
 * and most GNSS chips use): 8 bytes a point, exact to about 1 cm, and
 * differences between two nearby points are taken in integers before any
 * float is involved. That matters because a float holds only ~7 significant
 * digits: at latitude 54 its step is about 4e-6 degrees (0.4 m), near
 * longitude 180 about 1.5e-5 degrees (up to 1.7 m), and subtracting two such
 * floats a few metres apart throws much of the difference away.
 * The SDK's own sensor layer and TrackMapBuilder use float degrees
 * (una-sdk Docs/app-config-fields.md, "float"); converting once at the edge
 * and doing the geometry here keeps route maths exact.
 *
 * Distances use a local flat-earth (equirectangular) projection: within a few
 * kilometres it is within 0.1 % of the great-circle distance, far inside GPS
 * error, and it needs one cosine per call rather than a haversine's trig.
 * Longer spans (a whole route's length) are summed from short steps, so the
 * error never accumulates across a long leg.
 ******************************************************************************
 */

#ifndef TRAIL_GEO_POINT_HPP
#define TRAIL_GEO_POINT_HPP

#include <cstdint>

namespace Trail
{

struct GeoPoint {
    int32_t latE7 = 0;   ///< latitude, degrees x 10^7, north positive
    int32_t lonE7 = 0;   ///< longitude, degrees x 10^7, east positive
};

static_assert(sizeof(GeoPoint) == 8, "GeoPoint is stored in fixed arrays");

namespace Geo
{

constexpr int32_t kMaxLatE7 = 900000000;
constexpr int32_t kMaxLonE7 = 1800000000;

/// Mean Earth radius, metres (IUGG), as the SDK's TrackMapBuilder uses.
constexpr float kEarthRadiusM = 6371000.0f;

inline bool valid(const GeoPoint& p)
{
    return p.latE7 >= -kMaxLatE7 && p.latE7 <= kMaxLatE7 && p.lonE7 >= -kMaxLonE7 && p.lonE7 <= kMaxLonE7;
}

/// From the SDK's float degrees (GPS_LOCATION). Rounds to the nearest 1e-7.
GeoPoint fromDegrees(float lat, float lon);

/// Metres between two points; accurate for points up to a few km apart.
float distanceM(const GeoPoint& a, const GeoPoint& b);

/// Metres from @p p to the nearest point of segment a-b (a == b is allowed).
float distanceToSegmentM(const GeoPoint& p, const GeoPoint& a, const GeoPoint& b);

} // namespace Geo

} // namespace Trail

#endif // TRAIL_GEO_POINT_HPP
