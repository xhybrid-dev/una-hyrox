/**
 ******************************************************************************
 * @file    WristLog.hpp
 * @brief   The wrist raises the watch has reported, and how long ago the last
 *          one was. Times are the OS millisecond tick: unsigned arithmetic, so
 *          the wrap at about 49 days is harmless (Docs/writing-a-clockface.md).
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_WRIST_LOG_HPP
#define CLOCK_PROBE_WRIST_LOG_HPP

#include <cstdint>

namespace ClockProbe
{

class WristLog
{
public:
    static constexpr uint8_t kKeep = 16;

    void record(uint32_t nowMs)
    {
        mRing[mHead] = nowMs;
        mHead = static_cast<uint8_t>((mHead + 1u) % kKeep);
        if (mKept < kKeep) {
            ++mKept;
        }
        ++mToday;
        ++mTotal;
    }

    /// A new local day: today's count starts again; the total and the ring do not.
    void newDay() { mToday = 0; }

    uint32_t today() const { return mToday; }
    uint32_t total() const { return mTotal; }
    bool     any() const { return mTotal > 0; }

    /// Milliseconds since the last raise; 0xFFFFFFFF if there has been none.
    uint32_t lastAgeMs(uint32_t nowMs) const
    {
        if (mKept == 0) {
            return 0xFFFFFFFFu;
        }
        const uint8_t last = static_cast<uint8_t>((mHead + kKeep - 1u) % kKeep);
        return static_cast<uint32_t>(nowMs - mRing[last]);
    }

    /// Raises among the last kKeep that are no older than @p windowMs.
    uint8_t within(uint32_t nowMs, uint32_t windowMs) const
    {
        uint8_t n = 0;
        for (uint8_t i = 0; i < mKept; ++i) {
            if (static_cast<uint32_t>(nowMs - mRing[i]) <= windowMs) {
                ++n;
            }
        }
        return n;
    }

private:
    uint32_t mRing[kKeep] = {};
    uint8_t  mHead  = 0;
    uint8_t  mKept  = 0;
    uint32_t mToday = 0;
    uint32_t mTotal = 0;
};

} // namespace ClockProbe

#endif // CLOCK_PROBE_WRIST_LOG_HPP
