/**
 ******************************************************************************
 * @file    LapLockout.hpp
 * @brief   Ignores a second lap press that follows the first too closely, so a
 *          double tap with a wet finger does not skip a phase. Pure C++,
 *          header-only, with the clock passed in (a wrapping 32-bit
 *          millisecond count: unsigned differences).
 ******************************************************************************
 */

#ifndef SAUNA_LAP_LOCKOUT_HPP
#define SAUNA_LAP_LOCKOUT_HPP

#include <cstdint>

namespace Sauna {

/// How soon after a lap another is ignored. A default for Jon to adjust: no
/// phase of a sauna session is anywhere near this short.
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

} // namespace Sauna

#endif // SAUNA_LAP_LOCKOUT_HPP
