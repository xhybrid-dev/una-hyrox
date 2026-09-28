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
    : mPoints(nullptr)
    , mCount(0)
    , mCumulative(nullptr)
{
    bind(points, count, cumulative, lengthM);
}

void RouteTracker::bind(const GeoPoint* points, uint16_t count, float* cumulative, float lengthM)
{
    mPoints     = points;
    mCount      = points != nullptr && cumulative != nullptr ? count : 0;
    mCumulative = cumulative;
    mScale      = 1.0f;
    mLengthM    = 0.0f;
    if (mCount == 0) {
        reset();
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
    mFixesSince   = 0;
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

bool RouteTracker::plausible(float d, float along) const
{
    if (!mPos.everLocked || d <= kRejoinM) {
        return true;
    }
    const float gap = along - mLastAlong;
    const float ahead = kMaxSpeedMps * static_cast<float>(mFixesSince + 1u) + kSlackM;
    return gap >= -kSlackM && gap <= ahead;
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
        if (m.d > kAcquireM || !plausible(m.d, m.along)) {
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
    mFixesSince     = 0;
    mPos.everLocked = true;
    mPos.segment    = m.segment;
    mPos.alongM     = m.along * mScale;
    mPos.remainingM = mLengthM - mPos.alongM;
    if (mPos.remainingM < 0.0f) {
        mPos.remainingM = 0.0f;
    }
    if (mPos.remainingM <= kFinishM && mPos.alongM >= kFinishShare * mLengthM && m.d <= kFinishNearM) {
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
            const Match m = pick(fix, false);
            if (m.valid) {
                accept(m);
            }
        }
        return mPos;
    }

    // In the window if it has a good match; elsewhere only if clearly nearer
    // there (a shortcut, a detour, a restart), and plausible either way.
    const Match inWin    = pick(fix, true);
    const bool  lookWide = !inWin.valid || (window.valid && global.d + kJumpMarginM < window.d);
    const Match wide     = lookWide ? pick(fix, false) : Match {};
    if (inWin.valid && !(wide.valid && wide.d + kJumpMarginM < inWin.d)) {
        accept(inWin);
    } else if (wide.valid) {
        accept(wide);
        mAdvance = 0.0f;   // the jump itself says nothing about the runner's pace
    } else if (mFixesSince < 0xFFFFu) {
        ++mFixesSince;
    }
    // Otherwise off the route: progress holds where it was.
    return mPos;
}

} // namespace Trail
