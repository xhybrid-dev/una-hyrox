#include <gtest/gtest.h>

#include <string>

#include "WorkoutText.hpp"

using namespace Intervals;

namespace
{
template <typename F>
std::string s(F f)
{
    char buf[40];
    f(buf, sizeof(buf));
    return buf;
}

Step step(DurationKind kind, uint32_t value, StepIntensity intensity = StepIntensity::Active)
{
    return Step { kind, value, intensity, {}, 0 };
}
} // namespace

TEST(WorkoutText, Clock)
{
    EXPECT_EQ(s([](char* b, size_t n) { Text::clock(b, n, 0); }), "0:00");
    EXPECT_EQ(s([](char* b, size_t n) { Text::clock(b, n, 45); }), "0:45");
    EXPECT_EQ(s([](char* b, size_t n) { Text::clock(b, n, 230); }), "3:50");
    EXPECT_EQ(s([](char* b, size_t n) { Text::clock(b, n, 600); }), "10:00");
    EXPECT_EQ(s([](char* b, size_t n) { Text::clock(b, n, 3900); }), "1:05:00");
}

TEST(WorkoutText, PacePerMile)
{
    EXPECT_EQ(Text::secPerMile(240), 386u);   // 4:00 /km is 6:26 /mi
    EXPECT_EQ(Text::secPerMile(300), 483u);   // 5:00 /km is 8:03 /mi
}

TEST(WorkoutText, Distance)
{
    EXPECT_EQ(s([](char* b, size_t n) { Text::distance(b, n, 400, false); }), "400 m");
    EXPECT_EQ(s([](char* b, size_t n) { Text::distance(b, n, 1000, false); }), "1 km");
    EXPECT_EQ(s([](char* b, size_t n) { Text::distance(b, n, 1500, false); }), "1.5 km");
    EXPECT_EQ(s([](char* b, size_t n) { Text::distance(b, n, 21097, false); }), "21.1 km");
    EXPECT_EQ(s([](char* b, size_t n) { Text::distance(b, n, 1609, true); }), "1 mi");
    EXPECT_EQ(s([](char* b, size_t n) { Text::distance(b, n, 400, true); }), "0.25 mi");
    EXPECT_EQ(s([](char* b, size_t n) { Text::distance(b, n, 805, true); }), "0.5 mi");
    EXPECT_EQ(s([](char* b, size_t n) { Text::distance(b, n, 21097, true); }), "13.11 mi");
}

TEST(WorkoutText, Targets)
{
    const bool km = false;
    EXPECT_EQ(s([&](char* b, size_t n) { Text::target(b, n, Target { TargetKind::Pace, 230, 250 }, km); }),
              "3:50-4:10 /km");
    EXPECT_EQ(s([&](char* b, size_t n) { Text::target(b, n, Target { TargetKind::Pace, 240, 250 }, true); }),
              "6:26-6:42 /mi");
    EXPECT_EQ(s([&](char* b, size_t n) { Text::target(b, n, Target { TargetKind::HeartRateZone, 4, 4 }, km); }),
              "Zone 4");
    EXPECT_EQ(s([&](char* b, size_t n) { Text::target(b, n, Target { TargetKind::HeartRateZone, 2, 3 }, km); }),
              "Zones 2-3");
    EXPECT_EQ(s([&](char* b, size_t n) { Text::target(b, n, Target { TargetKind::HeartRateBpm, 140, 150 }, km); }),
              "140-150 bpm");
    EXPECT_EQ(s([&](char* b, size_t n) { Text::target(b, n, Target {}, km); }), "");
}

TEST(WorkoutText, StepLines)
{
    EXPECT_EQ(s([](char* b, size_t n) { Text::stepLine(b, n, step(DurationKind::Distance, 40000), false); }),
              "Run 400 m");
    EXPECT_EQ(s([](char* b, size_t n) {
                  Text::stepLine(b, n, step(DurationKind::Time, 600000, StepIntensity::Warmup), false);
              }),
              "Warm-up 10:00");
    EXPECT_EQ(s([](char* b, size_t n) {
                  Text::stepLine(b, n, step(DurationKind::Time, 90000, StepIntensity::Rest), false);
              }),
              "Rest 1:30");
    EXPECT_EQ(s([](char* b, size_t n) {
                  Text::stepLine(b, n, step(DurationKind::Open, 0, StepIntensity::Cooldown), false);
              }),
              "Cool-down, open");
}

TEST(WorkoutText, Summaries)
{
    WorkoutSummary a;
    a.distanceM = 2400;
    a.timeS     = 2040;   // 34 min
    EXPECT_EQ(s([&](char* b, size_t n) { Text::summary(b, n, a, false); }), "2.4 km, 34 min");

    WorkoutSummary b;
    b.timeS     = 61;   // rounds up: never looks shorter than it is
    b.openSteps = 1;
    EXPECT_EQ(s([&](char* x, size_t n) { Text::summary(x, n, b, false); }), "2 min, 1 open");

    WorkoutSummary c;
    c.openSteps = 3;
    EXPECT_EQ(s([&](char* x, size_t n) { Text::summary(x, n, c, false); }), "open");

    WorkoutSummary d;
    d.timeS = 3900;
    EXPECT_EQ(s([&](char* x, size_t n) { Text::summary(x, n, d, false); }), "1 h 05");

    WorkoutSummary e;
    EXPECT_EQ(s([&](char* x, size_t n) { Text::summary(x, n, e, false); }), "empty");

    WorkoutSummary f;
    f.distanceM = 21097;
    f.timeS     = 5400;
    f.openSteps = 2;
    const std::string all = s([&](char* x, size_t n) { Text::summary(x, n, f, true); });
    EXPECT_EQ(all, "13.11 mi, 1 h 30, 2 open");
    EXPECT_LT(all.size(), 32u) << "fits a list hint";
}

TEST(WorkoutText, ProblemsAreShortAndSpecific)
{
    const ParseError errors[] = { ParseError::TooLarge, ParseError::Syntax,       ParseError::BadVersion,
                                  ParseError::MissingField, ParseError::BadValue, ParseError::TooManySteps,
                                  ParseError::NameTooLong,  ParseError::Invalid };
    for (ParseError e : errors) {
        const std::string p = Text::problem(e, ValidationError::Ok);
        EXPECT_FALSE(p.empty());
        EXPECT_LT(p.size(), 32u) << p;
    }
    EXPECT_STREQ(Text::problem(ParseError::Invalid, ValidationError::NestedRepeat), "repeats inside repeats");
    EXPECT_STREQ(Text::problem(ParseError::Ok, ValidationError::Ok), "");
}

TEST(WorkoutText, ShortBuffersAreCutNotOverrun)
{
    char buf[5];
    const size_t n = Text::target(buf, sizeof(buf), Target { TargetKind::Pace, 230, 250 }, false);
    EXPECT_EQ(n, 4u);
    EXPECT_STREQ(buf, "3:50");
}
