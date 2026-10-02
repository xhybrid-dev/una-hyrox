#include <gtest/gtest.h>

#include <string>

#include "WorkoutParser.hpp"
#include "WorkoutSummary.hpp"

using namespace Intervals;

namespace
{
Workout parsed(const std::string& json)
{
    Workout w;
    EXPECT_EQ(parseWorkout(json.data(), json.size(), w).error, ParseError::Ok) << json;
    return w;
}
} // namespace

TEST(WorkoutSummary, ExpandsRepeatsIntoTotals)
{
    // 10 min warm-up, 6 x (400 m at pace, 90 s rest), 10 min cool-down.
    const Workout w = parsed(
        "{\"v\":1,\"name\":\"6x400\",\"sport\":\"run\",\"steps\":["
        "{\"type\":\"time\",\"value\":600,\"intensity\":\"warmup\"},"
        "{\"type\":\"dist\",\"value\":400,\"target\":{\"kind\":\"pace\",\"low\":230,\"high\":250}},"
        "{\"type\":\"time\",\"value\":90,\"intensity\":\"rest\"},"
        "{\"type\":\"repeat\",\"from\":1,\"count\":6},"
        "{\"type\":\"time\",\"value\":600,\"intensity\":\"cooldown\"}]}");
    const WorkoutSummary s = summarise(w);
    EXPECT_EQ(s.timeS, 600u + 6u * 90u + 600u);
    EXPECT_EQ(s.distanceM, 6u * 400u);
    EXPECT_EQ(s.stepsRun, 1u + 12u + 1u);
    EXPECT_EQ(s.openSteps, 0u);
    EXPECT_TRUE(s.hasTargets);
}

TEST(WorkoutSummary, CountsOpenStepsAndNoTargets)
{
    const Workout w = parsed(
        "{\"v\":1,\"name\":\"x\",\"sport\":\"run\",\"steps\":["
        "{\"type\":\"open\"},{\"type\":\"time\",\"value\":60},{\"type\":\"repeat\",\"from\":0,\"count\":3}]}");
    const WorkoutSummary s = summarise(w);
    EXPECT_EQ(s.openSteps, 3u);
    EXPECT_EQ(s.timeS, 180u);
    EXPECT_EQ(s.stepsRun, 6u);
    EXPECT_FALSE(s.hasTargets);
}

TEST(WorkoutSummary, BackToBackBlocks)
{
    const Workout w = parsed(
        "{\"v\":1,\"name\":\"x\",\"sport\":\"run\",\"steps\":["
        "{\"type\":\"time\",\"value\":30},{\"type\":\"repeat\",\"from\":0,\"count\":2},"
        "{\"type\":\"dist\",\"value\":100},{\"type\":\"repeat\",\"from\":2,\"count\":5}]}");
    const WorkoutSummary s = summarise(w);
    EXPECT_EQ(s.timeS, 60u);
    EXPECT_EQ(s.distanceM, 500u);
    EXPECT_EQ(s.stepsRun, 7u);
}

TEST(WorkoutSummary, EmptyWorkoutIsAllZero)
{
    const WorkoutSummary s = summarise(Workout {});
    EXPECT_EQ(s.timeS, 0u);
    EXPECT_EQ(s.distanceM, 0u);
    EXPECT_EQ(s.stepsRun, 0u);
}
