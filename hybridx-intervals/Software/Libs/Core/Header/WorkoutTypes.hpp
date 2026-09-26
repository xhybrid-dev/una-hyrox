/**
 ******************************************************************************
 * @file    WorkoutTypes.hpp
 * @brief   A structured workout: steps, targets, repeats (Phase P1).
 *
 * Header-only and SDK-free -- the engine, the evaluator and the host tests
 * all use it, with no dependency on una-sdk or a kernel.
 ******************************************************************************
 */

#ifndef INTERVALS_WORKOUT_TYPES_HPP
#define INTERVALS_WORKOUT_TYPES_HPP

#include <cstdint>

namespace Intervals
{

/// How a step ends. Values match SDK::Fit::WktStepDuration exactly
/// (una-sdk/Libs/Header/SDK/Fit/FitProfile.hpp:65-67), so lowering a Step to
/// a FIT WorkoutStep is a plain cast, not a translation table.
enum class DurationKind : uint8_t {
    Time                     = 0,
    Distance                 = 1,
    Open                     = 5,
    RepeatUntilStepsComplete = 6,
};

/// The step's purpose. Values match SDK::Fit::Intensity exactly
/// (FitProfile.hpp:56).
enum class StepIntensity : uint8_t {
    Active   = 0,
    Rest     = 1,
    Warmup   = 2,
    Cooldown = 3,
    Invalid  = 0xFF,
};

/// A real pace/HR target for the engine to evaluate live. This is an
/// APP-SIDE-ONLY concept: SDK::Fit::WktStepTarget defines only Open = 2
/// (FitProfile.hpp:68) -- there is no FIT-encodable way to record a real
/// target number today. A Workout's FIT export keeps writing
/// WktStepTarget::Open for every step, unchanged
/// (ActivityWriter::addWorkout); this enum never round-trips into FIT.
/// No Power or Cadence kind: the platform cannot sense either
/// (Docs/ExternalSensors.md:14-30).
enum class TargetKind : uint8_t {
    Open,
    Pace,           ///< seconds per km; see Target::low/high
    HeartRateZone,  ///< a zone index from HrZones::zoneOf
    HeartRateBpm,
};

/// A band around the target effort. For every kind, `low` is the slow/under
/// bound and `high` is the fast/over bound in that kind's own units (pace's
/// direction is inverted internally by TargetEvaluator, which is the one
/// place that has to know pace runs backwards).
struct Target {
    TargetKind kind = TargetKind::Open;
    uint16_t   low  = 0;
    uint16_t   high = 0;
};

/// One step. For DurationKind::RepeatUntilStepsComplete, this step is a
/// control-flow marker rather than a runnable one: durationValue is the
/// index of the first step to repeat from, and repeatCount is the number of
/// iterations -- exactly ActivityWriter::WorkoutStepData's convention
/// (una-sdk/Examples/Apps/Running/Software/Libs/Header/ActivityWriter.hpp:98-103),
/// so a later lowering to FIT is a field copy.
struct Step {
    DurationKind  durationType  = DurationKind::Open;
    uint32_t      durationValue = 0;   ///< Time: ms; Distance: cm; Repeat: first-step index
    StepIntensity intensity     = StepIntensity::Active;
    Target        target       {};
    uint16_t      repeatCount   = 0;   ///< RepeatUntilStepsComplete only
};

/// The two sports this project targets (brief: running and cycling, not
/// HYROX). A local, minimal enum -- Core stays SDK-free, so this is not
/// SDK::Fit::Sport; the FIT-writing boundary maps it later.
enum class Sport : uint8_t {
    Running,
    Cycling,
};

/// A complete workout. kMaxSteps/kNameChars are placeholder bounds -- there
/// is no spec yet for how large a workout this needs to hold; ask Jon
/// (docs/NOTES.md P1, open questions).
struct Workout {
    static constexpr uint8_t kMaxSteps  = 20;
    static constexpr uint8_t kNameChars = 32;

    char    name[kNameChars] = {};
    Sport   sport            = Sport::Running;
    Step    steps[kMaxSteps] {};
    uint8_t stepCount        = 0;

    /// Appends a step; false and no change if the workout is already full.
    bool addStep(const Step& step)
    {
        if (stepCount >= kMaxSteps) {
            return false;
        }
        steps[stepCount++] = step;
        return true;
    }
};

} // namespace Intervals

#endif // INTERVALS_WORKOUT_TYPES_HPP
