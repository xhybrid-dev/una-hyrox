/**
 ******************************************************************************
 * @file    SessionReview.cpp
 * @brief   How well a planned session kept to its paces. See SessionReview.hpp.
 ******************************************************************************
 */

#include "SessionReview.hpp"

namespace XTraining {
namespace Review {

namespace {

constexpr int32_t kMaxGap = 600;   ///< s/km: a gap beyond this is a lost signal, not a pace.

int32_t magnitude(int32_t v) { return v < 0 ? -v : v; }

int32_t clampGap(int32_t g) { return g > kMaxGap ? kMaxGap : g < -kMaxGap ? -kMaxGap : g; }

/// The signed gap a class stands for: faster is negative, slower positive, in band none.
int32_t signedGap(Cls cls, int32_t gap)
{
    const int32_t m = magnitude(clampGap(gap));
    return cls == Cls::Fast ? -m : cls == Cls::Slow ? m : 0;
}

/// Rounded integer division, away from zero at a half.
int32_t divRound(int64_t num, int64_t den)
{
    if (den <= 0) {
        return 0;
    }
    return static_cast<int32_t>(num >= 0 ? (num + den / 2) / den : -((-num + den / 2) / den));
}

uint8_t pct(uint32_t part, uint32_t total) { return static_cast<uint8_t>((static_cast<uint64_t>(part) * 100u + total / 2u) / total); }

}  // namespace

void Accumulator::reset()
{
    *this = Accumulator{};
}

void Accumulator::addSecond(Cls cls, int32_t gapSecKm, bool fastOnly)
{
    if (cls == Cls::None) {
        return;
    }
    if (fastOnly && cls == Cls::Slow) {
        cls = Cls::In;   // an easy step: slower than the band is not a miss
    }
    const int32_t g = signedGap(cls, gapSecKm);
    switch (cls) {
    case Cls::In:
        ++mInSec;
        if (mUIn < 0xFFFFu) {
            ++mUIn;
        }
        break;
    case Cls::Fast:
        ++mFastSec;
        mFastGap += static_cast<uint32_t>(-g);
        if (mUFast < 0xFFFFu) {
            ++mUFast;
            mUGap += g;
        }
        break;
    case Cls::Slow:
        ++mSlowSec;
        mSlowGap += static_cast<uint32_t>(g);
        if (mUSlow < 0xFFFFu) {
            ++mUSlow;
            mUGap += g;
        }
        break;
    case Cls::None:
        break;
    }
}

void Accumulator::addRep(Cls cls, int32_t gapSecKm, uint32_t sec)
{
    if (cls == Cls::None || sec == 0) {
        return;
    }
    endUnit();   // a rep is a unit of its own
    const uint16_t s = sec > 0xFFFFu ? static_cast<uint16_t>(0xFFFFu) : static_cast<uint16_t>(sec);
    const int32_t  g = signedGap(cls, gapSecKm);
    switch (cls) {
    case Cls::In:
        mInSec += s;
        closeInto(s, 0, 0, 0);
        break;
    case Cls::Fast:
        mFastSec += s;
        mFastGap += static_cast<uint64_t>(-g) * s;
        closeInto(0, s, 0, g * static_cast<int32_t>(s));
        break;
    case Cls::Slow:
        mSlowSec += s;
        mSlowGap += static_cast<uint64_t>(g) * s;
        closeInto(0, 0, s, g * static_cast<int32_t>(s));
        break;
    case Cls::None:
        break;
    }
}

void Accumulator::endUnit()
{
    if (mUIn + mUFast + mUSlow == 0) {
        return;
    }
    closeInto(mUIn, mUFast, mUSlow, mUGap);
    mUIn = mUFast = mUSlow = 0;
    mUGap = 0;
}

void Accumulator::closeInto(uint16_t in, uint16_t fast, uint16_t slow, int32_t gapSum)
{
    const uint32_t total = static_cast<uint32_t>(in) + fast + slow;
    if (total == 0) {
        return;
    }
    // A unit is in band when at least half its time was; otherwise it is as its misses lean.
    const Cls cls = static_cast<uint32_t>(in) * 2u >= total ? Cls::In : (fast > slow ? Cls::Fast : Cls::Slow);
    if (mUnitsAll < 0xFFu) {
        ++mUnitsAll;
        if (cls == Cls::In) {
            ++mUnitsIn;
        }
    }
    if (mUnitCount < kMaxUnits) {
        Unit& u   = mUnit[mUnitCount++];
        u.sec     = total > 0xFFFFu ? static_cast<uint16_t>(0xFFFFu) : static_cast<uint16_t>(total);
        u.meanGap = static_cast<int16_t>(divRound(gapSum, total));
        u.cls     = static_cast<uint8_t>(cls);
    }
}

Result Accumulator::finish() const
{
    Result r;
    const uint32_t total = mInSec + mFastSec + mSlowSec;
    r.judgedSec = total > 0xFFFFu ? static_cast<uint16_t>(0xFFFFu) : static_cast<uint16_t>(total);

    // The open unit counts as the last one, without closing it.
    Unit open;
    bool haveOpen = false;
    uint8_t unitsAll = mUnitsAll;
    uint8_t unitsIn  = mUnitsIn;
    const uint32_t openTotal = static_cast<uint32_t>(mUIn) + mUFast + mUSlow;
    if (openTotal != 0) {
        haveOpen = true;
        open.sec = openTotal > 0xFFFFu ? static_cast<uint16_t>(0xFFFFu) : static_cast<uint16_t>(openTotal);
        open.meanGap = static_cast<int16_t>(divRound(mUGap, openTotal));
        if (unitsAll < 0xFFu) {
            ++unitsAll;
            if (static_cast<uint32_t>(mUIn) * 2u >= openTotal) {
                ++unitsIn;
            }
        }
    }
    r.units   = unitsAll;
    r.unitsIn = unitsIn;

    r.judged = total >= kMinJudgedSec || (unitsAll >= 2 && total >= kMinRepsSec);
    if (!r.judged) {
        return r;
    }

    // The shares, rounded so that they add to 100.
    uint8_t in   = pct(mInSec, total);
    uint8_t fast = pct(mFastSec, total);
    uint8_t slow = pct(mSlowSec, total);
    int sum = in + fast + slow;
    if (sum != 100) {
        uint8_t* biggest = in >= fast && in >= slow ? &in : (fast >= slow ? &fast : &slow);
        *biggest = static_cast<uint8_t>(*biggest + (100 - sum));
    }
    r.inPct   = in;
    r.fastPct = fast;
    r.slowPct = slow;

    // The average miss in the dominant direction.
    if (mFastSec > mSlowSec) {
        r.gapSecKm = static_cast<int16_t>(-divRound(static_cast<int64_t>(mFastGap), mFastSec));
    } else if (mSlowSec > 0) {
        r.gapSecKm = static_cast<int16_t>(divRound(static_cast<int64_t>(mSlowGap), mSlowSec));
    }

    // The fade: the last third of the reps against the first third, time-weighted.
    const uint8_t n = static_cast<uint8_t>(mUnitCount + (haveOpen && mUnitCount < kMaxUnits ? 1 : 0));
    if (n >= kFadeMinUnits) {
        auto at = [&](uint8_t i) -> const Unit& { return i < mUnitCount ? mUnit[i] : open; };
        const uint8_t k = static_cast<uint8_t>(n / 3);
        int64_t firstSum = 0, lastSum = 0;
        uint32_t firstSec = 0, lastSec = 0;
        for (uint8_t i = 0; i < k; ++i) {
            const Unit& a = at(i);
            const Unit& b = at(static_cast<uint8_t>(n - 1 - i));
            firstSum += static_cast<int64_t>(a.meanGap) * a.sec;
            firstSec += a.sec;
            lastSum += static_cast<int64_t>(b.meanGap) * b.sec;
            lastSec += b.sec;
        }
        if (firstSec != 0 && lastSec != 0) {
            const int32_t f = divRound(lastSum, lastSec) - divRound(firstSum, firstSec);
            r.fadeSecKm = static_cast<int8_t>(f > 100 ? 100 : f < -100 ? -100 : f);
        }
    }
    return r;
}

int32_t repGapSecKm(const SDK::Workout::Step& step, const RepJudge::Result& r)
{
    if (!r.ok || (r.verdict != RepJudge::Verdict::Fast && r.verdict != RepJudge::Verdict::Slow)) {
        return 0;
    }
    const int32_t d = r.verdict == RepJudge::Verdict::Fast
                          ? static_cast<int32_t>(r.value) - static_cast<int32_t>(r.fastEnd)
                          : static_cast<int32_t>(r.value) - static_cast<int32_t>(r.slowEnd);
    if (r.byTime) {
        // Seconds over the step's distance, as s/km: d / (distanceCm / 100000).
        if (step.distanceCm == 0) {
            return 0;
        }
        return static_cast<int32_t>(static_cast<int64_t>(d) * 100000 / static_cast<int64_t>(step.distanceCm));
    }
    return d;
}

}  // namespace Review
}  // namespace XTraining
