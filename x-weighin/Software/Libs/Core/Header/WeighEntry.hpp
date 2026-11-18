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
#include "WeightUnit.hpp"

namespace XWeighIn
{

/// The entry works in tenths of the display unit: tenths of a kg, or tenths
/// of a lb (also for stones and pounds, which only differ in how it is shown).
/// One press is one tenth. The log still holds 0.01 kg: toEntry() and
/// fromEntry() convert.
constexpr int32_t kEntryStep = 1;
/// What the log accepts: 1.0-500.0 kg, in hundredths.
constexpr int32_t kEntryMinHundredths = 100;
constexpr int32_t kEntryMaxHundredths = kMaxKg * 100;
/// Where the entry starts with no log and no profile weight: 70.0 kg.
constexpr int32_t kEntryFallback = 7000;

/// Hundredths of a kg to entry units (tenths of the unit), halves up.
inline int32_t toEntry(int32_t hundredths, WeightUnit unit)
{
    return unit == WeightUnit::Kg ? (hundredths + 5) / 10 : hundredthsToLbTenths(hundredths);
}

/// Entry units back to hundredths of a kg.
inline int32_t fromEntry(int32_t entry, WeightUnit unit)
{
    return unit == WeightUnit::Kg ? entry * 10 : lbTenthsToHundredths(entry);
}

/// The entry's range in @p unit; fromEntry() of either end is a weight the log accepts.
inline int32_t entryMin(WeightUnit unit)
{
    return toEntry(kEntryMinHundredths, unit);
}
inline int32_t entryMax(WeightUnit unit)
{
    return toEntry(kEntryMaxHundredths, unit);
}

inline int32_t clampEntry(int32_t entry, WeightUnit unit)
{
    const int32_t lo = entryMin(unit);
    const int32_t hi = entryMax(unit);
    return entry < lo ? lo : (entry > hi ? hi : entry);
}

/**
 * @brief   Where the weigh-in screen starts, in hundredths of a kg, before it
 *          is rounded to the unit's step: the last logged weigh-in, else the
 *          UNA profile's weight (only a starting point: the profile does not
 *          follow changes, NOTES Gate W0), else kEntryFallback.
 */
inline int32_t startBase(int32_t lastLogged, int32_t profile)
{
    if (lastLogged > 0) {
        return lastLogged;
    }
    return profile > 0 ? profile : kEntryFallback;
}

/// The value the weigh-in screen opens on, in entry units of @p unit: @p base
/// (startBase()) rounded to the unit's step and kept in range.
inline int32_t startValue(int32_t base, WeightUnit unit)
{
    return clampEntry(toEntry(base, unit), unit);
}

/// @p value moved by @p steps presses (negative is down), within the range.
inline int32_t stepEntry(int32_t value, int32_t steps, WeightUnit unit)
{
    return clampEntry(value + steps * kEntryStep, unit);
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
