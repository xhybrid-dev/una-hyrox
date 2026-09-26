#include "WorkoutEngine.hpp"

namespace Intervals
{

void WorkoutEngine::start(const Workout& workout, uint32_t nowMs, uint32_t distanceCm, Events& events)
{
    mWorkout        = &workout;
    mActiveMarker   = kNoActiveMarker;
    mIterationsLeft = 0;
    mStarted        = true;
    mCompleted      = (workout.stepCount == 0);
    mStepIndex      = 0;

    if (mCompleted) {
        events.add(EventKind::WorkoutCompleted);
        return;
    }
    // A validated workout never has a repeat marker at index 0 (its
    // firstIndex would have to be < 0), so step 0 is always runnable.
    enterStep(0, nowMs, distanceCm, events);
}

void WorkoutEngine::enterStep(uint8_t index, uint32_t nowMs, uint32_t distanceCm, Events& events)
{
    mStepIndex           = index;
    mStepStartMs         = nowMs;
    mStepStartDistanceCm = distanceCm;
    events.add(EventKind::StepStarted, mStepIndex);
}

void WorkoutEngine::advancePastCompletedStep(uint32_t nowMs, uint32_t distanceCm, Events& events)
{
    uint8_t next = mStepIndex + 1;

    while (next < mWorkout->stepCount &&
           mWorkout->steps[next].durationType == DurationKind::RepeatUntilStepsComplete) {
        const Step& marker = mWorkout->steps[next];

        if (mActiveMarker != next) {
            mActiveMarker   = next;
            mIterationsLeft = marker.repeatCount;
        }
        --mIterationsLeft;

        if (mIterationsLeft > 0) {
            events.add(EventKind::RepeatBlockLooped, next, mIterationsLeft);
            next = static_cast<uint8_t>(marker.durationValue);
        } else {
            events.add(EventKind::RepeatBlockCompleted, next);
            mActiveMarker = kNoActiveMarker;
            ++next;
        }
    }

    if (next >= mWorkout->stepCount) {
        mCompleted = true;
        events.add(EventKind::WorkoutCompleted);
        return;
    }
    enterStep(next, nowMs, distanceCm, events);
}

void WorkoutEngine::tick(uint32_t nowMs, uint32_t distanceCm, Events& events)
{
    if (mCompleted || !mStarted) {
        return;
    }

    const Step& step = mWorkout->steps[mStepIndex];
    bool        done = false;
    switch (step.durationType) {
        case DurationKind::Time:
            done = (nowMs - mStepStartMs) >= step.durationValue;
            break;
        case DurationKind::Distance:
            done = (distanceCm - mStepStartDistanceCm) >= step.durationValue;
            break;
        case DurationKind::Open:
        case DurationKind::RepeatUntilStepsComplete:
            done = false;
            break;
    }

    if (done) {
        events.add(EventKind::StepCompleted, mStepIndex);
        advancePastCompletedStep(nowMs, distanceCm, events);
    }
}

void WorkoutEngine::advanceManually(uint32_t nowMs, uint32_t distanceCm, Events& events)
{
    if (mCompleted || !mStarted) {
        return;
    }
    events.add(EventKind::StepCompleted, mStepIndex);
    advancePastCompletedStep(nowMs, distanceCm, events);
}

const Step* WorkoutEngine::currentStep() const
{
    if (!mStarted || mCompleted || mWorkout == nullptr) {
        return nullptr;
    }
    return &mWorkout->steps[mStepIndex];
}

uint32_t WorkoutEngine::stepElapsedMs(uint32_t nowMs) const
{
    if (!mStarted || mCompleted) {
        return 0;
    }
    return nowMs - mStepStartMs;
}

uint32_t WorkoutEngine::stepRemainingMs(uint32_t nowMs) const
{
    const Step* step = currentStep();
    if (step == nullptr || step->durationType != DurationKind::Time) {
        return 0;
    }
    const uint32_t elapsed = stepElapsedMs(nowMs);
    return (elapsed >= step->durationValue) ? 0 : (step->durationValue - elapsed);
}

uint16_t WorkoutEngine::iterationsRemaining() const
{
    return (mActiveMarker == kNoActiveMarker) ? 0 : mIterationsLeft;
}

} // namespace Intervals
