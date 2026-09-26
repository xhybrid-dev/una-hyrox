/**
 ******************************************************************************
 * @file    RouteMath.cpp
 * @brief   Nearest point on a route (see the header).
 ******************************************************************************
 */

#include "RouteMath.hpp"

namespace Trail
{
namespace RouteMath
{

Nearest nearest(const GeoPoint* points, uint16_t count, const GeoPoint& p)
{
    Nearest best;
    if (points == nullptr || count == 0) {
        return best;
    }
    if (count == 1) {
        best.valid     = true;
        best.distanceM = Geo::distanceM(p, points[0]);
        return best;
    }
    for (uint16_t i = 0; i + 1 < count; ++i) {
        const float d = Geo::distanceToSegmentM(p, points[i], points[i + 1]);
        if (!best.valid || d < best.distanceM) {
            best.valid     = true;
            best.distanceM = d;
            best.segment   = i;
        }
    }
    return best;
}

} // namespace RouteMath
} // namespace Trail
