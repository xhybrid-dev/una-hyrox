/**
 ******************************************************************************
 * @file    TargetEvaluator.hpp
 * @brief   Is the athlete under, in, or over a step's target, right now?
 *
 * Pure functions/state, no clock, no SDK. The caller assembles a Sample once
 * per tick from SDK::Metric::SpeedSmoother::getPace() (seconds/m -> convert
 * to seconds/km before filling Sample::paceSecPerKm) and HrZones::zoneOf().
 ******************************************************************************
 */

#ifndef INTERVALS_TARGET_EVALUATOR_HPP
#define INTERVALS_TARGET_EVALUATOR_HPP

#include <cstdint>

#include "WorkoutTypes.hpp"

namespace Intervals
{

/// One tick's live measurements. `hasPace`/`hasHr` are false when the
/// underlying sensor/smoother has nothing valid yet (e.g. SpeedSmoother's
/// getPace() returning "---").
struct Sample {
    bool     hasPace      = false;
    uint16_t paceSecPerKm = 0;
    bool     hasHr        = false;
    uint16_t hrBpm         = 0;
    uint8_t  hrZone        = 0;
};

enum class ZoneState : uint8_t {
    NoTarget,   ///< the step's target is TargetKind::Open
    NoSample,   ///< the step has a target, but the needed sample isn't ready
    Under,
    InZone,
    Over,
};

/// Pure classification for one tick. For TargetKind::Pace, `target.low` is
/// the fast bound and `target.high` the slow bound in seconds/km -- pace
/// runs backwards (a bigger number is slower), so the comparison is
/// inverted here rather than asking every caller to remember that.
ZoneState classify(const Target& target, const Sample& sample);

/// Only reports a state change after kDebounceTicks consecutive ticks agree,
/// so one noisy sample can't trigger a haptic cue. Actually playing the
/// haptic is a GUI/later-phase concern; this only exposes the debounced edge.
class CueDebouncer {
public:
    static constexpr uint8_t kDebounceTicks = 3;

    /// Feed one tick's classify() result. Returns true exactly on the tick
    /// the debounced state changes, with `out` set to the new state.
    bool update(ZoneState sample, ZoneState& out);

    ZoneState current() const { return mCommitted; }

private:
    ZoneState mCommitted = ZoneState::NoTarget;
    ZoneState mCandidate = ZoneState::NoTarget;
    uint8_t   mRun       = 0;
};

} // namespace Intervals

#endif // INTERVALS_TARGET_EVALUATOR_HPP
