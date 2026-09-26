/**
 ******************************************************************************
 * @file    WorkoutEngine.hpp
 * @brief   The step-sequencing cursor: which step is running, and when it
 *          hands off to the next one.
 *
 * No internal clock or distance counter -- every call takes `now`/distance
 * as an argument (Race/Streak's convention), so a wrapping millisecond clock
 * needs no special case: elapsed time is always unsigned subtraction.
 *
 * Deliberately doesn't touch TargetEvaluator: sequencing (this file) and
 * target evaluation are separate, independently testable concerns, both
 * driven by whatever app loop calls them once per tick.
 *
 * Precondition: the Workout passed to start() has already passed
 * WorkoutValidation::validate() as Ok. This is not re-checked here.
 ******************************************************************************
 */

#ifndef INTERVALS_WORKOUT_ENGINE_HPP
#define INTERVALS_WORKOUT_ENGINE_HPP

#include <cstdint>

#include "WorkoutEvents.hpp"
#include "WorkoutTypes.hpp"

namespace Intervals
{

class WorkoutEngine {
public:
    static constexpr uint8_t kNoActiveMarker = 0xFF;

    /// Begins at step 0. `workout` must outlive the engine.
    void start(const Workout& workout, uint32_t nowMs, uint32_t distanceCm, Events& events);

    /// Advances Time/Distance steps that have reached their threshold, and
    /// resolves any RepeatUntilStepsComplete marker reached along the way.
    /// A no-op once completed() is true.
    void tick(uint32_t nowMs, uint32_t distanceCm, Events& events);

    /// Ends the current step early -- the only way an Open step advances.
    /// A no-op once completed() is true.
    void advanceManually(uint32_t nowMs, uint32_t distanceCm, Events& events);

    bool           completed() const { return mCompleted; }
    uint8_t        stepIndex() const { return mStepIndex; }
    const Step*    currentStep() const;
    uint32_t       stepElapsedMs(uint32_t nowMs) const;
    /// 0 for anything other than a Time step (there is no fixed end to be
    /// "remaining" from otherwise).
    uint32_t       stepRemainingMs(uint32_t nowMs) const;
    /// 0 when no repeat range is active.
    uint16_t       iterationsRemaining() const;

private:
    void enterStep(uint8_t index, uint32_t nowMs, uint32_t distanceCm, Events& events);
    void advancePastCompletedStep(uint32_t nowMs, uint32_t distanceCm, Events& events);

    const Workout* mWorkout             = nullptr;
    uint8_t        mStepIndex           = 0;
    uint32_t       mStepStartMs         = 0;
    uint32_t       mStepStartDistanceCm = 0;
    uint8_t        mActiveMarker        = kNoActiveMarker;
    uint16_t       mIterationsLeft      = 0;
    bool           mStarted             = false;
    bool           mCompleted           = false;
};

} // namespace Intervals

#endif // INTERVALS_WORKOUT_ENGINE_HPP
