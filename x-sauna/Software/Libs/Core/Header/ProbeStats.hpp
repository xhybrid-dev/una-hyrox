/**
 ******************************************************************************
 * @file    ProbeStats.hpp
 * @brief   What the Gate 0 probe keeps about one sensor's readings: the last,
 *          the range, the count and the gaps between samples. Pure C++,
 *          header-only, with the clock passed in.
 *
 * The probe exists to answer "what does this sensor actually do", so it keeps
 * the facts that answer that: how often a sample arrives (the shortest, the
 * longest and the last gap), and the lowest and highest reading since the
 * range was last cleared. Readings are hundredths (FixedPoint.hpp).
 *
 * Times are the sensor's own millisecond timestamps, a wrapping 32-bit count:
 * every difference is an unsigned subtraction.
 ******************************************************************************
 */

#ifndef SAUNA_PROBE_STATS_HPP
#define SAUNA_PROBE_STATS_HPP

#include <cstdint>

#include "FixedPoint.hpp"

namespace Sauna {

/// How a probed sensor is doing, for the screen and the log.
enum class Sense : uint8_t {
    NotStarted = 0,    ///< Not connected yet
    ConnectFailed,     ///< connect() said no: the sensor is not offered
    NoData,            ///< Connected, nothing has arrived
    Ok,                ///< Valid samples are arriving
};

inline const char *senseName(Sense s)
{
    switch (s) {
    case Sense::NotStarted:    return "not started";
    case Sense::ConnectFailed: return "connect failed";
    case Sense::NoData:        return "no data";
    case Sense::Ok:            return "ok";
    }
    return "?";
}

class StreamStats {
public:
    /// Record a valid sample taken at @p sampleMs with reading @p x100.
    void add(uint32_t sampleMs, int32_t x100)
    {
        if (mCount != 0) {
            mLastGapMs = sampleMs - mLastAtMs;
            if (mCount == 1 || mLastGapMs < mMinGapMs) {
                mMinGapMs = mLastGapMs;
            }
            if (mCount == 1 || mLastGapMs > mMaxGapMs) {
                mMaxGapMs = mLastGapMs;
            }
        }
        mLastAtMs = sampleMs;
        mLast     = x100;
        if (!mHasRange || x100 < mMin) {
            mMin = x100;
        }
        if (!mHasRange || x100 > mMax) {
            mMax = x100;
        }
        mHasRange = true;
        if (mCount < UINT32_MAX) {
            ++mCount;
        }
    }

    bool     has() const { return mCount != 0; }
    uint32_t count() const { return mCount; }
    int32_t  last() const { return mCount != 0 ? mLast : kX100Invalid; }
    int32_t  min() const { return mHasRange ? mMin : kX100Invalid; }
    int32_t  max() const { return mHasRange ? mMax : kX100Invalid; }

    /// Gaps between samples; 0 until there are two.
    uint32_t lastGapMs() const { return mLastGapMs; }
    uint32_t minGapMs() const { return mCount > 1 ? mMinGapMs : 0u; }
    uint32_t maxGapMs() const { return mCount > 1 ? mMaxGapMs : 0u; }

    /// Forget the lowest and highest, keeping the last reading (so the range
    /// then grows from where the sensor is now).
    void clearRange()
    {
        mHasRange = mCount != 0;
        mMin      = mLast;
        mMax      = mLast;
    }

private:
    uint32_t mCount     = 0;
    uint32_t mLastAtMs  = 0;
    uint32_t mLastGapMs = 0;
    uint32_t mMinGapMs  = 0;
    uint32_t mMaxGapMs  = 0;
    int32_t  mLast      = kX100Invalid;
    int32_t  mMin       = kX100Invalid;
    int32_t  mMax       = kX100Invalid;
    bool     mHasRange  = false;
};

} // namespace Sauna

#endif // SAUNA_PROBE_STATS_HPP
