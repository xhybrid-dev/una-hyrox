/**
 ******************************************************************************
 * @file    TurnFinder.hpp
 * @brief   The next turn on the route, for "Left in 50 m".
 *
 * A turn is where the route's direction changes by at least kMinAngleDeg
 * within a short stretch. Direction is measured over chords of kChordM
 * metres either side of a point (30 m before it to it, and it to 30 m
 * after), so the GPS wobble of a recorded track and the small zig-zags of a
 * path do not count, while a corner, a junction turn or a hairpin does.
 *
 * The route is sampled every kGridM metres along its length, at positions
 * fixed by the route, not by the runner. A turn is the sample of largest
 * angle in a run of samples over the threshold. Because the grid does not
 * move with the runner, the same turn is found at the same place every
 * second: the cue does not flicker, and it cannot be found twice.
 *
 * Nothing is stored: each call looks ahead over the route the runner is on
 * (about 50 samples for 400 m), a few hundred microseconds on the watch.
 *
 * The route the watch holds is thinned (10 m and up between points), so a
 * corner may be cut by up to a point spacing: the turn is found within about
 * that distance of the corner, in the right direction. Routes with wide
 * spacing (very long routes) round their corners, and gentle bends may be
 * missed; sharp turns are still found.
 ******************************************************************************
 */

#ifndef TRAIL_TURN_FINDER_HPP
#define TRAIL_TURN_FINDER_HPP

#include <cstdint>

#include "GeoPoint.hpp"

namespace Trail
{

struct Turn {
    bool    valid     = false;
    float   alongM    = 0.0f;   ///< where along the route (the caller's own scale)
    int16_t angleDeg  = 0;      ///< how far the route turns: positive right, negative left
};

class TurnFinder
{
public:
    static constexpr float kChordM      = 30.0f;
    static constexpr float kGridM       = 10.0f;
    static constexpr float kMinAngleDeg = 45.0f;   ///< less than this is a bend, not a turn
    static constexpr float kSharpDeg    = 110.0f;
    static constexpr float kUTurnDeg    = 145.0f;
    static constexpr float kEndClearM   = 30.0f;   ///< no turn this near the start or the finish

    /// The first turn whose peak is at or beyond @p fromM and before
    /// @p toM along the route. @p cumulative is the distance from the start to
    /// each point (points[0] at 0), in the same unit as @p fromM.
    static Turn next(const GeoPoint* points, uint16_t count, const float* cumulative, float fromM, float toM);

    /// The point @p alongM along the route (clamped to its ends).
    static GeoPoint pointAt(const GeoPoint* points, uint16_t count, const float* cumulative, float alongM);

    /// "Left", "Right", "Sharp left", "Sharp right" or "U-turn".
    static const char* name(int16_t angleDeg);
    static bool        isSharp(int16_t angleDeg);
};

} // namespace Trail

#endif // TRAIL_TURN_FINDER_HPP
