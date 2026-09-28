/**
 ******************************************************************************
 * @file    ElevationProfile.cpp
 * @brief   The route's elevation profile (see the header).
 ******************************************************************************
 */

#include "ElevationProfile.hpp"

#include <cmath>

namespace Trail
{

void ElevationProfile::clear()
{
    *this = ElevationProfile {};
}

void ElevationProfile::build(const float* cumulative, const int16_t* eleHalfM, const uint16_t* ascentM,
                             uint16_t count, bool hasElevation, float lengthM)
{
    clear();
    if (!hasElevation || cumulative == nullptr || eleHalfM == nullptr || ascentM == nullptr || count < 2 ||
        lengthM <= 0.0f || cumulative[count - 1] <= 0.0f) {
        return;
    }
    mValid   = true;
    mLengthM = lengthM;
    const float total = cumulative[count - 1];
    uint16_t    seg   = 0;   // bins go forwards: the segment only advances
    for (uint8_t i = 0; i < kBins; ++i) {
        const float s = total * static_cast<float>(i) / static_cast<float>(kBins - 1);
        while (seg + 2 < count && cumulative[seg + 1] < s) {
            ++seg;
        }
        const float span = cumulative[seg + 1] - cumulative[seg];
        float       t    = span > 0.0f ? (s - cumulative[seg]) / span : 0.0f;
        t                = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        const float e    = static_cast<float>(eleHalfM[seg]) + t * static_cast<float>(eleHalfM[seg + 1] - eleHalfM[seg]);
        const float a    = static_cast<float>(ascentM[seg]) + t * static_cast<float>(ascentM[seg + 1] - ascentM[seg]);
        mEleHalf[i]      = static_cast<int16_t>(std::lround(e));
        mAscent[i]       = static_cast<uint16_t>(std::lround(a < 0.0f ? 0.0f : a));
    }
    mMinHalf = mMaxHalf = mEleHalf[0];
    for (uint8_t i = 1; i < kBins; ++i) {
        mMinHalf = mEleHalf[i] < mMinHalf ? mEleHalf[i] : mMinHalf;
        mMaxHalf = mEleHalf[i] > mMaxHalf ? mEleHalf[i] : mMaxHalf;
    }
}

float ElevationProfile::fraction(float alongM) const
{
    if (!mValid || mLengthM <= 0.0f) {
        return 0.0f;
    }
    float f = alongM / mLengthM * static_cast<float>(kBins - 1);
    return f < 0.0f ? 0.0f : (f > static_cast<float>(kBins - 1) ? static_cast<float>(kBins - 1) : f);
}

float ElevationProfile::elevationM(float alongM) const
{
    if (!mValid) {
        return 0.0f;
    }
    const float f  = fraction(alongM);
    const int   i  = static_cast<int>(f) >= kBins - 1 ? kBins - 2 : static_cast<int>(f);
    const float t  = f - static_cast<float>(i);
    const float e  = static_cast<float>(mEleHalf[i]) + t * static_cast<float>(mEleHalf[i + 1] - mEleHalf[i]);
    return e * 0.5f;
}

float ElevationProfile::ascentLeftM(float alongM) const
{
    if (!mValid) {
        return 0.0f;
    }
    const float f    = fraction(alongM);
    const int   i    = static_cast<int>(f) >= kBins - 1 ? kBins - 2 : static_cast<int>(f);
    const float t    = f - static_cast<float>(i);
    const float done = static_cast<float>(mAscent[i]) + t * static_cast<float>(mAscent[i + 1] - mAscent[i]);
    const float left = totalAscentM() - done;
    return left < 0.0f ? 0.0f : left;
}

ElevationProfile::Climb ElevationProfile::nextClimb(float alongM) const
{
    Climb c;
    if (!mValid) {
        return c;
    }
    const int i0 = static_cast<int>(fraction(alongM));
    float valley = binM(static_cast<uint8_t>(i0));
    float peak   = valley;
    int   vIdx   = i0;
    int   pIdx   = i0;
    bool  climbing = false;
    for (int i = i0 + 1; i < kBins; ++i) {
        const float e = binM(static_cast<uint8_t>(i));
        if (!climbing) {
            if (e <= valley) {   // a plateau at the valley: the climb starts at its far end
                valley = peak = e;
                vIdx = pIdx = i;
            } else {
                if (e > peak) {
                    peak = e;
                    pIdx = i;
                }
                climbing = peak - valley >= kClimbMinM;
            }
        } else if (e > peak) {
            peak = e;
            pIdx = i;
        } else if (peak - e > kDipM) {
            break;
        }
    }
    if (!climbing) {
        return c;
    }
    c.found       = true;
    const float start = binAlongM(vIdx) - alongM;
    c.startAheadM = start > 0.0f ? start : 0.0f;
    c.riseM       = peak - valley;
    c.lengthM     = binAlongM(pIdx) - binAlongM(vIdx);
    return c;
}

} // namespace Trail
