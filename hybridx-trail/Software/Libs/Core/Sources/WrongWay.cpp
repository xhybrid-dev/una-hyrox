/**
 ******************************************************************************
 * @file    WrongWay.cpp
 * @brief   The wrong-way alert state machine (see the header).
 ******************************************************************************
 */

#include "WrongWay.hpp"

namespace Trail
{

void WrongWay::reset()
{
    mState       = State::NotStarted;
    mAnchorM     = 0.0f;
    mLowM        = 0.0f;
    mSinceAnchor = 0;
    mPending     = false;
    mPendingMs   = 0;
    mLastAlertMs = 0;
}

void WrongWay::arm(float alongM)
{
    mAnchorM     = alongM;
    mSinceAnchor = 0;
    mPending     = false;
}

WrongWay::Event WrongWay::update(uint32_t nowMs, const RouteTracker::Position& pos, float precisionM,
                                 bool offCourse)
{
    if (mState == State::Finished) {
        return Event::None;
    }
    if (pos.finished) {
        mState   = State::Finished;   // a wrong-way alert, if any, ends quietly
        mPending = false;
        return Event::None;
    }
    if (mState == State::NotStarted) {
        if (pos.everLocked) {
            mState = State::RightWay;
            arm(pos.alongM);
        }
        return Event::None;
    }
    if (offCourse) {
        mState = State::RightWay;   // quietly: the off-course alert is the news now
        arm(pos.alongM);
        return Event::None;
    }
    if (pos.jumped) {
        // Somewhere new on the route (a rejoin, a shortcut, a reversed loop's
        // start): count again from here. Still wrong until they run forward.
        if (mState == State::Wrong) {
            mLowM    = pos.alongM;
            mPending = false;
        } else {
            arm(pos.alongM);
        }
        return Event::None;
    }
    if ((mConfig.maxPrecisionM > 0.0f && precisionM > mConfig.maxPrecisionM) || !pos.onRoute ||
        pos.offRouteM > mConfig.nearM) {
        mPending = false;   // a fix that does not count breaks any run of ones that do
        return Event::None;
    }

    if (mState == State::RightWay) {
        if (pos.alongM > mAnchorM || mSinceAnchor >= mConfig.maxFixes) {
            arm(pos.alongM);
            return Event::None;
        }
        ++mSinceAnchor;
        if (pos.wayBack || mAnchorM - pos.alongM < mConfig.backM) {
            mPending = false;
            return Event::None;
        }
        if (!mPending) {
            mPending   = true;
            mPendingMs = nowMs;
        }
        if (nowMs - mPendingMs >= mConfig.confirmMs) {
            mState       = State::Wrong;
            mPending     = false;
            mLowM        = pos.alongM;
            mLastAlertMs = nowMs;
            return Event::WentWrong;
        }
        return Event::None;
    }

    // State::Wrong
    if (pos.alongM < mLowM) {
        mLowM = pos.alongM;
    }
    if (pos.alongM >= mLowM + mConfig.rightM) {
        if (!mPending) {
            mPending   = true;
            mPendingMs = nowMs;
        }
        if (nowMs - mPendingMs >= mConfig.confirmMs) {
            mState = State::RightWay;
            arm(pos.alongM);
            return Event::RightWay;
        }
        return Event::None;
    }
    mPending = false;
    if (mConfig.remindMs > 0 && nowMs - mLastAlertMs >= mConfig.remindMs) {
        mLastAlertMs = nowMs;
        return Event::StillWrong;
    }
    return Event::None;
}

} // namespace Trail
