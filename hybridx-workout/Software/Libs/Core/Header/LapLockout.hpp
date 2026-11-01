/**
 ******************************************************************************
 * @file    LapLockout.hpp
 * @brief   Ignores a second lap press that follows the first too closely, so a
 *          double tap does not end a lap at once. (X Sauna's.) Pure C++,
 *          header-only, with the clock passed in (a wrapping 32-bit
 *          millisecond count: unsigned differences).
 ******************************************************************************
 */

#ifndef WORKOUT_LAP_LOCKOUT_HPP
#define WORKOUT_LAP_LOCKOUT_HPP

#include <cstdint>

namespace Workout {

/// How soon after a lap another is ignored. A default for Jon to adjust: no set,
/// round or station is anywhere near this short.
constexpr uint32_t kLapLockoutMs = 3000;

class LapLockout {
public:
    explicit LapLockout(uint32_t lockoutMs = kLapLockoutMs) : mLockoutMs(lockoutMs) {}

    /// True if a press at @p nowMs counts, and records it; false inside the lockout.
    bool allow(uint32_t nowMs)
    {
        if (mHave && nowMs - mLastMs < mLockoutMs) {
            return false;
        }
        mHave   = true;
        mLastMs = nowMs;
        return true;
    }

    /// A new session: the first press counts.
    void reset() { mHave = false; }

private:
    uint32_t mLockoutMs;
    uint32_t mLastMs = 0;
    bool     mHave = false;
};

} // namespace Workout

#endif // WORKOUT_LAP_LOCKOUT_HPP
