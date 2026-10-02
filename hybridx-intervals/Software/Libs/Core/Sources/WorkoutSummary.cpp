#include "WorkoutSummary.hpp"

namespace Intervals
{

namespace
{
void addStep(WorkoutSummary& sum, const Step& step, uint32_t times)
{
    switch (step.durationType) {
        case DurationKind::Time:
            sum.timeS += (step.durationValue / 1000u) * times;
            break;
        case DurationKind::Distance:
            sum.distanceM += (step.durationValue / 100u) * times;
            break;
        case DurationKind::Open:
            sum.openSteps = static_cast<uint16_t>(sum.openSteps + times);
            break;
        case DurationKind::RepeatUntilStepsComplete:
            return;
    }
    sum.stepsRun = static_cast<uint16_t>(sum.stepsRun + times);
    if (step.target.kind != TargetKind::Open) {
        sum.hasTargets = true;
    }
}
} // namespace

WorkoutSummary summarise(const Workout& workout)
{
    WorkoutSummary sum;
    for (uint8_t i = 0; i < workout.stepCount && i < Workout::kMaxSteps; ++i) {
        const Step& step = workout.steps[i];
        if (step.durationType != DurationKind::RepeatUntilStepsComplete) {
            addStep(sum, step, 1);
            continue;
        }
        // The block before this marker has been counted once on the way
        // here; count it (repeatCount - 1) more times.
        const uint16_t extra = step.repeatCount > 0 ? static_cast<uint16_t>(step.repeatCount - 1) : 0;
        for (uint32_t j = step.durationValue; j < i; ++j) {
            addStep(sum, workout.steps[j], extra);
        }
    }
    return sum;
}

} // namespace Intervals
