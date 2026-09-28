/**
 ******************************************************************************
 * @file    ElevationProfile.hpp
 * @brief   The route's climb and descent as a small fixed profile.
 *
 * The route the watch holds has up to 2,000 points; the elevation screen
 * needs a hundred. build() resamples the route's elevation at kBins evenly
 * spaced distances, and the climb so far the same way, so the screen draws
 * a line and the questions "how much climbing is left?" and "what is the
 * next climb?" are answered from 96 numbers, not the route.
 *
 * Elevations are in half metres (an int16 covers +-16 km) and come from the
 * GPX. The climb uses the same 5 m dead band as the route's total ascent
 * (RouteBuilder), so the sum agrees with the figure the route list shows.
 * A route without elevation has an invalid profile; the screen says so.
 *
 * A climb is the next rise of at least kClimbMinM, allowing dips of up to
 * kDipM. If the runner is already climbing, it is measured from where they
 * are. Resolution is the bin width, route length / 95: 100-200 m on a
 * typical route, so a climb starts and ends to about that.
 *
 * Plain data, copyable (the service sends the GUI a pointer to it).
 ******************************************************************************
 */

#ifndef TRAIL_ELEVATION_PROFILE_HPP
#define TRAIL_ELEVATION_PROFILE_HPP

#include <cstdint>

namespace Trail
{

class ElevationProfile
{
public:
    static constexpr uint8_t kBins       = 96;
    static constexpr float   kClimbMinM  = 25.0f;
    static constexpr float   kDipM       = 8.0f;

    struct Climb {
        bool  found      = false;
        float startAheadM = 0.0f;   ///< metres until it begins (0: already on it)
        float riseM       = 0.0f;
        float lengthM     = 0.0f;
    };

    void clear();

    /// @param cumulative distance from the start to each point (unscaled)
    /// @param eleHalfM   elevation at each point, half metres
    /// @param ascentM    climb so far at each point, metres
    /// @param lengthM    the route's length (scaled, as the tracker's)
    void build(const float* cumulative, const int16_t* eleHalfM, const uint16_t* ascentM, uint16_t count,
               bool hasElevation, float lengthM);

    bool  valid() const { return mValid; }
    float lengthM() const { return mLengthM; }
    float minM() const { return static_cast<float>(mMinHalf) * 0.5f; }
    float maxM() const { return static_cast<float>(mMaxHalf) * 0.5f; }
    float totalAscentM() const { return static_cast<float>(mAscent[kBins - 1]); }

    /// Elevation of bin @p i, metres.
    float binM(uint8_t i) const { return static_cast<float>(mEleHalf[i < kBins ? i : kBins - 1]) * 0.5f; }

    float elevationM(float alongM) const;
    float ascentLeftM(float alongM) const;
    Climb nextClimb(float alongM) const;

private:
    float fraction(float alongM) const;   ///< 0..kBins-1, fractional bin
    float binAlongM(int i) const { return mLengthM * static_cast<float>(i) / static_cast<float>(kBins - 1); }

    bool     mValid   = false;
    float    mLengthM = 0.0f;
    int16_t  mMinHalf = 0;
    int16_t  mMaxHalf = 0;
    int16_t  mEleHalf[kBins] {};
    uint16_t mAscent[kBins] {};
};

} // namespace Trail

#endif // TRAIL_ELEVATION_PROFILE_HPP
