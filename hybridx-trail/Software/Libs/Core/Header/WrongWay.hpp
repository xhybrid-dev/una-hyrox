/**
 ******************************************************************************
 * @file    WrongWay.hpp
 * @brief   When to buzz because the runner is on the route but running it
 *          backwards: the wrong-way alert, a small state machine beside
 *          OffCourse (T4, Jon's field test).
 *
 * The evidence is the tracker's progress along the route, not GPS heading
 * (which T1 found swings by tens of degrees over 10 m of running). The rules,
 * in the order they apply to each fix:
 *
 *   1. Before the runner first reaches the route, nothing. After the finish,
 *      nothing ever again (walking back to the car).
 *   2. Off course (OffCourse says so): that alert is the news. Any wrong-way
 *      alert is dropped quietly, and the count starts again from here. A
 *      tracker jump (a rejoin, a shortcut, a reversed loop's start) counts
 *      from the new place: towards wrong way, or, if already wrong, towards
 *      right way (so a loop run backwards is not warned afresh each lap).
 *   3. A fix whose precision is worse than maxPrecisionM, or more than nearM
 *      from the line, changes nothing and breaks any count in progress.
 *   4. The anchor is the furthest point along the route since the count
 *      started. backM behind it, held for confirmMs: WentWrong. Two guards:
 *      the tracker saying the route comes back past here the other way
 *      (Position::wayBack: an out-and-back's turnaround, which the tracker
 *      is about to take) never counts; and the anchor starts again where the
 *      runner is after maxFixes counted fixes without a new furthest point,
 *      so GPS drifting at a feed station (slower than 0.5 m/s) never adds up.
 *   5. While wrong: the lowest point along the route is kept, and rightM
 *      forward of it, held for confirmMs, is RightWay. Every remindMs while
 *      still wrong, StillWrong.
 *
 * Time is the kernel's millisecond clock, only ever subtracted unsigned.
 * Distances are the tracker's scaled metres.
 ******************************************************************************
 */

#ifndef TRAIL_WRONG_WAY_HPP
#define TRAIL_WRONG_WAY_HPP

#include <cstdint>

#include "OffCourse.hpp"
#include "RouteTracker.hpp"

namespace Trail
{

class WrongWay
{
public:
    enum class State : uint8_t { NotStarted, RightWay, Wrong, Finished };
    using Event = OffCourse::Event;

    struct Config {
        float    backM         = 30.0f;    ///< this far back along the route ...
        uint32_t confirmMs     = 3000u;    ///< ... for this long: wrong way
        float    rightM        = 20.0f;    ///< this far forward again, for confirmMs: right way
        uint32_t remindMs      = 60000u;   ///< 0: no reminders
        float    nearM         = 12.0f;    ///< fixes further from the line do not count
        float    maxPrecisionM = 20.0f;    ///< nor fixes worse than this; 0 or less is "unknown", which counts
        uint16_t maxFixes      = 60;       ///< the anchor starts again after this many fixes without a new furthest point
    };

    WrongWay() = default;
    explicit WrongWay(const Config& config) : mConfig(config) {}

    void reset();

    /// One fix: the tracker's position, the GPS's precision, and whether the
    /// off-course alert is on (OffCourse::State::Off).
    Event update(uint32_t nowMs, const RouteTracker::Position& pos, float precisionM, bool offCourse);

    State         state() const { return mState; }
    bool          wrong() const { return mState == State::Wrong; }
    const Config& config() const { return mConfig; }

private:
    void arm(float alongM);

    Config   mConfig {};
    State    mState       = State::NotStarted;
    float    mAnchorM     = 0.0f;   ///< furthest along since the count started
    float    mLowM        = 0.0f;   ///< while wrong: the lowest along
    uint16_t mSinceAnchor = 0;      ///< counted fixes since the anchor moved
    bool     mPending     = false;  ///< a count towards the other state is running
    uint32_t mPendingMs   = 0;
    uint32_t mLastAlertMs = 0;
};

} // namespace Trail

#endif // TRAIL_WRONG_WAY_HPP
