/**
 ******************************************************************************
 * @file    WorkoutValidation.hpp
 * @brief   Is a Workout well-formed enough for WorkoutEngine to run?
 ******************************************************************************
 */

#ifndef INTERVALS_WORKOUT_VALIDATION_HPP
#define INTERVALS_WORKOUT_VALIDATION_HPP

#include "WorkoutTypes.hpp"

namespace Intervals
{

enum class ValidationError : uint8_t {
    Ok,
    Empty,                  ///< stepCount == 0
    RepeatIndexOutOfRange,  ///< durationValue is not a valid step index
    RepeatIndexNotBefore,   ///< durationValue must be strictly before the repeat step
    RepeatCountZero,        ///< a repeat step with repeatCount == 0
    NestedRepeat,           ///< a repeat range containing another repeat marker
};

/// Pure, bounded check -- no state, safe to call from a host test or the
/// watch. Only P1's shape is checked here: WorkoutEngine supports one
/// active repeat range at a time (docs/NOTES.md P1, open questions), so a
/// repeat marker whose [firstStepIndex, thisIndex) range contains another
/// repeat marker is rejected as nesting rather than silently mishandled.
ValidationError validate(const Workout& workout);

} // namespace Intervals

#endif // INTERVALS_WORKOUT_VALIDATION_HPP
