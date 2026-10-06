#include "WorkoutRunner.hpp"

namespace Intervals
{

void WorkoutRunner::absorb(const Events& events, uint32_t activeMs, TickResult& out)
{
    for (uint8_t i = 0; i < events.count; ++i) {
        const Event& e = events.items[i];
        switch (e.kind) {
            case EventKind::StepCompleted:
                out.stepEnded = true;
                out.endedStep = e.a;
                break;
            case EventKind::StepStarted:
                out.stepStarted = true;
                mStepStartMs    = activeMs;
                mDebouncer      = CueDebouncer {};
                mLive           = ZoneState::NoTarget;
                mOutOfZone      = false;
                break;
            case EventKind::WorkoutCompleted:
                out.completed = true;
                break;
            case EventKind::RepeatBlockLooped:
            case EventKind::RepeatBlockCompleted:
            case EventKind::ZoneChanged:
                break;
        }
    }
}

WorkoutRunner::TickResult WorkoutRunner::start(const Workout& workout, uint32_t activeMs, uint32_t distanceCm)
{
    TickResult out;
    mWorkout   = &workout;
    mEngine    = WorkoutEngine {};
    mLastCueMs = activeMs;
    Events events;
    mEngine.start(workout, activeMs, distanceCm, events);
    mStepStartCm = distanceCm;
    absorb(events, activeMs, out);
    return out;
}

WorkoutRunner::TickResult WorkoutRunner::tick(uint32_t activeMs, uint32_t distanceCm, const Sample& sample)
{
    TickResult out;
    if (mWorkout == nullptr || mEngine.completed()) {
        return out;
    }

    Events events;
    mEngine.tick(activeMs, distanceCm, events);
    absorb(events, activeMs, out);
    if (out.stepStarted) {
        mStepStartCm = distanceCm;
    }
    if (mEngine.completed()) {
        return out;
    }

    const Step* step = mEngine.currentStep();
    mLive            = classify(step->target, sample);
    if (static_cast<uint32_t>(activeMs - mStepStartMs) < kSettleMs) {
        return out;   // settling: the readings still describe the last step's effort
    }

    ZoneState changed = ZoneState::NoTarget;
    if (mDebouncer.update(mLive, changed)) {
        const bool isOut = (changed == ZoneState::Under || changed == ZoneState::Over);
        mOutOfZone      = isOut;
        if (isOut) {
            out.cue      = true;
            out.cueState = changed;
            mLastCueMs   = activeMs;
        }
    } else if (mOutOfZone && static_cast<uint32_t>(activeMs - mLastCueMs) >= kRemindMs) {
        out.cue      = true;
        out.cueState = mDebouncer.current();
        out.reminder = true;
        mLastCueMs   = activeMs;
    }
    return out;
}

WorkoutRunner::TickResult WorkoutRunner::advance(uint32_t activeMs, uint32_t distanceCm)
{
    TickResult out;
    if (mWorkout == nullptr || mEngine.completed()) {
        return out;
    }
    Events events;
    mEngine.advanceManually(activeMs, distanceCm, events);
    absorb(events, activeMs, out);
    if (out.stepStarted) {
        mStepStartCm = distanceCm;
    }
    return out;
}

void WorkoutRunner::stop()
{
    mWorkout   = nullptr;
    mEngine    = WorkoutEngine {};
    mDebouncer = CueDebouncer {};
    mLive      = ZoneState::NoTarget;
    mOutOfZone = false;
}

WorkoutRunner::View WorkoutRunner::view(uint32_t activeMs, uint32_t distanceCm) const
{
    View v;
    v.completed = mWorkout != nullptr && mEngine.completed();
    const Step* step = mWorkout != nullptr ? mEngine.currentStep() : nullptr;
    if (step == nullptr) {
        return v;
    }
    v.running   = true;
    v.stepIndex = mEngine.stepIndex();
    v.step      = step;
    v.nextIndex = mEngine.nextStepIndex();
    v.elapsedMs = mEngine.stepElapsedMs(activeMs);
    v.remainingMs = mEngine.stepRemainingMs(activeMs);
    if (step->durationType == DurationKind::Distance) {
        const uint32_t done = distanceCm - mStepStartCm;
        v.remainingCm       = done >= step->durationValue ? 0 : step->durationValue - done;
    }
    mEngine.repeatPosition(v.pass, v.passes);
    v.zone     = mDebouncer.current();
    v.zoneLive = mLive;
    v.settling = static_cast<uint32_t>(activeMs - mStepStartMs) < kSettleMs;
    return v;
}

} // namespace Intervals
