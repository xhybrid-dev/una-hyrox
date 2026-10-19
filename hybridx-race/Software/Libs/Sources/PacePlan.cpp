/**
 ******************************************************************************
 * @file    PacePlan.cpp
 * @date    15-10-2026
 * @author  HybridX
 * @brief   See PacePlan.hpp.
 ******************************************************************************
 */

#include "PacePlan.hpp"

namespace Race
{
namespace
{

/// The last race's time for @p seg, scaled for run length, or 0 when it has none.
uint32_t lastRaceMs(const SegmentDesc &seg, const ActivitySummary &last, uint16_t runDistanceM,
                    uint16_t lastRunDistanceM)
{
    const uint8_t n = last.count < kMaxSegments ? last.count : kMaxSegments;
    for (uint8_t i = 0u; i < n; ++i) {
        const SegmentSummary &s = last.segments[i];
        if (s.type != static_cast<uint8_t>(seg.type) || s.round != seg.round ||
            s.durationMs == 0u) {
            continue;
        }
        if (seg.type == SegmentType::Run) {
            const uint64_t scaled =
                    static_cast<uint64_t>(s.durationMs) * runDistanceM / lastRunDistanceM;
            return scaled == 0u ? 1u : static_cast<uint32_t>(scaled);
        }
        return s.durationMs;
    }
    return 0u;
}

}  // namespace

void PacePlan::clear()
{
    mActive = false;
    mStatus = Status::Off;
    mCount = 0u;
    for (uint8_t i = 0u; i < kMaxSegments; ++i) {
        mTarget[i] = 0u;
    }
}

PacePlan::Status PacePlan::build(PaceMode mode, const SegmentDesc *plan, uint8_t count,
                                 const ActivitySummary &last, uint32_t targetTotalMs,
                                 uint16_t runDistanceM)
{
    clear();
    if (mode == PaceMode::Off) {
        return mStatus;
    }
    if (!last.valid || last.count == 0u) {
        mStatus = Status::NoLastRace;
        return mStatus;
    }
    if (plan == nullptr || count == 0u || count > kMaxSegments) {
        mStatus = Status::NoMatch;
        return mStatus;
    }

    const uint16_t thisRun = runDistanceM == 0u ? kAssumedRunDistanceM : runDistanceM;
    const uint16_t lastRun = last.runDistanceM == 0u ? kAssumedRunDistanceM : last.runDistanceM;

    // What the last race did for each of this race's segments.
    uint32_t base[kMaxSegments] = {};
    uint8_t matched = 0u;
    uint64_t baseSum = 0u;
    for (uint8_t i = 0u; i < count; ++i) {
        base[i] = lastRaceMs(plan[i], last, thisRun, lastRun);
        if (base[i] != 0u) {
            ++matched;
            baseSum += base[i];
        }
    }

    if (mode == PaceMode::LastRace) {
        if (matched == 0u) {
            mStatus = Status::NoMatch;
            return mStatus;
        }
        for (uint8_t i = 0u; i < count; ++i) {
            mTarget[i] = base[i];
        }
    } else {
        if (targetTotalMs == 0u) {
            mStatus = Status::NoTarget;
            return mStatus;
        }
        // The shape has to cover the whole race, or the targets could not add up to the
        // target time.
        if (matched != count || baseSum == 0u) {
            mStatus = Status::NoMatch;
            return mStatus;
        }
        uint64_t given = 0u;
        for (uint8_t i = 0u; i + 1u < count; ++i) {
            uint64_t t = static_cast<uint64_t>(targetTotalMs) * base[i] / baseSum;
            if (t == 0u) {
                t = 1u;
            }
            mTarget[i] = static_cast<uint32_t>(t);
            given += t;
        }
        // The last takes the remainder, so the targets add up to the target time exactly.
        mTarget[count - 1u] = given < targetTotalMs ? static_cast<uint32_t>(targetTotalMs - given)
                                                    : 1u;
    }

    mCount = count;
    mActive = true;
    mStatus = Status::Ready;
    return mStatus;
}

PacePlan::Live PacePlan::live(const RaceModel &race, uint32_t nowMs) const
{
    Live out;
    if (!mActive) {
        return out;
    }

    int64_t delta = 0;
    bool any = false;

    const uint8_t done = race.recordedCount();
    for (uint8_t i = 0u; i < done && i < mCount; ++i) {
        const SegmentResult *seg = race.recorded(i);
        const uint32_t target = mTarget[i];
        if (seg != nullptr && target != 0u) {
            delta += static_cast<int64_t>(seg->activeMs) - static_cast<int64_t>(target);
            any = true;
        }
    }

    const bool open = race.currentSegment() != nullptr && done < mCount;
    if (open) {
        const uint32_t segMs = race.currentSegmentActiveMs(nowMs);
        const uint32_t target = mTarget[done];
        out.segmentTargetMs = target;
        if (target != 0u) {
            any = true;
            if (segMs > target) {
                delta += static_cast<int64_t>(segMs) - static_cast<int64_t>(target);
            }
        }

        // Where the race ends if the plan is held from here, when every remaining segment has a
        // target to hold.
        bool all = target != 0u;
        uint64_t remaining = target > segMs ? target - segMs : 0u;
        for (uint8_t j = static_cast<uint8_t>(done + 1u); j < mCount; ++j) {
            if (mTarget[j] == 0u) {
                all = false;
                break;
            }
            remaining += mTarget[j];
        }
        if (all) {
            out.hasProjection = true;
            out.projectedMs = static_cast<uint32_t>(race.totalActiveMs(nowMs) + remaining);
        }
    }

    out.hasDelta = any;
    out.deltaMs = static_cast<int32_t>(delta);
    return out;
}

}  // namespace Race
