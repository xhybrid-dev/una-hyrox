#include "TargetEvaluator.hpp"

namespace Intervals
{

ZoneState classify(const Target& target, const Sample& sample)
{
    switch (target.kind) {
        case TargetKind::Open:
            return ZoneState::NoTarget;

        case TargetKind::Pace: {
            if (!sample.hasPace) {
                return ZoneState::NoSample;
            }
            // low = fast bound, high = slow bound; a bigger sec/km is slower.
            if (sample.paceSecPerKm > target.high) {
                return ZoneState::Under;   // slower than the slow bound: under effort
            }
            if (sample.paceSecPerKm < target.low) {
                return ZoneState::Over;    // faster than the fast bound: over effort
            }
            return ZoneState::InZone;
        }

        case TargetKind::HeartRateZone: {
            if (!sample.hasHr) {
                return ZoneState::NoSample;
            }
            if (sample.hrZone < target.low) {
                return ZoneState::Under;
            }
            if (sample.hrZone > target.high) {
                return ZoneState::Over;
            }
            return ZoneState::InZone;
        }

        case TargetKind::HeartRateBpm: {
            if (!sample.hasHr) {
                return ZoneState::NoSample;
            }
            if (sample.hrBpm < target.low) {
                return ZoneState::Under;
            }
            if (sample.hrBpm > target.high) {
                return ZoneState::Over;
            }
            return ZoneState::InZone;
        }
    }
    return ZoneState::NoTarget;
}

bool CueDebouncer::update(ZoneState sample, ZoneState& out)
{
    if (sample == mCommitted) {
        mCandidate = sample;
        mRun       = 0;
        return false;
    }

    if (sample == mCandidate) {
        ++mRun;
    } else {
        mCandidate = sample;
        mRun       = 1;
    }

    if (mRun >= kDebounceTicks) {
        mCommitted = mCandidate;
        mRun       = 0;
        out        = mCommitted;
        return true;
    }
    return false;
}

} // namespace Intervals
