#include <gtest/gtest.h>

#include "WorkoutTypes.hpp"
#include "WorkoutValidation.hpp"

using namespace Intervals;

namespace
{
Step runStep(DurationKind kind = DurationKind::Time, uint32_t value = 1000)
{
    return Step { kind, value, StepIntensity::Active, {}, 0 };
}

Step repeatMarker(uint32_t firstIndex, uint16_t repeatCount)
{
    return Step { DurationKind::RepeatUntilStepsComplete, firstIndex, StepIntensity::Active, {}, repeatCount };
}
} // namespace

TEST(WorkoutValidation, EmptyWorkoutIsRejected)
{
    Workout w;
    EXPECT_EQ(validate(w), ValidationError::Empty);
}

TEST(WorkoutValidation, PlainStepsWithNoRepeatAreOk)
{
    Workout w;
    w.addStep(runStep());
    w.addStep(runStep(DurationKind::Distance, 500));
    EXPECT_EQ(validate(w), ValidationError::Ok);
}

TEST(WorkoutValidation, ARepeatBlockPointingBackwardsIsOk)
{
    Workout w;
    w.addStep(runStep());               // 0
    w.addStep(runStep());               // 1
    w.addStep(repeatMarker(0, 3));       // 2: repeat steps 0-1, 3 times
    EXPECT_EQ(validate(w), ValidationError::Ok);
}

TEST(WorkoutValidation, RepeatIndexOutOfRangeIsRejected)
{
    Workout w;
    w.addStep(runStep());
    w.addStep(repeatMarker(5, 2));   // index 5 doesn't exist
    EXPECT_EQ(validate(w), ValidationError::RepeatIndexOutOfRange);
}

TEST(WorkoutValidation, RepeatIndexNotBeforeItselfIsRejected)
{
    Workout w;
    w.addStep(runStep());
    w.addStep(repeatMarker(1, 2));   // points at itself
    EXPECT_EQ(validate(w), ValidationError::RepeatIndexNotBefore);
}

TEST(WorkoutValidation, RepeatIndexPointingForwardIsRejected)
{
    Workout w;
    w.addStep(repeatMarker(1, 2));   // index 0, points forward at index 1
    w.addStep(runStep());
    EXPECT_EQ(validate(w), ValidationError::RepeatIndexNotBefore);
}

TEST(WorkoutValidation, RepeatCountZeroIsRejected)
{
    Workout w;
    w.addStep(runStep());
    w.addStep(repeatMarker(0, 0));
    EXPECT_EQ(validate(w), ValidationError::RepeatCountZero);
}

TEST(WorkoutValidation, NestedRepeatIsRejected)
{
    Workout w;
    w.addStep(runStep());               // 0
    w.addStep(runStep());               // 1
    w.addStep(repeatMarker(1, 2));       // 2: an inner repeat over just step 1
    w.addStep(repeatMarker(0, 3));       // 3: an outer repeat over 0-2, which contains a marker
    EXPECT_EQ(validate(w), ValidationError::NestedRepeat);
}

TEST(WorkoutValidation, TwoSiblingRepeatBlocksAreOk)
{
    Workout w;
    w.addStep(runStep());               // 0
    w.addStep(repeatMarker(0, 2));       // 1: block A
    w.addStep(runStep());               // 2
    w.addStep(repeatMarker(2, 3));       // 3: block B, does not overlap block A
    EXPECT_EQ(validate(w), ValidationError::Ok);
}
