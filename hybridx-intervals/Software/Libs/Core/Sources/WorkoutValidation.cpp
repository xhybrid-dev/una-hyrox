#include "WorkoutValidation.hpp"

namespace Intervals
{

ValidationError validate(const Workout& workout)
{
    if (workout.stepCount == 0) {
        return ValidationError::Empty;
    }

    for (uint8_t i = 0; i < workout.stepCount; ++i) {
        const Step& step = workout.steps[i];
        if (step.durationType != DurationKind::RepeatUntilStepsComplete) {
            continue;
        }

        const uint32_t firstIndex = step.durationValue;
        if (firstIndex >= workout.stepCount) {
            return ValidationError::RepeatIndexOutOfRange;
        }
        if (firstIndex >= i) {
            return ValidationError::RepeatIndexNotBefore;
        }
        if (step.repeatCount == 0) {
            return ValidationError::RepeatCountZero;
        }

        for (uint32_t j = firstIndex; j < i; ++j) {
            if (workout.steps[j].durationType == DurationKind::RepeatUntilStepsComplete) {
                return ValidationError::NestedRepeat;
            }
        }
    }

    return ValidationError::Ok;
}

} // namespace Intervals
