/**
 ******************************************************************************
 * @file    RouteMath.hpp
 * @brief   Where a position is relative to a route: the off-course distance.
 *
 * The heart of breadcrumb navigation: how far is the runner from the line?
 * Every segment is checked (a route of 2,000 points is 2,000 cheap flat-earth
 * calculations, well under a millisecond), so a route that doubles back on
 * itself or crosses itself is handled without special cases. The segment
 * index lets the app later prefer the segment near the last one it matched,
 * so an out-and-back route doesn't jump to the wrong leg.
 ******************************************************************************
 */

#ifndef TRAIL_ROUTE_MATH_HPP
#define TRAIL_ROUTE_MATH_HPP

#include <cstdint>

#include "GeoPoint.hpp"

namespace Trail
{
namespace RouteMath
{

struct Nearest {
    bool     valid     = false;   ///< false for an empty route
    float    distanceM = 0.0f;    ///< metres from the position to the route
    uint16_t segment   = 0;       ///< the nearest segment starts at points[segment]
};

/// The nearest point of the polyline @p points[0..count) to @p p. A one-point
/// route is measured to that point.
Nearest nearest(const GeoPoint* points, uint16_t count, const GeoPoint& p);

} // namespace RouteMath
} // namespace Trail

#endif // TRAIL_ROUTE_MATH_HPP
