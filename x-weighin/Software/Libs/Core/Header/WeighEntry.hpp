/**
 ******************************************************************************
 * @file    WeighEntry.hpp
 * @brief   The weigh-in screen's arithmetic: where the value starts, how a
 *          press moves it, and how a held button repeats.
 *
 * Header-only: the GUI uses it, and the GUI ELF links no Libs sources.
 * Host-tested. Time is injected (milliseconds from the watch's wrapping
 * clock) and every difference is unsigned, so a wrap is harmless.
 ******************************************************************************
 */

#ifndef XWEIGHIN_WEIGH_ENTRY_HPP
#define XWEIGHIN_WEIGH_ENTRY_HPP

#include <cstdint>

#include "WeighProfile.hpp"

namespace XWeighIn
{

/// One press: 0.1 kg, in 0.01 kg.
constexpr int32_t kEntryStep     = 10;
/// The entry's range, 1.0-500.0 kg: what the log accepts.
constexpr int32_t kEntryMin      = 100;
constexpr int32_t kEntryMax      = kMaxKg * 100;
/// Where the entry starts with no log and no profile weight: 70.0 kg.
constexpr int32_t kEntryFallback = 7000;

/// To the nearest 0.1 kg, halves up (positive values).
inline int32_t roundToTenth(int32_t hundredths)
{
    return ((hundredths + 5) / 10) * 10;
}

inline int32_t clampEntry(int32_t hundredths)
{
    return hundredths < kEntryMin ? kEntryMin : (hundredths > kEntryMax ? kEntryMax : hundredths);
}

/**
 * @brief   The value the weigh-in screen opens on: the last logged weigh-in,
 *          else the UNA profile's weight (only a starting point: the profile
 *          does not follow changes, NOTES Gate W0), else kEntryFallback.
 *          Rounded to 0.1 kg, the entry's step.
 */
inline int32_t startValue(int32_t lastLogged, int32_t profile)
{
    if (lastLogged > 0) {
        return clampEntry(roundToTenth(lastLogged));
    }
    if (profile > 0) {
        return clampEntry(roundToTenth(profile));
    }
    return kEntryFallback;
}

/// @p value moved by @p steps presses (negative is down), within the range.
inline int32_t stepEntry(int32_t value, int32_t steps)
{
    return clampEntry(value + steps * kEntryStep);
}

/**
 * @brief   Hold-to-repeat for the UP and DOWN buttons, as the SDK's Timer
 *          editor does it (Examples/Apps/Timer, EditView.cpp): a tap is one
 *          step; a hold starts repeating after kStartMs, every kSlowMs at
 *          first and quicker each step down to kFastMs.
 *
 * The kernel sends press, release and click codes (SDK/GUI/Button.hpp). A tap
 * is counted from its click. A hold is counted from tick(); if a click follows
 * the release of a hold that repeated, takeClick() swallows it so the hold
 * does not end with an extra step.
 */
class HoldRepeat
{
public:
    static constexpr uint32_t kStartMs   = 600;
    static constexpr uint32_t kSlowMs    = 200;
    static constexpr uint32_t kFastMs    = 50;
    static constexpr uint32_t kAccelMs   = 25;
    /// A click this soon after a repeating hold's release belongs to it.
    static constexpr uint32_t kClickMs   = 500;
    /// The most steps one tick may return, so a stalled GUI does not jump.
    static constexpr uint32_t kMaxBurst  = 4;

    void press(uint32_t nowMs)
    {
        mHeld      = true;
        mRepeated  = false;
        mPressedAt = nowMs;
        mNextAt    = kStartMs;
        mInterval  = kSlowMs;
    }

    void release(uint32_t nowMs)
    {
        if (mHeld) {
            mHeld       = false;
            mReleasedAt = nowMs;
        }
    }

    /// Steps due since the last tick while held (0 when not held).
    uint32_t tick(uint32_t nowMs)
    {
        if (!mHeld) {
            return 0;
        }
        const uint32_t elapsed = nowMs - mPressedAt;   // unsigned: wrap-safe
        uint32_t steps = 0;
        while (elapsed >= mNextAt && steps < kMaxBurst) {
            ++steps;
            mRepeated = true;
            mNextAt  += mInterval;
            if (mInterval > kFastMs) {
                mInterval = (mInterval - kFastMs > kAccelMs) ? mInterval - kAccelMs : kFastMs;
            }
        }
        if (steps == kMaxBurst && elapsed >= mNextAt) {
            mNextAt = elapsed + mInterval;   // drop the backlog rather than leap
        }
        return steps;
    }

    /// A click arrived: true if it is a tap to count, false if it only ends a
    /// hold that has already been counted.
    bool takeClick(uint32_t nowMs)
    {
        const bool endsHold = mRepeated && (mHeld || nowMs - mReleasedAt < kClickMs);
        mRepeated = false;
        mHeld     = false;
        return !endsHold;
    }

    bool held() const { return mHeld; }

private:
    bool     mHeld       = false;
    bool     mRepeated   = false;
    uint32_t mPressedAt  = 0;
    uint32_t mReleasedAt = 0;
    uint32_t mNextAt     = 0;
    uint32_t mInterval   = kSlowMs;
};

}  // namespace XWeighIn

#endif  // XWEIGHIN_WEIGH_ENTRY_HPP
