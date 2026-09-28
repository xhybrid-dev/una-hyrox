/**
 ******************************************************************************
 * @file    HeadingFusion.cpp
 * @brief   GPS and compass heading (see the header).
 ******************************************************************************
 */

#include "HeadingFusion.hpp"

#include <cmath>

#include "GeoPoint.hpp"

namespace Trail
{

namespace
{
constexpr float kDegToRad = 3.14159265358979f / 180.0f;
}

void HeadingFusion::reset()
{
    // The learnt offset is a property of the watch and the place, not of one
    // run: it survives a reset.
    mMoving      = false;
    mHaveCompass = false;
    mCx = mCy   = 0.0f;
    mHeadingDeg = 0.0f;
    mSource     = Source::None;
}

void HeadingFusion::update(bool gpsValid, float gpsDeg, float speedMps, bool compassValid, float compassDeg)
{
    if (compassValid) {
        const float x = std::sin(compassDeg * kDegToRad);
        const float y = std::cos(compassDeg * kDegToRad);
        if (!mHaveCompass) {
            mCx = x;
            mCy = y;
        } else {
            mCx += kCompassAlpha * (x - mCx);
            mCy += kCompassAlpha * (y - mCy);
        }
        mHaveCompass = true;
    }
    const bool  compassOk  = mHaveCompass && compassValid && (mCx * mCx + mCy * mCy) > 0.01f;
    const float compassNow = compassOk ? Geo::wrap360(std::atan2(mCx, mCy) / kDegToRad) : 0.0f;

    if (mMoving) {
        mMoving = speedMps >= kStillMps;
    } else {
        mMoving = speedMps >= kMovingMps;
    }

    // Learn compass -> true north while running, from the GPS heading.
    if (gpsValid && compassOk && speedMps >= kLearnMps) {
        const float diff = Geo::wrap180(gpsDeg - compassNow);
        if (!mOffsetKnown) {
            mOffsetDeg   = diff;
            mOffsetKnown = true;
        } else {
            mOffsetDeg = Geo::wrap180(mOffsetDeg + kLearnAlpha * Geo::wrap180(diff - mOffsetDeg));
        }
    }

    if (mMoving && gpsValid) {
        mHeadingDeg = gpsDeg;
        mSource     = Source::Gps;
    } else if (compassOk) {
        mHeadingDeg = Geo::wrap360(compassNow + mOffsetDeg);
        mSource     = Source::Compass;
    } else if (gpsValid) {
        mHeadingDeg = gpsDeg;   // held while standing, no compass
        mSource     = Source::Gps;
    }
    // else: keep whatever we had (or None)
}

} // namespace Trail
