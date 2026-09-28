/**
 ******************************************************************************
 * @file    HeadingFusion.hpp
 * @brief   Which way the runner faces: GPS while running, compass when slow.
 *
 * Two sources, each good where the other is not:
 *
 *   - GPS direction of travel (CourseOverGround) is exact once you are moving
 *     but says nothing when you stop or turn on the spot;
 *   - the watch's compass answers when you stand at a junction and turn, but
 *     swings with the arm at running pace.
 *
 * So above kMovingMps the GPS heading is used, below kStillMps the compass
 * (with hysteresis between, so it does not flick at the threshold). The
 * compass is filtered (a circular average, so 359 and 1 average to 0, not
 * 180) and corrected by an offset the fusion learns while you run: the
 * difference between GPS and compass heading, averaged slowly. That one
 * number absorbs the magnetic declination (the SDK applies none: "Neither
 * bearing is true north"), so no table for where in the world you are, and
 * how the watch sits on the wrist. Learning only happens at running speed,
 * where the GPS heading is trustworthy.
 *
 * The compass reads the bearing of the watch's 12 o'clock (SDK
 * SensorDataParserMagneticField): pointing the forearm the way you face, as
 * you do reading the watch, is the heading.
 *
 * Pure: the caller feeds numbers; the service owns the sensors. Not thread
 * safe; called from the service's loop.
 ******************************************************************************
 */

#ifndef TRAIL_HEADING_FUSION_HPP
#define TRAIL_HEADING_FUSION_HPP

#include <cstdint>

namespace Trail
{

class HeadingFusion
{
public:
    enum class Source : uint8_t { None, Gps, Compass };

    static constexpr float kMovingMps    = 1.8f;   ///< at least this: moving, GPS heading
    static constexpr float kStillMps     = 1.2f;   ///< below this: slow, compass heading
    static constexpr float kLearnMps     = 2.5f;   ///< learn the offset at least this fast
    static constexpr float kCompassAlpha = 0.35f;  ///< weight of each new compass sample
    static constexpr float kLearnAlpha   = 0.04f;  ///< weight of each new offset sample

    void reset();

    /// One update (about once a second).
    /// @param gpsValid   GPS course over ground is known (held while standing)
    /// @param gpsDeg     its bearing, degrees clockwise from true north
    /// @param speedMps   ground speed
    /// @param compassValid a calibrated, current compass sample was given
    /// @param compassDeg   its bearing (12 o'clock, magnetic north)
    void update(bool gpsValid, float gpsDeg, float speedMps, bool compassValid, float compassDeg);

    bool   valid() const { return mSource != Source::None; }
    float  headingDeg() const { return mHeadingDeg; }
    Source source() const { return mSource; }
    /// What has been learnt to add to the compass to get true north.
    float  offsetDeg() const { return mOffsetDeg; }
    bool   offsetKnown() const { return mOffsetKnown; }

private:
    bool   mMoving      = false;
    bool   mHaveCompass = false;
    float  mCx = 0.0f;   ///< filtered compass, as a unit-ish vector
    float  mCy = 0.0f;
    float  mOffsetDeg   = 0.0f;
    bool   mOffsetKnown = false;
    float  mHeadingDeg  = 0.0f;
    Source mSource      = Source::None;
};

} // namespace Trail

#endif // TRAIL_HEADING_FUSION_HPP
