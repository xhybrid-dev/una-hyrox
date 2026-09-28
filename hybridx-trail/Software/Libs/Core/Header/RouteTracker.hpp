/**
 ******************************************************************************
 * @file    RouteTracker.hpp
 * @brief   Where the runner is along the route: distance done and remaining.
 *
 * Each GPS fix is matched to a point on the route. The obvious match, the
 * nearest point anywhere, goes wrong exactly where trail routes get
 * interesting:
 *
 *   - a figure-of-eight crosses itself: at the crossing both legs are 0 m
 *     away, and "nearest" can pick either;
 *   - an out-and-back runs the same path twice: on the way back the outward
 *     leg is just as near, and "nearest" would put the runner kilometres
 *     behind;
 *   - a loop starts and finishes at the same place: at the start, "nearest"
 *     can match the finish and report the route as done.
 *
 * So the tracker remembers where it last matched and searches a window
 * around that (kWindowBackM behind, kWindowAheadM ahead, along the route).
 * It only leaves the window when the runner is clearly nearer somewhere else
 * (by kJumpMarginM): a shortcut, a detour that rejoins further on, or a start
 * part-way along.
 *
 * Within the search, each candidate is scored on two things: how far the fix
 * is from it, and how far it is along the route from where the runner is
 * expected to be. The expectation is the last match plus the runner's recent
 * progress per fix (a smoothed figure, 0 when standing), and a candidate
 * behind the expectation costs double: runners go forwards. That is what
 * separates the two legs of an out-and-back at the turnaround (the same path,
 * equally near, but only one is ahead), and the two strokes of a
 * figure-of-eight at the crossing (equally near, far apart along the route).
 * At the very first lock the along-route part of the score is the distance
 * from the start instead, so a loop starts at its start, not its finish, and
 * a runner part-way along is still placed where they are.
 *
 * A match is also never taken on distance alone when it is "far": more than
 * kSlackM behind the last match, or further ahead than a runner could have
 * gone since (kMaxSpeedMps a fix). A far match needs the runner firmly on the
 * route there (within kRejoinM). Without that, a runner drifting 50 m away
 * from the route near where it doubles back was matched to the return leg
 * 545 m ahead, and later "finished" 57 m from the finish (NOTES, T2: found in
 * the simulator). A genuine shortcut or rejoin is still followed: the runner
 * is on the route when they rejoin it.
 *
 * GPS direction of travel (CourseOverGround) is not used here: with a few
 * metres of GPS noise a second it swings by tens of degrees at running pace,
 * and a tracker that trusted it put the runner on the wrong leg (NOTES, T1).
 *
 * Distances along the route are measured on the thinned route the watch holds,
 * then scaled to the length measured from every point of the GPX
 * (RouteBuilder::lengthM()), so "remaining" agrees with the length the route
 * list shows.
 *
 * Fixed memory: the caller supplies the route and a float per point for the
 * cumulative distances. Every fix costs one pass over the segments, about
 * 2,000 flat-earth projections for a long route: well under a millisecond.
 ******************************************************************************
 */

#ifndef TRAIL_ROUTE_TRACKER_HPP
#define TRAIL_ROUTE_TRACKER_HPP

#include <cstdint>

#include "GeoPoint.hpp"

namespace Trail
{

class RouteTracker
{
public:
    static constexpr float kAcquireM     = 60.0f;    ///< nearer than this counts as on the route
    static constexpr float kWindowBackM  = 150.0f;   ///< search this far behind the last match
    static constexpr float kWindowAheadM = 600.0f;   ///< and this far ahead (10 min at 1 m/s between fixes)
    static constexpr float kJumpMarginM  = 30.0f;    ///< leave the window only if this much nearer elsewhere
    static constexpr float kAlongWeight  = 0.2f;     ///< score: metres off the route + this x metres along from expected
    static constexpr float kMaxAdvanceM  = 25.0f;    ///< the most the expectation moves per fix (9 m/s)
    static constexpr float kRejoinM      = 20.0f;    ///< a far match needs the runner this near the route
    static constexpr float kMaxSpeedMps  = 7.0f;     ///< faster than any runner: beyond it along the route is "far"
    static constexpr float kSlackM       = 30.0f;    ///< and this much either way is never far
    static constexpr float kFinishM      = 40.0f;    ///< this near the end along the route, ...
    static constexpr float kFinishShare  = 0.9f;     ///< ... this far along, ...
    static constexpr float kFinishNearM  = 30.0f;    ///< ... and this near the route there, is the finish

    struct Position {
        bool     everLocked = false;   ///< matched to the route at least once since reset()
        bool     onRoute    = false;   ///< this fix is within kAcquireM of the route
        bool     finished   = false;   ///< reached the finish; stays set until reset()
        uint16_t segment    = 0;       ///< the matched segment starts at points[segment]
        float    alongM     = 0.0f;    ///< distance from the start, along the route (scaled)
        float    remainingM = 0.0f;    ///< distance to the finish, along the route (scaled)
        float    offRouteM  = 0.0f;    ///< distance from the fix to the nearest part of the route
    };

    /// @param points     the route (at least 1 point), not copied: must outlive the tracker.
    /// @param cumulative caller's storage for @p count floats.
    /// @param lengthM    the route's length from the full GPX; 0 to use the thinned length.
    RouteTracker(const GeoPoint* points, uint16_t count, float* cumulative, float lengthM = 0.0f);

    /// No route yet: bind() one before tracking.
    RouteTracker() : RouteTracker(nullptr, 0, nullptr) {}

    /// Point the tracker at a (new) route, as the constructor does, and reset().
    void bind(const GeoPoint* points, uint16_t count, float* cumulative, float lengthM = 0.0f);

    /// Forget the runner's progress, e.g. when a new run starts.
    void reset();

    /// Match one fix (about one a second). Returns the position, also kept for
    /// position().
    const Position& update(const GeoPoint& fix);

    const Position& position() const { return mPos; }
    float           lengthM() const { return mLengthM; }
    uint16_t        count() const { return mCount; }

    /// Scaled distance from the start to points[i].
    float alongAtM(uint16_t i) const { return i < mCount ? mCumulative[i] * mScale : mLengthM; }

private:
    struct Match {
        bool     valid   = false;
        uint16_t segment = 0;
        float    d       = 0.0f;   ///< metres to the route
        float    along   = 0.0f;   ///< unscaled metres from the start
    };

    Match matchSegment(const GeoPoint& fix, uint16_t i) const;
    bool  inWindow(uint16_t i) const;
    /// The along-route part of the score (see the class comment).
    float alongCost(float along) const;
    /// Whether a match @p d from the fix and @p along the route may be taken.
    bool  plausible(float d, float along) const;
    /// The best-scoring segment within kAcquireM (and the window, if @p windowOnly).
    Match pick(const GeoPoint& fix, bool windowOnly) const;
    void  accept(const Match& m);

    const GeoPoint* mPoints;
    uint16_t        mCount;
    float*          mCumulative;
    float           mScale   = 1.0f;
    float           mLengthM = 0.0f;
    Position        mPos {};
    float           mLastAlong = 0.0f;   ///< unscaled
    float           mAdvance   = 0.0f;   ///< smoothed progress per fix, unscaled
    uint16_t        mFixesSince = 0;     ///< fixes since the last accepted match
};

} // namespace Trail

#endif // TRAIL_ROUTE_TRACKER_HPP
