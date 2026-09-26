/**
 ******************************************************************************
 * @file    RouteBuilder.hpp
 * @brief   Turns a stream of GPX points into a route that fits a fixed array.
 *
 * A GPX from a planner or a recorded run can hold a point every metre or
 * every second: tens of thousands for a long trail. The watch keeps a fixed
 * number (the caller's array), and keeps them evenly spread along the route:
 *
 *   - a point is kept once it is at least the current spacing from the last
 *     one kept (10 m to start with, which is finer than GPS can place you);
 *   - if the array fills, the spacing doubles and the points already kept are
 *     thinned again in place, so a file of any length fits, just coarser;
 *   - the first and last points of the file are always kept.
 *
 * The route's length, ascent and descent are summed from every point in the
 * file, not the kept ones, so thinning never changes them. Ascent uses a 5 m
 * dead band, as GPX elevations (especially from recorded tracks) wander by a
 * metre or two point to point, and summing that noise inflates the climb.
 *
 * If a file holds both a track and a route, the kind that arrives first is
 * used and the other is counted in ignoredPoints: joining them would draw a
 * line from one's end to the other's start.
 ******************************************************************************
 */

#ifndef TRAIL_ROUTE_BUILDER_HPP
#define TRAIL_ROUTE_BUILDER_HPP

#include <cstdint>

#include "GeoPoint.hpp"
#include "GpxReader.hpp"

namespace Trail
{

class RouteBuilder : public GpxSink
{
public:
    static constexpr uint16_t kStartSpacingM = 10;
    static constexpr int32_t  kEleBandCm     = 500;

    /// @param points caller's storage, @p capacity >= 2.
    RouteBuilder(GeoPoint* points, uint16_t capacity);

    void reset();

    // GpxSink
    void point(const GeoPoint& p, bool hasEle, int32_t eleCm, PointKind kind) override;

    /// Call once the file is read: keeps the last point if it wasn't.
    void finish();

    const GeoPoint* points() const { return mPoints; }
    uint16_t        count() const { return mCount; }
    uint16_t        spacingM() const { return mSpacingM; }
    uint32_t        rawPoints() const { return mRaw; }
    uint32_t        ignoredPoints() const { return mIgnored; }
    uint32_t        lengthM() const { return static_cast<uint32_t>(mLengthM + 0.5f); }
    bool            hasElevation() const { return mEleSeen; }
    uint32_t        ascentM() const { return static_cast<uint32_t>(mAscentCm / 100); }
    uint32_t        descentM() const { return static_cast<uint32_t>(mDescentCm / 100); }

private:
    void keep(const GeoPoint& p);
    void rethin();

    GeoPoint* mPoints;
    uint16_t  mCapacity;
    uint16_t  mCount    = 0;
    uint16_t  mSpacingM = kStartSpacingM;

    bool      mStarted  = false;
    PointKind mKind     = PointKind::Track;
    GeoPoint  mLast {};           ///< the last point seen, kept or not
    bool      mLastKept = false;
    uint32_t  mRaw      = 0;
    uint32_t  mIgnored  = 0;
    float     mLengthM  = 0.0f;

    bool    mEleSeen   = false;
    int32_t mEleRefCm  = 0;
    int64_t mAscentCm  = 0;
    int64_t mDescentCm = 0;
};

} // namespace Trail

#endif // TRAIL_ROUTE_BUILDER_HPP
