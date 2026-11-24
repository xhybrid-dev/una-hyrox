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
    mHighAlong    = 0.0f;
    mSinceHigh    = 0;
    mTurnCount    = 0;
    mAdvance      = 0.0f;
    mFixesSince   = 0;
    mCovered      = 0;
}

RouteTracker::Match RouteTracker::matchSegment(const GeoPoint& fix, uint16_t i) const
{
    Match m;
    float t  = 0.0f;
    m.valid  = true;
    m.segment = i;
    m.d      = Geo::projectOntoSegmentM(fix, mPoints[i], mPoints[i + 1], t);
    m.t      = t;
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

bool RouteTracker::far(float along) const
{
    const float gap   = along - mLastAlong;
    const float ahead = kMaxSpeedMps * static_cast<float>(mFixesSince + 1u) + kSlackM;
    return gap < -kSlackM || gap > ahead;
}

void RouteTracker::cover(float from, float to)
{
    const float length = mCount > 0 ? mCumulative[mCount - 1] : 0.0f;
    if (length <= 0.0f) {
        return;
    }
    auto bin = [length](float along) {
        const int b = static_cast<int>(along / length * static_cast<float>(kCoverBins));
        return b < 0 ? 0 : (b >= kCoverBins ? kCoverBins - 1 : b);
    };
    for (int b = bin(from); b <= bin(to); ++b) {
        mCovered |= uint64_t { 1 } << b;
    }
}

float RouteTracker::covered() const
{
    int n = 0;
    for (uint64_t bits = mCovered; bits != 0; bits &= bits - 1) {
        ++n;
    }
    return static_cast<float>(n) / static_cast<float>(kCoverBins);
}

bool RouteTracker::plausible(float d, float along) const
{
    return !mPos.everLocked || d <= kRejoinM || !far(along);
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

float RouteTracker::segmentBearing(uint16_t i) const
{
    return Geo::bearingDeg(mPoints[i], mPoints[i + 1]);
}

RouteTracker::Match RouteTracker::turnaround(const GeoPoint& fix, const Match& m) const
{
    Match       best;
    const float back = mHighAlong - m.along;
    if (back < kTurnBackM) {
        return best;
    }
    // The way back is the opposite way to the leg the runner is matched on.
    const float wantDeg = Geo::wrap360(segmentBearing(m.segment) + 180.0f);
    const float minAlong = mHighAlong + back - kTurnSlackM;
    const float maxAlong = mHighAlong + kWindowAheadM;
    for (uint16_t i = 0; i + 1 < mCount; ++i) {
        if (mCumulative[i + 1] < minAlong || mCumulative[i] > maxAlong) {
            continue;
        }
        const Match c = matchSegment(fix, i);
        if (c.d > kRejoinM || c.along < minAlong || c.along > maxAlong) {
            continue;
        }
        const float turn = Geo::wrap180(segmentBearing(i) - wantDeg);
        if (turn > kTurnMaxDeg || turn < -kTurnMaxDeg) {
            continue;
        }
        if (!best.valid || c.d < best.d) {
            best = c;
        }
    }
    return best;
}

void RouteTracker::accept(const Match& m, bool jump)
{
    bool fromHere = true;   // the furthest point starts again here (first lock, or a jump)
    if (mPos.everLocked) {
        const bool isFar = far(m.along);
        float step = m.along - mLastAlong;
        if (step > 0.0f && mFixesSince <= kCoverMaxMissed && !isFar && !jump) {
            cover(mLastAlong, m.along);   // an ordinary step forwards; a rejoin covers nothing
        }
        step     = step < 0.0f ? 0.0f : (step > kMaxAdvanceM ? kMaxAdvanceM : step);
        mAdvance = jump ? 0.0f : 0.7f * mAdvance + 0.3f * step;   // a jump itself says nothing about pace
        fromHere = jump || isFar;
    }
    mPos.jumped     = fromHere && mPos.everLocked;
    // The furthest point; after kTurnMaxFixes without a new one (a rest), it
    // starts again where the runner is, so a turn is judged from there.
    if (fromHere || m.along > mHighAlong || mSinceHigh >= kTurnMaxFixes) {
        mHighAlong = m.along;
        mSinceHigh = 0;
    } else if (mSinceHigh < 0xFFFFu) {
        ++mSinceHigh;
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
    if (mPos.remainingM <= kFinishM && mPos.alongM >= kFinishShare * mLengthM && m.d <= kFinishNearM &&
        covered() >= kFinishCredit) {
        mPos.finished = true;
    }
}

const RouteTracker::Position& RouteTracker::update(const GeoPoint& fix)
{
    mPos.jumped  = false;
    mPos.wayBack = false;
    if (mCount == 0) {
        return mPos;
    }
    if (mCount == 1) {
        mPos.offRouteM = Geo::distanceM(fix, mPoints[0]);
        mPos.nearest   = mPoints[0];
        mPos.onRoute   = mPos.offRouteM <= kAcquireM;
        if (mPos.onRoute) {
            Match m;
            m.valid = true;
            accept(m, false);
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
    mPos.nearest   = Geo::lerp(mPoints[global.segment], mPoints[global.segment + 1], global.t);

    if (!mPos.everLocked) {
        if (mPos.onRoute) {
            const Match m = pick(fix, false);
            if (m.valid) {
                accept(m, false);
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
        // Fallen behind the furthest point: if the route comes back past the
        // runner the other way, they turned where it turns (an out-and-back).
        const Match back = turnaround(fix, inWin);
        mTurnCount       = back.valid ? static_cast<uint8_t>(mTurnCount + 1) : 0;
        if (back.valid && mTurnCount >= kTurnConfirm) {
            mTurnCount = 0;
            accept(back, true);
        } else {
            accept(inWin, false);
            mPos.wayBack = back.valid;
        }
    } else if (wide.valid) {
        mTurnCount = 0;
        accept(wide, true);
    } else {
        mTurnCount = 0;
        if (mFixesSince < 0xFFFFu) {
            ++mFixesSince;
        }
    }
    // Otherwise off the route: progress holds where it was.
    return mPos;
}

} // namespace Trail
