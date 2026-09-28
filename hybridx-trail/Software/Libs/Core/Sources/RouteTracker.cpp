/**
 ******************************************************************************
 * @file    RouteTracker.cpp
 * @brief   Matching fixes to the route (see the header).
 ******************************************************************************
 */

#include "RouteTracker.hpp"

#include <cmath>

namespace Trail
{

RouteTracker::RouteTracker(const GeoPoint* points, uint16_t count, float* cumulative, float lengthM)
    : mPoints(points)
    , mCount(points != nullptr && cumulative != nullptr ? count : 0)
    , mCumulative(cumulative)
{
    if (mCount == 0) {
        return;
    }
    mCumulative[0] = 0.0f;
    for (uint16_t i = 1; i < mCount; ++i) {
        mCumulative[i] = mCumulative[i - 1] + Geo::distanceM(mPoints[i - 1], mPoints[i]);
    }
    const float thinned = mCumulative[mCount - 1];
    mScale              = (lengthM > 0.0f && thinned > 0.0f) ? lengthM / thinned : 1.0f;
    mLengthM            = thinned * mScale;
    reset();
}

void RouteTracker::reset()
{
    mPos          = Position {};
    mPos.remainingM = mLengthM;
    mLastAlong    = 0.0f;
    mAdvance      = 0.0f;
}

RouteTracker::Match RouteTracker::matchSegment(const GeoPoint& fix, uint16_t i) const
{
    Match m;
    float t  = 0.0f;
    m.valid  = true;
    m.segment = i;
    m.d      = Geo::projectOntoSegmentM(fix, mPoints[i], mPoints[i + 1], t);
    m.along  = mCumulative[i] + t * (mCumulative[i + 1] - mCumulative[i]);
    return m;
}

bool RouteTracker::inWindow(uint16_t i) const
{
    return mCumulative[i + 1] >= mLastAlong - kWindowBackM && mCumulative[i] <= mLastAlong + kWindowAheadM;
}

float RouteTracker::alongCost(float along) const
{
    if (!mPos.everLocked) {
        return along;   // first lock: nearest the start
    }
    const float expected = mLastAlong + mAdvance;
    return along >= expected ? along - expected : 2.0f * (expected - along);
}

RouteTracker::Match RouteTracker::pick(const GeoPoint& fix, bool windowOnly) const
{
    Match best;
    float bestScore = 0.0f;
    for (uint16_t i = 0; i + 1 < mCount; ++i) {
        if (windowOnly && !inWindow(i)) {
            continue;
        }
        const Match m = matchSegment(fix, i);
        if (m.d > kAcquireM) {
            continue;
        }
        const float score = m.d + kAlongWeight * alongCost(m.along);
        if (!best.valid || score < bestScore) {
            best      = m;
            bestScore = score;
        }
    }
    return best;
}

void RouteTracker::accept(const Match& m)
{
    if (mPos.everLocked) {
        float step = m.along - mLastAlong;
        step       = step < 0.0f ? 0.0f : (step > kMaxAdvanceM ? kMaxAdvanceM : step);
        mAdvance   = 0.7f * mAdvance + 0.3f * step;
    }
    mLastAlong      = m.along;
    mPos.everLocked = true;
    mPos.segment    = m.segment;
    mPos.alongM     = m.along * mScale;
    mPos.remainingM = mLengthM - mPos.alongM;
    if (mPos.remainingM < 0.0f) {
        mPos.remainingM = 0.0f;
    }
    if (mPos.remainingM <= kFinishM && mPos.alongM >= kFinishShare * mLengthM) {
        mPos.finished = true;
    }
}

const RouteTracker::Position& RouteTracker::update(const GeoPoint& fix)
{
    if (mCount == 0) {
        return mPos;
    }
    if (mCount == 1) {
        mPos.offRouteM = Geo::distanceM(fix, mPoints[0]);
        mPos.onRoute   = mPos.offRouteM <= kAcquireM;
        if (mPos.onRoute) {
            Match m;
            m.valid = true;
            accept(m);
        }
        return mPos;
    }

    // One pass for the nearest segment anywhere, and the nearest in the window.
    Match global;
    Match window;
    for (uint16_t i = 0; i + 1 < mCount; ++i) {
        const Match m = matchSegment(fix, i);
        if (!global.valid || m.d < global.d) {
            global = m;
        }
        if (mPos.everLocked && inWindow(i) && (!window.valid || m.d < window.d)) {
            window = m;
        }
    }
    mPos.offRouteM = global.d;
    mPos.onRoute   = global.d <= kAcquireM;

    if (!mPos.everLocked) {
        if (mPos.onRoute) {
            accept(pick(fix, false));
        }
        return mPos;
    }

    const bool windowGood = window.valid && window.d <= kAcquireM && global.d + kJumpMarginM >= window.d;
    if (windowGood) {
        accept(pick(fix, true));
    } else if (mPos.onRoute) {
        // Rejoined somewhere else: a shortcut, a detour, or a restart. The
        // jump itself says nothing about the runner's pace.
        accept(pick(fix, false));
        mAdvance = 0.0f;
    }
    // Otherwise off the route: progress holds where it was.
    return mPos;
}

} // namespace Trail
