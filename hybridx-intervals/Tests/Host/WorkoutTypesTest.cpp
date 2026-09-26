#include <gtest/gtest.h>

#include <cstring>

#include "WorkoutTypes.hpp"

using namespace Intervals;

TEST(Workout, AddStepAppendsInOrder)
{
    Workout w;
    EXPECT_TRUE(w.addStep(Step { DurationKind::Time, 1000, StepIntensity::Warmup, {}, 0 }));
    EXPECT_TRUE(w.addStep(Step { DurationKind::Distance, 500, StepIntensity::Active, {}, 0 }));
    ASSERT_EQ(w.stepCount, 2u);
    EXPECT_EQ(w.steps[0].intensity, StepIntensity::Warmup);
    EXPECT_EQ(w.steps[1].durationType, DurationKind::Distance);
}

TEST(Workout, AddStepRefusesPastCapacity)
{
    Workout w;
    for (uint8_t i = 0; i < Workout::kMaxSteps; ++i) {
        ASSERT_TRUE(w.addStep(Step {}));
    }
    EXPECT_EQ(w.stepCount, Workout::kMaxSteps);
    EXPECT_FALSE(w.addStep(Step {}));
    EXPECT_EQ(w.stepCount, Workout::kMaxSteps) << "a refused add must not change stepCount";
}

TEST(Workout, DurationKindValuesMatchFitProfile)
{
    // una-sdk/Libs/Header/SDK/Fit/FitProfile.hpp:65-67 -- mirrored exactly so
    // a later lowering to FIT is a plain cast.
    EXPECT_EQ(static_cast<uint8_t>(DurationKind::Time), 0u);
    EXPECT_EQ(static_cast<uint8_t>(DurationKind::Distance), 1u);
    EXPECT_EQ(static_cast<uint8_t>(DurationKind::Open), 5u);
    EXPECT_EQ(static_cast<uint8_t>(DurationKind::RepeatUntilStepsComplete), 6u);
}

TEST(Workout, IntensityValuesMatchFitProfile)
{
    // FitProfile.hpp:56.
    EXPECT_EQ(static_cast<uint8_t>(StepIntensity::Active), 0u);
    EXPECT_EQ(static_cast<uint8_t>(StepIntensity::Rest), 1u);
    EXPECT_EQ(static_cast<uint8_t>(StepIntensity::Warmup), 2u);
    EXPECT_EQ(static_cast<uint8_t>(StepIntensity::Cooldown), 3u);
    EXPECT_EQ(static_cast<uint8_t>(StepIntensity::Invalid), 0xFFu);
}

TEST(Workout, NameFieldSizedAndZeroed)
{
    Workout w;
    EXPECT_EQ(std::strlen(w.name), 0u);
    EXPECT_EQ(sizeof(w.name), static_cast<size_t>(Workout::kNameChars));
}
