/**
 ******************************************************************************
 * @file    OffCourse.hpp
 * @brief   When to buzz: the off-course alert, as a small state machine.
 *
 * The rules, in the order they apply to each GPS fix:
 *
 *   1. Before the runner first reaches the route (RouteTracker's everLocked),
 *      nothing: walking to the start is not being lost. (NotStarted)
 *   2. After the finish, nothing more, ever: one Finished event. (Finished)
 *   3. A fix whose own precision is worse than maxPrecisionM changes nothing,
 *      and restarts any count in progress: under trees or in a gully a bad fix
 *      can land 60 m away, and one bad fix must never buzz.
 *   4. On course, more than offM from the route for confirmOffMs in a row:
 *      WentOff. (Off)
 *   5. Off course, within backM of the route for confirmBackMs in a row:
 *      BackOn. (OnCourse)
 *   6. Still off course, every remindMs: StillOff, a reminder.
 *
 * offM and backM differ on purpose (hysteresis): a runner 40 m off the line
 * on a switchback that doesn't match the GPX exactly neither triggers the
 * alert nor, once alerted, clears it by wobbling between 45 and 55 m.
 *
 * Time comes from the caller (the kernel's millisecond clock), and is only
 * ever subtracted as unsigned: right across the clock's wrap.
 ******************************************************************************
 */

#ifndef TRAIL_OFF_COURSE_HPP
#define TRAIL_OFF_COURSE_HPP

#include <cstdint>

#include "RouteTracker.hpp"

namespace Trail
{

class OffCourse
{
public:
    enum class State : uint8_t { NotStarted, OnCourse, Off, Finished };
    enum class Event : uint8_t { None, WentOff, StillOff, BackOn, Finished };

    struct Config {
        float    offM          = 50.0f;
        float    backM         = 30.0f;
        uint32_t confirmOffMs  = 5000u;
        uint32_t confirmBackMs = 3000u;
        uint32_t remindMs      = 60000u;   ///< 0: no reminders
        float    maxPrecisionM = 25.0f;    ///< worse fixes are ignored; 0 or less means "unknown", which counts
    };

    OffCourse() = default;
    explicit OffCourse(const Config& config) : mConfig(config) {}

    void reset();

    /// One fix: the tracker's position for it, and the GPS's precision.
    Event update(uint32_t nowMs, const RouteTracker::Position& pos, float precisionM);

    State         state() const { return mState; }
    const Config& config() const { return mConfig; }

    /// How long the runner has been off course (0 unless Off).
    uint32_t offForMs(uint32_t nowMs) const { return mState == State::Off ? nowMs - mOffSinceMs : 0u; }

    static const char* name(State s);
    static const char* name(Event e);

private:
    Config   mConfig {};
    State    mState       = State::NotStarted;
    bool     mPending     = false;   ///< a count towards the other state is running
    uint32_t mPendingMs   = 0;       ///< since when
    uint32_t mOffSinceMs  = 0;
    uint32_t mLastAlertMs = 0;
};

} // namespace Trail

#endif // TRAIL_OFF_COURSE_HPP
