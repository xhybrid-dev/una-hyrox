/**
 ******************************************************************************
 * @file    HeatHint.hpp
 * @brief   A rough guide to the surroundings from the watch's temperature
 *          reading: cooler, similar or warmer than where the session started,
 *          and whether it is rising or falling. Pure C++, header-only, with
 *          the clock passed in.
 *
 * Gate SA0 (Jon, 21 October 2026): the sensor reads between room and body
 * temperature, so it is a temperature inside the watch with a long lag, not
 * the air. It is therefore used only as a hint, and only RELATIVE: against the
 * mean of the session's first minute, never as a number to trust. The hint
 * never changes the phase; the lap button does.
 *
 * Every threshold below is an UNTUNED placeholder. No sauna or plunge reading
 * exists yet to set them from, so the hint is OFF (Config::enabled false) and
 * says Unknown until it is switched on with numbers from real probe logs.
 *
 * The reading is smoothed (a 1-in-8 moving average), compared with the
 * baseline for the level, and with its own value about a minute ago for the
 * trend. Both use hysteresis: a state is left at half the threshold that
 * entered it, so a reading sitting on a threshold does not flicker.
 * Readings are hundredths of the sensor's own unit (FixedPoint.hpp).
 ******************************************************************************
 */

#ifndef SAUNA_HEAT_HINT_HPP
#define SAUNA_HEAT_HINT_HPP

#include <cstdint>

#include "FixedPoint.hpp"

namespace Sauna {

struct HeatHintConfig {
    bool     enabled = false;           ///< Off until tuned from real data
    uint32_t baselineMs = 60000;        ///< How long the starting mean is taken over
    uint32_t baselineMinSamples = 20;   ///< ...and from at least this many samples
    int32_t  warmerX100 = 300;          ///< UNTUNED: above baseline by this is Warmer
    int32_t  coolerX100 = 300;          ///< UNTUNED: below baseline by this is Cooler
    int32_t  risingX100 = 100;          ///< UNTUNED: change over about a minute
};

class HeatHint {
public:
    enum class Level : uint8_t { Unknown = 0, Cooler, Similar, Warmer };
    enum class Trend : uint8_t { Unknown = 0, Falling, Steady, Rising };

    HeatHint() = default;
    explicit HeatHint(const HeatHintConfig &config) : mCfg(config) {}

    void reset()
    {
        *this = HeatHint(mCfg);
    }

    /// A reading at @p nowMs, about once a second. kX100Invalid is ignored.
    void add(uint32_t nowMs, int32_t tempX100)
    {
        if (tempX100 == kX100Invalid) {
            return;
        }
        const int64_t scaled = static_cast<int64_t>(tempX100) * 16;
        if (!mAny) {
            mAny        = true;
            mFirstMs    = nowMs;
            mLastSlotMs = nowMs;
            mEma16      = scaled;
        } else {
            mEma16 += (scaled - mEma16) / 8;
        }
        const int32_t smooth = static_cast<int32_t>(mEma16 / 16);

        if (!mBaselineDone) {
            mBaseSum += tempX100;
            ++mBaseCount;
            if (nowMs - mFirstMs >= mCfg.baselineMs && mBaseCount >= mCfg.baselineMinSamples) {
                mBaseline     = static_cast<int32_t>(mBaseSum / static_cast<int64_t>(mBaseCount));
                mBaselineDone = true;
            }
        }

        if (mBaselineDone) {
            mDelta = smooth - mBaseline;
            mLevel = nextLevel(mLevel, mDelta);
        }

        if (mRingCount == kRing) {
            const int32_t change = smooth - mRing[mHead];    // the oldest slot
            mTrend = nextTrend(mTrend, change);
        }
        if (nowMs - mLastSlotMs >= kSlotMs) {
            mLastSlotMs   = nowMs;
            mRing[mHead]  = smooth;
            mHead         = static_cast<uint8_t>((mHead + 1u) % kRing);
            if (mRingCount < kRing) {
                ++mRingCount;
            }
        }
    }

    /// Cooler, similar or warmer than the first minute; Unknown when off or
    /// the baseline is not in yet.
    Level level() const { return mCfg.enabled && mBaselineDone ? mLevel : Level::Unknown; }

    /// Rising, steady or falling over the last minute or so; Unknown when off
    /// or there is not a minute of readings.
    Trend trend() const { return mCfg.enabled && mRingCount == kRing ? mTrend : Trend::Unknown; }

    /// Smoothed reading minus the baseline; 0 until the baseline is in.
    int32_t deltaX100() const { return mBaselineDone ? mDelta : 0; }

    bool baselineReady() const { return mBaselineDone; }
    int32_t baselineX100() const { return mBaselineDone ? mBaseline : kX100Invalid; }

private:
    static constexpr uint8_t  kRing   = 7;        ///< 7 slots, 10 s apart: a minute
    static constexpr uint32_t kSlotMs = 10000;

    Level nextLevel(Level now, int32_t delta) const
    {
        const int32_t warm = mCfg.warmerX100;
        const int32_t cool = mCfg.coolerX100;
        switch (now) {
        case Level::Warmer:
            return delta >= warm / 2 ? Level::Warmer : Level::Similar;
        case Level::Cooler:
            return delta <= -(cool / 2) ? Level::Cooler : Level::Similar;
        default:
            if (delta >= warm) {
                return Level::Warmer;
            }
            if (delta <= -cool) {
                return Level::Cooler;
            }
            return Level::Similar;
        }
    }

    Trend nextTrend(Trend now, int32_t change) const
    {
        const int32_t t = mCfg.risingX100;
        switch (now) {
        case Trend::Rising:
            return change >= t / 2 ? Trend::Rising : Trend::Steady;
        case Trend::Falling:
            return change <= -(t / 2) ? Trend::Falling : Trend::Steady;
        default:
            if (change >= t) {
                return Trend::Rising;
            }
            if (change <= -t) {
                return Trend::Falling;
            }
            return Trend::Steady;
        }
    }

    HeatHintConfig mCfg;
    bool     mAny = false;
    bool     mBaselineDone = false;
    uint32_t mFirstMs = 0;
    uint32_t mLastSlotMs = 0;
    int64_t  mEma16 = 0;
    int64_t  mBaseSum = 0;
    uint32_t mBaseCount = 0;
    int32_t  mBaseline = 0;
    int32_t  mDelta = 0;
    int32_t  mRing[kRing] = {};
    uint8_t  mHead = 0;
    uint8_t  mRingCount = 0;
    Level    mLevel = Level::Similar;
    Trend    mTrend = Trend::Steady;
};

} // namespace Sauna

#endif // SAUNA_HEAT_HINT_HPP
