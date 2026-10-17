/**
 ******************************************************************************
 * @file    PacePlan.hpp
 * @date    15-10-2026
 * @author  HybridX
 * @brief   A time target for each segment, and how the race stands against it.
 *          Pure C++, no SDK dependencies.
 ******************************************************************************
 *
 * The watch has no GPS in a race (brief 6.1), so a plan is only ever a time
 * per segment. Nothing here knows HYROX pacing: no share table has been
 * supplied (brief 7.6, decision D6) and none is invented. Both modes take their
 * numbers from the athlete's own last saved race.
 *
 *  - LastRace: each segment's target is the time it took in the last race.
 *  - Target:   the last race's *shape* (each segment's share of its total),
 *              scaled so the race finishes in the target time. A table of
 *              shares from real results can later replace the shape in
 *              shapeFromLastRace() without touching anything else.
 *
 * Segments are matched by (type, round), so a Half can use a Full as its plan.
 * A run's time is scaled by this race's run length over the last race's, so a
 * 500 m sim measures against a 1 km race in proportion. Stations and Roxzone
 * are never scaled.
 *
 ******************************************************************************
 */
#ifndef RACE_PACE_PLAN_HPP
#define RACE_PACE_PLAN_HPP

#include <cstdint>

#include "ActivitySummary.hpp"
#include "RaceData.hpp"
#include "RaceModel.hpp"

namespace Race
{

class PacePlan
{
public:
    /// Why a plan is, or is not, in force.
    enum class Status : uint8_t
    {
        Off = 0,     ///< The mode is Off
        Ready,       ///< Targets are set
        NoLastRace,  ///< There is no saved race to measure against
        NoMatch,     ///< LastRace: nothing in it matches this race. Target: it
                     ///< does not cover every segment of this race
        NoTarget,    ///< Target mode, but no target time was given
    };

    /// How the race stands, right now.
    struct Live
    {
        bool     hasDelta = false;        ///< False until a segment has a target
        int32_t  deltaMs = 0;             ///< Positive is behind plan
        bool     hasProjection = false;   ///< False when any remaining segment has no target
        uint32_t projectedMs = 0;         ///< Finish time if the rest of the plan is held
        uint32_t segmentTargetMs = 0;     ///< The open segment's target, 0 = none
    };

    /// A run length of 0 (an older summary, or none given) means the race's.
    static constexpr uint16_t kAssumedRunDistanceM = kRunDistanceDefaultM;

    /**
     * @brief Set the targets for a race.
     *
     * @param mode          What to build.
     * @param plan          The race's segments (RaceModel::buildTemplate()).
     * @param count         Number of segments in @p plan.
     * @param last          The last saved race.
     * @param targetTotalMs Target finish, active time, Target mode only.
     * @param runDistanceM  This race's run length, metres.
     * @retval Why the plan is, or is not, in force. Anything but Ready leaves
     *         the plan inactive, so the race simply runs without one.
     */
    Status build(PaceMode mode, const SegmentDesc *plan, uint8_t count,
                 const ActivitySummary &last, uint32_t targetTotalMs, uint16_t runDistanceM);

    /// Forget the plan.
    void clear();

    /// @retval True when targets are in force.
    bool active() const { return mActive; }

    /// @retval Why the plan is, or is not, in force.
    Status status() const { return mStatus; }

    /// @retval Segments the plan covers.
    uint8_t count() const { return mCount; }

    /**
     * @brief A segment's target.
     *
     * @param index Zero-based segment index.
     * @retval Milliseconds, or 0 for no target (and for an index out of range).
     */
    uint32_t targetMs(uint8_t index) const
    {
        return (mActive && index < mCount) ? mTarget[index] : 0u;
    }

    /**
     * @brief How the race stands against the plan.
     *
     * Computed from the model's closed segments each call, so an undone split,
     * a pause, or a split the lockout ignored needs no handling of its own.
     *
     * Delta counts every closed segment that has a target (actual minus target)
     * plus the time the open segment has run past its own target. It never goes
     * negative within a segment: being ahead is credited at the split, not
     * guessed halfway through.
     *
     * The projection is the active time so far plus the open segment's
     * remaining target plus every later target: where the race ends if the
     * plan is held from here. It is withheld when any remaining segment has no
     * target, rather than understated.
     *
     * @param race  The race.
     * @param nowMs Monotonic tick.
     */
    Live live(const RaceModel &race, uint32_t nowMs) const;

private:
    bool     mActive = false;
    Status   mStatus = Status::Off;
    uint8_t  mCount = 0u;
    uint32_t mTarget[kMaxSegments] {};
};

}  // namespace Race

#endif  // RACE_PACE_PLAN_HPP
