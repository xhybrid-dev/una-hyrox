/**
 ******************************************************************************
 * @file    OffCourse.cpp
 * @brief   The off-course alert state machine (see the header).
 ******************************************************************************
 */

#include "OffCourse.hpp"

namespace Trail
{

void OffCourse::reset()
{
    mState       = State::NotStarted;
    mPending     = false;
    mPendingMs   = 0;
    mOffSinceMs  = 0;
    mLastAlertMs = 0;
}

OffCourse::Event OffCourse::update(uint32_t nowMs, const RouteTracker::Position& pos, float precisionM)
{
    if (mState == State::Finished) {
        return Event::None;
    }
    if (pos.finished) {
        mState   = State::Finished;
        mPending = false;
        return Event::Finished;
    }
    if (mState == State::NotStarted) {
        if (pos.everLocked) {
            mState = State::OnCourse;
        }
        return Event::None;
    }
    if (precisionM > mConfig.maxPrecisionM) {
        mPending = false;   // a bad fix breaks any run of good ones
        return Event::None;
    }

    if (mState == State::OnCourse) {
        if (pos.offRouteM <= mConfig.offM) {
            mPending = false;
            return Event::None;
        }
        if (!mPending) {
            mPending   = true;
            mPendingMs = nowMs;
        }
        if (nowMs - mPendingMs >= mConfig.confirmOffMs) {
            mState       = State::Off;
            mPending     = false;
            mOffSinceMs  = mPendingMs;   // off since the first fix that was
            mLastAlertMs = nowMs;
            return Event::WentOff;
        }
        return Event::None;
    }

    // State::Off
    if (pos.offRouteM <= mConfig.backM) {
        if (!mPending) {
            mPending   = true;
            mPendingMs = nowMs;
        }
        if (nowMs - mPendingMs >= mConfig.confirmBackMs) {
            mState   = State::OnCourse;
            mPending = false;
            return Event::BackOn;
        }
        return Event::None;
    }
    mPending = false;
    if (mConfig.remindMs > 0 && nowMs - mLastAlertMs >= mConfig.remindMs) {
        mLastAlertMs = nowMs;
        return Event::StillOff;
    }
    return Event::None;
}

const char* OffCourse::name(State s)
{
    switch (s) {
        case State::NotStarted: return "not started";
        case State::OnCourse:   return "on course";
        case State::Off:        return "off course";
        case State::Finished:   return "finished";
    }
    return "?";
}

const char* OffCourse::name(Event e)
{
    switch (e) {
        case Event::None:     return "-";
        case Event::WentOff:  return "went off";
        case Event::StillOff: return "still off";
        case Event::BackOn:   return "back on";
        case Event::Finished: return "finished";
    }
    return "?";
}

} // namespace Trail
