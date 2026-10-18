/**
 ******************************************************************************
 * @file    HoldTimer.hpp
 * @brief   One button's hold, and its taps: "hold N ms to do the careful
 *          thing", and "a quick press does the ordinary thing" exactly once.
 *          Pure C++, header-only, with the clock passed in. Copied from
 *          Knitting (knitting/Software/Libs/Core/Header/HoldTimer.hpp), with
 *          its tests.
 *
 * The kernel sends a press code, a release code and a click code per button
 * (SDK/GUI/Button.hpp) and no long-press code, so a screen times the hold
 * itself. What is known of the order (the PC simulator, which follows the
 * kernel, LvglHost.cpp): press, then a click only if the button comes up
 * within 500 ms, then release. A press of 500 ms or more therefore sends no
 * click at all, and the real watch's order is not documented. So a tap is
 * whichever of click or release comes first, and the other is ignored:
 *
 *   - press(now) starts it; poll(now) is true exactly once, when held for the
 *     full length (the hold has fired);
 *   - release(now) returns true if it ends a press that was not a hold and
 *     has not yet been counted as a tap (a slow tap, or a click that came
 *     later). It returns false after a fired hold, and after a click already
 *     counted;
 *   - takeClick(now) returns true if the click is a tap to act on, false if
 *     the press was a fired hold or a release already counted it. A click
 *     counts as a duplicate only within kClickMs of the release;
 *   - repeated press codes while held are ignored; cancel() drops everything,
 *     for a suspend that swallows the release.
 *
 * The clock is a wrapping 32-bit millisecond count: every difference is an
 * unsigned subtraction.
 ******************************************************************************
 */

#ifndef TRAILQUEST_HOLD_TIMER_HPP
#define TRAILQUEST_HOLD_TIMER_HPP

#include <cstdint>

namespace TrailQuest
{

class HoldTimer
{
public:
    /// A click this soon after a release is that same press's, not a new tap.
    static constexpr uint32_t kClickMs = 400;

    explicit HoldTimer(uint32_t holdMs) : mHoldMs(holdMs == 0 ? 1 : holdMs) {}

    void press(uint32_t nowMs)
    {
        if (mHeld) {
            return;
        }
        mHeld      = true;
        mFired     = false;
        mTapped    = false;
        mSwallow   = false;
        mPressedAt = nowMs;
    }

    /// The button came up. True if that is a tap nobody has counted yet.
    bool release(uint32_t nowMs)
    {
        if (!mHeld) {
            return false;
        }
        const bool tap = !mFired && !mTapped;
        mSwallow    = mFired || tap;    // a click right after this is a duplicate
        mReleasedAt = nowMs;
        mHeld       = false;
        mFired      = false;
        return tap;
    }

    void cancel()
    {
        mHeld    = false;
        mFired   = false;
        mTapped  = false;
        mSwallow = false;
    }

    /// True once, when the button has been held for the full length.
    bool poll(uint32_t nowMs)
    {
        if (!mHeld || mFired || nowMs - mPressedAt < mHoldMs) {
            return false;
        }
        mFired = true;
        return true;
    }

    /// How far through the hold, 0..1000. Zero when not held.
    uint32_t progressPermille(uint32_t nowMs) const
    {
        if (!mHeld) {
            return 0;
        }
        const uint32_t elapsed = nowMs - mPressedAt;
        return elapsed >= mHoldMs ? 1000u : elapsed * 1000u / mHoldMs;
    }

    /// A click arrived: true if it is a tap to act on.
    bool takeClick(uint32_t nowMs)
    {
        if (mHeld) {
            if (mFired || mTapped) {
                return false;
            }
            mTapped = true;             // the release that follows must not count it again
            return true;
        }
        const bool duplicate = mSwallow && nowMs - mReleasedAt < kClickMs;
        mSwallow = false;
        return !duplicate;
    }

    bool held() const { return mHeld; }

    /// Held and not yet fired: the hold a progress bar should show.
    bool pending() const { return mHeld && !mFired; }

    uint32_t holdMs() const { return mHoldMs; }

private:
    uint32_t mHoldMs;
    bool     mHeld       = false;
    bool     mFired      = false;
    bool     mTapped     = false;   ///< this press has already counted as a tap
    bool     mSwallow    = false;
    uint32_t mPressedAt  = 0;
    uint32_t mReleasedAt = 0;
};

} // namespace TrailQuest

#endif // TRAILQUEST_HOLD_TIMER_HPP
