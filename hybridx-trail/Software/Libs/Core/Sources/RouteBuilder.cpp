/**
 ******************************************************************************
 * @file    RouteBuilder.cpp
 * @brief   Thinning a GPX stream into a fixed array (see the header).
 ******************************************************************************
 */

#include "RouteBuilder.hpp"

namespace Trail
{

RouteBuilder::RouteBuilder(GeoPoint* points, uint16_t capacity)
    : mPoints(points)
    , mCapacity(capacity < 2 ? 2 : capacity)
{
}

void RouteBuilder::reset()
{
    mCount     = 0;
    mSpacingM  = kStartSpacingM;
    mStarted   = false;
    mLastKept  = false;
    mRaw       = 0;
    mIgnored   = 0;
    mLengthM   = 0.0f;
    mEleSeen   = false;
    mEleRefCm  = 0;
    mAscentCm  = 0;
    mDescentCm = 0;
}

void RouteBuilder::point(const GeoPoint& p, bool hasEle, int32_t eleCm, PointKind kind)
{
    if (!mStarted) {
        mStarted = true;
        mKind    = kind;
    } else if (kind != mKind) {
        ++mIgnored;
        return;
    }

    if (mRaw > 0) {
        mLengthM += Geo::distanceM(mLast, p);
    }
    ++mRaw;

    if (hasEle) {
        if (!mEleSeen) {
            mEleSeen  = true;
            mEleRefCm = eleCm;
        } else if (eleCm - mEleRefCm >= kEleBandCm) {
            mAscentCm += eleCm - mEleRefCm;
            mEleRefCm = eleCm;
        } else if (mEleRefCm - eleCm >= kEleBandCm) {
            mDescentCm += mEleRefCm - eleCm;
            mEleRefCm = eleCm;
        }
    }

    mLast     = p;
    mLastKept = false;
    if (mCount == 0 || Geo::distanceM(mPoints[mCount - 1], p) >= static_cast<float>(mSpacingM)) {
        keep(p);
    }
}

void RouteBuilder::keep(const GeoPoint& p)
{
    if (mCount == mCapacity) {
        rethin();
        // Still too close to the (new) last kept point at the wider spacing:
        // it'll be picked up later, or by finish() if it's the last.
        if (Geo::distanceM(mPoints[mCount - 1], p) < static_cast<float>(mSpacingM)) {
            return;
        }
    }
    mPoints[mCount++] = p;
    mLastKept         = true;
}

void RouteBuilder::rethin()
{
    // Double until something is freed: a pathological file (every point
    // exactly one spacing apart) must still make room.
    do {
        if (mSpacingM >= 32768) {
            // Nothing sensible left to do: drop every other point.
            uint16_t out = 1;
            for (uint16_t i = 2; i < mCount; i += 2) {
                mPoints[out++] = mPoints[i];
            }
            mCount = out;
            return;
        }
        mSpacingM = static_cast<uint16_t>(mSpacingM * 2);
        uint16_t out = 1;
        for (uint16_t i = 1; i < mCount; ++i) {
            if (Geo::distanceM(mPoints[out - 1], mPoints[i]) >= static_cast<float>(mSpacingM)) {
                mPoints[out++] = mPoints[i];
            }
        }
        mCount = out;
    } while (mCount == mCapacity);
}

void RouteBuilder::finish()
{
    if (mRaw > 1 && !mLastKept) {
        if (mCount == mCapacity) {
            mPoints[mCount - 1] = mLast;   // the end matters more than the point before it
        } else {
            mPoints[mCount++] = mLast;
        }
        mLastKept = true;
    }
}

} // namespace Trail
