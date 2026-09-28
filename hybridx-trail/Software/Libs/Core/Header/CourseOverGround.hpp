/**
 ******************************************************************************
 * @file    CourseOverGround.hpp
 * @brief   The runner's direction of travel, from successive GPS fixes.
 *
 * The compass would be the obvious source for a heading-up map, but on Jon's
 * watch it never reported a calibration in two minutes outdoors (NOTES,
 * Gate T0). So the heading comes from GPS: the bearing from an anchor fix to
 * the current one, once the runner has moved at least kMinMoveM from the
 * anchor, which then moves up. Below that distance, GPS wander (a metre or
 * two a second, standing still) would swing the bearing about, so the last
 * heading is held instead.
 *
 * The same heading tells RouteTracker which way along the route the runner
 * is going (out or back on an out-and-back).
 ******************************************************************************
 */

#ifndef TRAIL_COURSE_OVER_GROUND_HPP
#define TRAIL_COURSE_OVER_GROUND_HPP

#include "GeoPoint.hpp"

namespace Trail
{

class CourseOverGround
{
public:
    static constexpr float kMinMoveM = 10.0f;

    void reset()
    {
        mHaveAnchor = false;
        mValid      = false;
        mHeadingDeg = 0.0f;
    }

    /// Feed one fix. Returns valid().
    bool update(const GeoPoint& fix)
    {
        if (!mHaveAnchor) {
            mAnchor     = fix;
            mHaveAnchor = true;
            return mValid;
        }
        if (Geo::distanceM(mAnchor, fix) >= kMinMoveM) {
            mHeadingDeg = Geo::bearingDeg(mAnchor, fix);
            mValid      = true;
            mAnchor     = fix;
        }
        return mValid;
    }

    /// True once the runner has moved kMinMoveM; stays true, holding the last
    /// heading, while they stand still.
    bool  valid() const { return mValid; }
    float headingDeg() const { return mHeadingDeg; }

private:
    GeoPoint mAnchor {};
    bool     mHaveAnchor = false;
    bool     mValid      = false;
    float    mHeadingDeg = 0.0f;
};

} // namespace Trail

#endif // TRAIL_COURSE_OVER_GROUND_HPP
