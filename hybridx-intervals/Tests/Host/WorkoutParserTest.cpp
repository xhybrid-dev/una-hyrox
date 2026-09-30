#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "WorkoutEngine.hpp"
#include "TargetEvaluator.hpp"
#include "WorkoutParser.hpp"

using namespace Intervals;

namespace
{

ParseResult parse(const std::string& s, Workout& w)
{
    return parseWorkout(s.data(), s.size(), w);
}

ParseError errorOf(const std::string& s)
{
    Workout w;
    return parse(s, w).error;
}

std::string readFixture(const char* name)
{
    std::string path = std::string(FIXTURE_DIR) + "/" + name;
    FILE*       f    = std::fopen(path.c_str(), "rb");
    EXPECT_NE(f, nullptr) << path;
    std::string s;
    if (f != nullptr) {
        char buf[512];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) {
            s.append(buf, n);
        }
        std::fclose(f);
    }
    return s;
}

// A minimal valid file with a hole for one step's JSON.
std::string withStep(const std::string& step)
{
    return "{\"v\":1,\"name\":\"t\",\"sport\":\"run\",\"steps\":[" + step + "]}";
}

} // namespace

TEST(WorkoutParser, ParsesTheWorkedExample)
{
    Workout     w;
    const auto  s   = readFixture("6x400_pace.json");
    const auto  res = parse(s, w);
    ASSERT_EQ(res.error, ParseError::Ok);

    EXPECT_STREQ(w.name, "6 x 400 m");
    EXPECT_EQ(w.sport, Sport::Running);
    ASSERT_EQ(w.stepCount, 5);

    EXPECT_EQ(w.steps[0].durationType, DurationKind::Time);
    EXPECT_EQ(w.steps[0].durationValue, 600000u);            // 600 s in ms
    EXPECT_EQ(w.steps[0].intensity, StepIntensity::Warmup);
    EXPECT_EQ(w.steps[0].target.kind, TargetKind::Open);

    EXPECT_EQ(w.steps[1].durationType, DurationKind::Distance);
    EXPECT_EQ(w.steps[1].durationValue, 40000u);             // 400 m in cm
    EXPECT_EQ(w.steps[1].target.kind, TargetKind::Pace);
    EXPECT_EQ(w.steps[1].target.low, 230);
    EXPECT_EQ(w.steps[1].target.high, 250);

    EXPECT_EQ(w.steps[2].intensity, StepIntensity::Rest);
    EXPECT_EQ(w.steps[3].durationType, DurationKind::RepeatUntilStepsComplete);
    EXPECT_EQ(w.steps[3].durationValue, 1u);
    EXPECT_EQ(w.steps[3].repeatCount, 6);
    EXPECT_EQ(w.steps[4].intensity, StepIntensity::Cooldown);
}

TEST(WorkoutParser, ParsedWorkoutRunsToCompletionInTheEngine)
{
    Workout w;
    ASSERT_EQ(parse(readFixture("6x400_pace.json"), w).error, ParseError::Ok);

    WorkoutEngine engine;
    Events        ev;
    uint32_t      now  = 1000;
    uint32_t      dist = 0;
    engine.start(w, now, dist, ev);

    // Drive a second at a time: 4 m/s is fast enough to finish every distance step.
    for (int i = 0; i < 20000 && !engine.completed(); ++i) {
        now += 1000;
        dist += 400;
        Events e;
        engine.tick(now, dist, e);
    }
    EXPECT_TRUE(engine.completed());
}

TEST(WorkoutParser, ParsesABikeWorkoutWithAnOpenStepAndHrZone)
{
    Workout w;
    ASSERT_EQ(parse(readFixture("hr_zone2_bike.json"), w).error, ParseError::Ok);
    EXPECT_EQ(w.sport, Sport::Cycling);
    ASSERT_EQ(w.stepCount, 2);
    EXPECT_EQ(w.steps[0].target.kind, TargetKind::HeartRateZone);
    EXPECT_EQ(w.steps[0].target.low, 2);
    EXPECT_EQ(w.steps[1].durationType, DurationKind::Open);
    EXPECT_EQ(w.steps[1].intensity, StepIntensity::Cooldown);
}

TEST(WorkoutParser, KeyOrderWhitespaceAndBomDoNotMatter)
{
    const std::string s =
        "\xEF\xBB\xBF \n\t{ \"steps\" : [ { \"value\" : 60 , \"type\" : \"time\" } ],\r\n"
        "\"sport\":\"run\", \"name\" : \"x\" , \"v\" : 1 }  \n";
    Workout w;
    ASSERT_EQ(parse(s, w).error, ParseError::Ok);
    EXPECT_EQ(w.steps[0].durationValue, 60000u);
    EXPECT_EQ(w.steps[0].intensity, StepIntensity::Active);   // absent means active
}

TEST(WorkoutParser, UnknownKeysOfAnyShapeAreSkipped)
{
    const std::string s =
        "{\"v\":1,\"extra\":{\"a\":[1,2,{\"b\":null}],\"c\":\"s\\\"q\"},\"name\":\"x\",\"sport\":\"run\","
        "\"note\":true,\"n\":-1.5e3,\"steps\":[{\"type\":\"open\",\"colour\":[1,2],\"target\":{\"kind\":\"open\",\"z\":{}}}]}";
    Workout w;
    ASSERT_EQ(parse(s, w).error, ParseError::Ok);
    EXPECT_EQ(w.stepCount, 1);
}

TEST(WorkoutParser, NameKeepsEscapesAndUtf8)
{
    Workout w;
    ASSERT_EQ(parse("{\"v\":1,\"name\":\"5\\\" \\\\ caf\xC3\xA9\",\"sport\":\"run\",\"steps\":[{\"type\":\"open\"}]}", w).error,
              ParseError::Ok);
    EXPECT_STREQ(w.name, "5\" \\ caf\xC3\xA9");
}

TEST(WorkoutParser, EmptyAndNonJsonInputIsSyntax)
{
    EXPECT_EQ(errorOf(""), ParseError::Syntax);
    EXPECT_EQ(errorOf("   "), ParseError::Syntax);
    EXPECT_EQ(errorOf("[]"), ParseError::Syntax);
    EXPECT_EQ(errorOf("not json"), ParseError::Syntax);
    Workout w;
    EXPECT_EQ(parseWorkout(nullptr, 5, w).error, ParseError::Syntax);
}

TEST(WorkoutParser, TrailingGarbageIsSyntax)
{
    EXPECT_EQ(errorOf(withStep("{\"type\":\"open\"}") + " x"), ParseError::Syntax);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"open\"}") + "{}"), ParseError::Syntax);
}

TEST(WorkoutParser, TooLargeIsReportedWithoutReading)
{
    std::string s(kMaxWorkoutFileBytes + 1, ' ');
    EXPECT_EQ(errorOf(s), ParseError::TooLarge);
    // Exactly the limit is not too large (it is only empty space here).
    EXPECT_EQ(errorOf(std::string(kMaxWorkoutFileBytes, ' ')), ParseError::Syntax);
}

TEST(WorkoutParser, WrongVersionIsBadVersion)
{
    EXPECT_EQ(errorOf("{\"v\":2,\"name\":\"x\",\"sport\":\"run\",\"steps\":[{\"type\":\"open\"}]}"), ParseError::BadVersion);
    EXPECT_EQ(errorOf("{\"v\":0,\"name\":\"x\"}"), ParseError::BadVersion);
    // A future version may carry things this parser cannot read; v first wins.
    EXPECT_EQ(errorOf("{\"v\":3,\"steps\":[{\"type\":\"warp\"}]}"), ParseError::BadVersion);
    EXPECT_EQ(errorOf("{\"v\":99999999999,\"name\":\"x\"}"), ParseError::BadVersion);
}

TEST(WorkoutParser, MissingTopLevelFields)
{
    EXPECT_EQ(errorOf("{}"), ParseError::MissingField);
    EXPECT_EQ(errorOf("{\"name\":\"x\",\"sport\":\"run\",\"steps\":[{\"type\":\"open\"}]}"), ParseError::MissingField);
    EXPECT_EQ(errorOf("{\"v\":1,\"sport\":\"run\",\"steps\":[{\"type\":\"open\"}]}"), ParseError::MissingField);
    EXPECT_EQ(errorOf("{\"v\":1,\"name\":\"x\",\"steps\":[{\"type\":\"open\"}]}"), ParseError::MissingField);
    EXPECT_EQ(errorOf("{\"v\":1,\"name\":\"x\",\"sport\":\"run\"}"), ParseError::MissingField);
}

TEST(WorkoutParser, MissingStepFields)
{
    EXPECT_EQ(errorOf(withStep("{}")), ParseError::MissingField);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\"}")), ParseError::MissingField);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"dist\"}")), ParseError::MissingField);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"repeat\",\"count\":2}")), ParseError::MissingField);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"repeat\",\"from\":0}")), ParseError::MissingField);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"open\",\"target\":{}}")), ParseError::MissingField);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"open\",\"target\":{\"kind\":\"pace\",\"low\":200}}")), ParseError::MissingField);
}

TEST(WorkoutParser, UnknownNamesAreBadValueNotADefault)
{
    EXPECT_EQ(errorOf(withStep("{\"type\":\"warp\"}")), ParseError::BadValue);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"open\",\"intensity\":\"hard\"}")), ParseError::BadValue);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"open\",\"target\":{\"kind\":\"power\",\"low\":1,\"high\":2}}")), ParseError::BadValue);
    EXPECT_EQ(errorOf("{\"v\":1,\"name\":\"x\",\"sport\":\"swim\",\"steps\":[{\"type\":\"open\"}]}"), ParseError::BadValue);
    // A word too long to be any name is also unknown.
    EXPECT_EQ(errorOf(withStep("{\"type\":\"a_very_long_word_indeed\"}")), ParseError::BadValue);
}

TEST(WorkoutParser, NumbersAreStrictIntegersInRange)
{
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\",\"value\":0}")), ParseError::BadValue);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\",\"value\":86401}")), ParseError::BadValue);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\",\"value\":86400}")), ParseError::Ok);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"dist\",\"value\":100001}")), ParseError::BadValue);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"dist\",\"value\":100000}")), ParseError::Ok);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\",\"value\":-5}")), ParseError::BadValue);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\",\"value\":1.5}")), ParseError::BadValue);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\",\"value\":1e2}")), ParseError::BadValue);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\",\"value\":\"60\"}")), ParseError::BadValue);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\",\"value\":007}")), ParseError::Syntax);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\",\"value\":99999999999999999999}")), ParseError::BadValue);
    EXPECT_EQ(errorOf(withStep("{\"type\":\"time\",\"value\":}")), ParseError::Syntax);
}

TEST(WorkoutParser, TargetRangesAndOrdering)
{
    auto t = [](const char* kind, int lo, int hi) {
        return withStep(std::string("{\"type\":\"open\",\"target\":{\"kind\":\"") + kind + "\",\"low\":" + std::to_string(lo) +
                        ",\"high\":" + std::to_string(hi) + "}}");
    };
    EXPECT_EQ(errorOf(t("pace", 230, 250)), ParseError::Ok);
    EXPECT_EQ(errorOf(t("pace", 250, 230)), ParseError::BadValue);   // low is the faster bound
    EXPECT_EQ(errorOf(t("pace", 0, 250)), ParseError::BadValue);
    EXPECT_EQ(errorOf(t("pace", 100, 3601)), ParseError::BadValue);
    EXPECT_EQ(errorOf(t("hrzone", 1, 7)), ParseError::Ok);
    EXPECT_EQ(errorOf(t("hrzone", 0, 3)), ParseError::BadValue);
    EXPECT_EQ(errorOf(t("hrzone", 3, 8)), ParseError::BadValue);
    EXPECT_EQ(errorOf(t("hrbpm", 30, 250)), ParseError::Ok);
    EXPECT_EQ(errorOf(t("hrbpm", 29, 100)), ParseError::BadValue);
    EXPECT_EQ(errorOf(t("hrbpm", 100, 251)), ParseError::BadValue);
    EXPECT_EQ(errorOf(t("open", 9, 1)), ParseError::Ok);              // open ignores low/high
}

TEST(WorkoutParser, TargetClassifiesTheWayTheEvaluatorReadsIt)
{
    Workout w;
    ASSERT_EQ(parse(withStep("{\"type\":\"open\",\"target\":{\"kind\":\"pace\",\"low\":230,\"high\":250}}"), w).error,
              ParseError::Ok);
    // 200 sec/km is faster than the fast bound (230): Over. 300 is slower than 250: Under.
    Sample fast; fast.hasPace = true; fast.paceSecPerKm = 200;
    Sample slow; slow.hasPace = true; slow.paceSecPerKm = 300;
    Sample in;   in.hasPace = true;   in.paceSecPerKm = 240;
    EXPECT_EQ(classify(w.steps[0].target, fast), ZoneState::Over);
    EXPECT_EQ(classify(w.steps[0].target, slow), ZoneState::Under);
    EXPECT_EQ(classify(w.steps[0].target, in), ZoneState::InZone);
}

TEST(WorkoutParser, NameLimits)
{
    auto named = [](const std::string& name) {
        return "{\"v\":1,\"name\":\"" + name + "\",\"sport\":\"run\",\"steps\":[{\"type\":\"open\"}]}";
    };
    Workout w;
    ASSERT_EQ(parse(named(std::string(Workout::kNameChars - 1, 'a')), w).error, ParseError::Ok);
    EXPECT_EQ(std::strlen(w.name), static_cast<size_t>(Workout::kNameChars - 1));
    EXPECT_EQ(errorOf(named(std::string(Workout::kNameChars, 'a'))), ParseError::NameTooLong);
    EXPECT_EQ(errorOf(named(std::string(500, 'a'))), ParseError::NameTooLong);
    EXPECT_EQ(errorOf(named("")), ParseError::BadValue);
    EXPECT_EQ(errorOf(named("a\\nb")), ParseError::BadValue);          // \n not allowed
    EXPECT_EQ(errorOf(named("a\\u0041")), ParseError::BadValue);       // \u not allowed
}

TEST(WorkoutParser, StepCountLimit)
{
    auto steps = [](int n) {
        std::string s;
        for (int i = 0; i < n; ++i) {
            s += (i ? "," : "");
            s += "{\"type\":\"open\"}";
        }
        return withStep(s);
    };
    Workout w;
    ASSERT_EQ(parse(steps(Workout::kMaxSteps), w).error, ParseError::Ok);
    EXPECT_EQ(w.stepCount, Workout::kMaxSteps);
    EXPECT_EQ(errorOf(steps(Workout::kMaxSteps + 1)), ParseError::TooManySteps);
    EXPECT_EQ(errorOf(steps(0)), ParseError::Invalid);   // an empty workout: validate() says Empty
}

TEST(WorkoutParser, ValidationFailuresAreInvalidWithTheReason)
{
    Workout w;
    ParseResult r = parse(withStep("{\"type\":\"repeat\",\"from\":0,\"count\":2}"), w);
    EXPECT_EQ(r.error, ParseError::Invalid);
    EXPECT_EQ(r.validation, ValidationError::RepeatIndexNotBefore);

    r = parse(withStep("{\"type\":\"open\"},{\"type\":\"repeat\",\"from\":0,\"count\":0}"), w);
    EXPECT_EQ(r.error, ParseError::Invalid);
    EXPECT_EQ(r.validation, ValidationError::RepeatCountZero);

    r = parse(withStep("{\"type\":\"open\"},{\"type\":\"repeat\",\"from\":9,\"count\":2}"), w);
    EXPECT_EQ(r.error, ParseError::Invalid);
    EXPECT_EQ(r.validation, ValidationError::RepeatIndexOutOfRange);

    r = parse(withStep("{\"type\":\"open\"},{\"type\":\"repeat\",\"from\":0,\"count\":2},"
                       "{\"type\":\"repeat\",\"from\":0,\"count\":2}"), w);
    EXPECT_EQ(r.error, ParseError::Invalid);
    EXPECT_EQ(r.validation, ValidationError::NestedRepeat);

    r = parse(withStep("{\"type\":\"repeat\",\"from\":0,\"count\":1000}"), w);
    EXPECT_EQ(r.error, ParseError::BadValue);   // over the repeat cap
}

TEST(WorkoutParser, DuplicateStepsKeyIsRejected)
{
    EXPECT_EQ(errorOf("{\"v\":1,\"name\":\"x\",\"sport\":\"run\",\"steps\":[{\"type\":\"open\"}],\"steps\":[]}"),
              ParseError::BadValue);
}

TEST(WorkoutParser, DeeplyNestedUnknownValueIsSyntaxNotAStackOverflow)
{
    std::string deep(2000, '[');
    deep += std::string(2000, ']');
    EXPECT_EQ(errorOf("{\"v\":1,\"junk\":" + deep + "}"), ParseError::Syntax);
}

TEST(WorkoutParser, ErrorOffsetPointsAtTheProblem)
{
    const std::string s = withStep("{\"type\":\"warp\"}");
    Workout           w;
    const ParseResult r = parse(s, w);
    EXPECT_EQ(r.error, ParseError::BadValue);
    EXPECT_GT(r.offset, 0u);
    EXPECT_LE(r.offset, s.size());
}

// Every prefix of a valid file, and every single-byte corruption of it, must
// return an error or Ok without reading outside the buffer. The buffer is
// heap-allocated at exactly the prefix length so ASan catches any overrun.
TEST(WorkoutParser, TruncationAndCorruptionNeverOverread)
{
    const std::string good = readFixture("6x400_pace.json");
    ASSERT_FALSE(good.empty());

    // The fixture ends "}\n": every prefix shorter than the closing brace is incomplete.
    const size_t complete = good.find_last_of('}') + 1;
    for (size_t n = 0; n < complete; ++n) {
        char* exact = new char[n == 0 ? 1 : n];
        std::memcpy(exact, good.data(), n);
        Workout w;
        const ParseResult r = parseWorkout(exact, n, w);
        EXPECT_NE(r.error, ParseError::Ok) << "prefix " << n;
        delete[] exact;
    }

    const char probes[] = { '\0', '"', '{', '}', '[', ']', ',', ':', '\\', '-', '9', 'x', '\xFF', '\n' };
    for (size_t i = 0; i < good.size(); ++i) {
        for (char c : probes) {
            std::string bad = good;
            bad[i]          = c;
            char* exact     = new char[bad.size()];
            std::memcpy(exact, bad.data(), bad.size());
            Workout w;
            (void)parseWorkout(exact, bad.size(), w);
            delete[] exact;
        }
    }
}
