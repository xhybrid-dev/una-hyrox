// HybridX Workout: the words for the kinds of workout (PlanText, W3).

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "PlanText.hpp"

using namespace Workout;

namespace {

template <typename F>
std::string text(F f)
{
    char buf[48];
    f(buf, sizeof(buf));
    return buf;
}

PlanView view(SessionType t, StepKind k, uint8_t round, uint8_t rounds, uint8_t station = 0)
{
    PlanView v;
    v.type = static_cast<uint8_t>(t);
    v.kind = static_cast<uint8_t>(k);
    v.step = 1;
    v.steps = 2;
    v.round = round;
    v.rounds = rounds;
    v.station = station;
    return v;
}

LapEventData lapEvent(Cue cue, StepKind finishedKind, uint8_t round, uint8_t finishedLap = 1, bool autoEnded = false)
{
    LapEventData e;
    e.cue = static_cast<uint8_t>(cue);
    e.kind = static_cast<uint8_t>(finishedKind);
    e.round = round;
    e.finishedLap = finishedLap;
    e.autoEnded = autoEnded ? 1 : 0;
    return e;
}

} // namespace

TEST(PlanText, EveryKindHasANameAndTheyGoRound)
{
    const char *want[] = {"Free", "Strength", "EMOM", "AMRAP", "Tabata", "Intervals", "Circuit"};
    SessionType t = SessionType::Plain;
    for (uint8_t i = 0; i < kSessionTypeCount; ++i) {
        EXPECT_STREQ(kindName(t), want[i]);
        t = nextKind(t);
    }
    EXPECT_EQ(t, SessionType::Plain);   // round again
}

TEST(PlanText, SecondsDistancesAndCountdowns)
{
    EXPECT_EQ(text([](char *b, size_t n) { formatSeconds(b, n, 45); }), "45 s");
    EXPECT_EQ(text([](char *b, size_t n) { formatSeconds(b, n, 90); }), "90 s");
    EXPECT_EQ(text([](char *b, size_t n) { formatSeconds(b, n, 120); }), "2 min");
    EXPECT_EQ(text([](char *b, size_t n) { formatSeconds(b, n, 150); }), "2:30");
    EXPECT_EQ(text([](char *b, size_t n) { formatRunM(b, n, 1000); }), "1 km");
    EXPECT_EQ(text([](char *b, size_t n) { formatRunM(b, n, 500); }), "500 m");
    // A countdown rounds up: 0:01 until the step ends, 0:00 only at the end.
    EXPECT_EQ(text([](char *b, size_t n) { formatCountdown(b, n, 1); }), "00:01");
    EXPECT_EQ(text([](char *b, size_t n) { formatCountdown(b, n, 1000); }), "00:01");
    EXPECT_EQ(text([](char *b, size_t n) { formatCountdown(b, n, 1001); }), "00:02");
    EXPECT_EQ(text([](char *b, size_t n) { formatCountdown(b, n, 0); }), "00:00");
    EXPECT_EQ(text([](char *b, size_t n) { formatCountdown(b, n, UINT32_MAX); }), "99:59:59");
}

TEST(PlanText, EachKindHasItsSettings)
{
    PlanField f[3];
    EXPECT_EQ(planFields(SessionType::Plain, f), 0);
    EXPECT_EQ(planFields(SessionType::Tabata, f), 0);
    ASSERT_EQ(planFields(SessionType::Strength, f), 3);
    EXPECT_EQ(f[0], PlanField::Sets);
    EXPECT_EQ(f[1], PlanField::SetRest);
    EXPECT_EQ(f[2], PlanField::SetCap);
    ASSERT_EQ(planFields(SessionType::Emom, f), 1);
    EXPECT_EQ(f[0], PlanField::EmomMin);
    ASSERT_EQ(planFields(SessionType::Amrap, f), 1);
    EXPECT_EQ(f[0], PlanField::AmrapMin);
    ASSERT_EQ(planFields(SessionType::Custom, f), 3);
    EXPECT_EQ(f[2], PlanField::Rounds);
    ASSERT_EQ(planFields(SessionType::Circuit, f), 2);
    EXPECT_EQ(f[1], PlanField::RunM);
}

TEST(PlanText, EveryChoiceIsInsideItsRangeAndRisesAndClampLeavesIt)
{
    const PlanField all[] = {PlanField::Sets, PlanField::SetRest, PlanField::SetCap, PlanField::EmomMin,
                             PlanField::AmrapMin, PlanField::Work, PlanField::Rest, PlanField::Rounds,
                             PlanField::CircuitRounds, PlanField::RunM};
    for (PlanField f : all) {
        uint8_t n = 0;
        const uint16_t *list = choicesOf(f, n);
        ASSERT_NE(list, nullptr);
        ASSERT_GT(n, 1);
        EXPECT_LE(n, 20);
        for (uint8_t i = 0; i < n; ++i) {
            if (i > 0) {
                EXPECT_GT(list[i], list[i - 1]) << fieldName(f);
            }
            // Setting the choice and clamping must keep it: it is in range.
            PlanConfig c;
            uint16_t before = 0;
            for (uint8_t guard = 0; guard < 40; ++guard) {
                c = stepField(c, f);
                if (fieldValue(c, f) == list[i]) {
                    break;
                }
                ASSERT_NE(fieldValue(c, f), before) << fieldName(f);   // it moves
                before = fieldValue(c, f);
            }
            EXPECT_EQ(fieldValue(c, f), list[i]) << fieldName(f) << " choice " << int(i);
        }
    }
}

TEST(PlanText, NextChoiceStepsUpAndWraps)
{
    const uint16_t list[] = {5, 10, 20};
    EXPECT_EQ(nextChoice(list, 3, 5), 10);
    EXPECT_EQ(nextChoice(list, 3, 10), 20);
    EXPECT_EQ(nextChoice(list, 3, 20), 5);
    EXPECT_EQ(nextChoice(list, 3, 7), 10);    // not in the list: the next one up
    EXPECT_EQ(nextChoice(list, 3, 99), 5);
    EXPECT_EQ(nextChoice(nullptr, 0, 7), 7);
    PlanConfig c;                              // 3 sets, the default
    c = stepField(c, PlanField::Sets);
    EXPECT_EQ(c.sets, 4);
    c.sets = 20;
    c = stepField(c, PlanField::Sets);
    EXPECT_EQ(c.sets, 1);
}

TEST(PlanText, FieldValuesAndThePlanInALine)
{
    PlanConfig c;
    EXPECT_EQ(text([&](char *b, size_t n) { formatField(b, n, c, PlanField::Sets); }), "3");
    EXPECT_EQ(text([&](char *b, size_t n) { formatField(b, n, c, PlanField::SetRest); }), "60 s");
    EXPECT_EQ(text([&](char *b, size_t n) { formatField(b, n, c, PlanField::SetCap); }), "5 min");
    EXPECT_EQ(text([&](char *b, size_t n) { formatField(b, n, c, PlanField::EmomMin); }), "10 min");
    EXPECT_EQ(text([&](char *b, size_t n) { formatField(b, n, c, PlanField::RunM); }), "1 km");

    c.type = SessionType::Plain;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, c); }), "");
    c.type = SessionType::Strength;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, c); }), "3 sets, 60 s rest");
    c.sets = 1;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, c); }), "1 set, 60 s rest");
    c.type = SessionType::Emom;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, c); }), "10 min");
    c.type = SessionType::Amrap;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, c); }), "12 min cap");
    c.type = SessionType::Tabata;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, c); }), "8 x 20 s / 10 s");
    c.type = SessionType::Custom;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, c); }), "40 s / 20 s x 6");
    c.type = SessionType::Circuit;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, c); }), "8 rounds, 1 km");
    c.circuitRounds = 1;
    c.runM = 500;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, c); }), "1 round, 500 m");
}

TEST(PlanText, StepNamesForRowsAndToasts)
{
    auto name = [](SessionType t, StepKind k, uint8_t r, uint8_t st = 0, uint8_t lap = 1, bool a = false) {
        return text([&](char *b, size_t n) {
            formatStepName(b, n, static_cast<uint8_t>(t), static_cast<uint8_t>(k), r, st, lap, a);
        });
    };
    EXPECT_EQ(name(SessionType::Plain, StepKind::Work, 0, 0, 3), "Lap 3");
    EXPECT_EQ(name(SessionType::Plain, StepKind::Work, 0, 0, 3, true), "Auto 3");
    EXPECT_EQ(name(SessionType::Strength, StepKind::Work, 2), "Set 2");
    EXPECT_EQ(name(SessionType::Strength, StepKind::Rest, 2), "Rest 2");
    EXPECT_EQ(name(SessionType::Emom, StepKind::Work, 7), "Min 7");
    EXPECT_EQ(name(SessionType::Amrap, StepKind::Work, 4), "Round 4");
    EXPECT_EQ(name(SessionType::Tabata, StepKind::Rest, 8), "Rest 8");
    EXPECT_EQ(name(SessionType::Custom, StepKind::Work, 1), "Work 1");
    EXPECT_EQ(name(SessionType::Circuit, StepKind::Run, 3), "Run 3");
    EXPECT_EQ(name(SessionType::Circuit, StepKind::Station, 2, 2), "Sled push");
    EXPECT_EQ(name(SessionType::Circuit, StepKind::Station, 8, 8), "Wall balls");
}

TEST(PlanText, TheLiveHeading)
{
    auto heading = [](const PlanView &v) { return text([&](char *b, size_t n) { formatStepHeading(b, n, v); }); };
    PlanView free;
    EXPECT_EQ(heading(free), "");
    EXPECT_EQ(heading(view(SessionType::Strength, StepKind::Work, 2, 3)), "Set 2/3");
    EXPECT_EQ(heading(view(SessionType::Strength, StepKind::Rest, 2, 3)), "Rest");
    EXPECT_EQ(heading(view(SessionType::Emom, StepKind::Work, 4, 10)), "Min 4/10");
    EXPECT_EQ(heading(view(SessionType::Amrap, StepKind::Work, 4, 1)), "Round 4");
    EXPECT_EQ(heading(view(SessionType::Tabata, StepKind::Rest, 3, 8)), "Rest 3/8");
    EXPECT_EQ(heading(view(SessionType::Custom, StepKind::Work, 3, 6)), "Work 3/6");
    EXPECT_EQ(heading(view(SessionType::Circuit, StepKind::Run, 2, 8)), "Run 2/8");
    EXPECT_EQ(heading(view(SessionType::Circuit, StepKind::Station, 7, 8, 7)), "Lunges");
    PlanView done = view(SessionType::Emom, StepKind::Work, 10, 10);
    done.done = 1;
    EXPECT_EQ(heading(done), "Done");
}

TEST(PlanText, WhatCountsDownAndWhatTheButtonDoes)
{
    PlanView v = view(SessionType::Emom, StepKind::Work, 1, 10);
    v.remainingMs = 30000;
    EXPECT_TRUE(countsDown(v));
    v = view(SessionType::Strength, StepKind::Work, 1, 3);
    v.remainingMs = 250000;                       // the cap: a backstop, so it counts up
    EXPECT_FALSE(countsDown(v));
    v.kind = static_cast<uint8_t>(StepKind::Rest);
    EXPECT_TRUE(countsDown(v));
    v = view(SessionType::Circuit, StepKind::Run, 1, 8);
    EXPECT_FALSE(countsDown(v));                  // open: remainingMs 0
    v = view(SessionType::Tabata, StepKind::Work, 1, 8);
    v.remainingMs = 5000;
    v.done = 1;
    EXPECT_FALSE(countsDown(v));

    EXPECT_TRUE(buttonCounts(uint8_t(SessionType::Plain)));
    EXPECT_TRUE(buttonCounts(uint8_t(SessionType::Strength)));
    EXPECT_TRUE(buttonCounts(uint8_t(SessionType::Amrap)));
    EXPECT_TRUE(buttonCounts(uint8_t(SessionType::Circuit)));
    EXPECT_FALSE(buttonCounts(uint8_t(SessionType::Emom)));
    EXPECT_FALSE(buttonCounts(uint8_t(SessionType::Tabata)));
    EXPECT_FALSE(buttonCounts(uint8_t(SessionType::Custom)));
}

TEST(PlanText, TheToastSaysWhatBeginsNow)
{
    auto head = [](SessionType t, const LapEventData &e) {
        return text([&](char *b, size_t n) { formatCueHeading(b, n, static_cast<uint8_t>(t), e); });
    };
    EXPECT_EQ(head(SessionType::Strength, lapEvent(Cue::Rest, StepKind::Work, 1)), "Rest");
    EXPECT_EQ(head(SessionType::Strength, lapEvent(Cue::Work, StepKind::Rest, 1)), "Set 2");
    EXPECT_EQ(head(SessionType::Emom, lapEvent(Cue::Work, StepKind::Work, 4)), "Minute 5");
    EXPECT_EQ(head(SessionType::Tabata, lapEvent(Cue::Work, StepKind::Rest, 3)), "Work 4");
    EXPECT_EQ(head(SessionType::Amrap, lapEvent(Cue::Round, StepKind::Work, 3)), "Round 4");
    EXPECT_EQ(head(SessionType::Circuit, lapEvent(Cue::Station, StepKind::Run, 2)), "Sled push");
    EXPECT_EQ(head(SessionType::Circuit, lapEvent(Cue::Run, StepKind::Station, 2)), "Run 3");
    EXPECT_EQ(head(SessionType::Custom, lapEvent(Cue::Done, StepKind::Work, 6)), "Done");
    EXPECT_EQ(head(SessionType::Plain, lapEvent(Cue::Round, StepKind::Work, 0, 3)), "Lap 4");
    EXPECT_EQ(head(SessionType::Plain, lapEvent(Cue::Round, StepKind::Work, 0, 3, true)), "Auto lap");
}

TEST(PlanText, TheSummaryLine)
{
    auto line = [](SessionType t, uint8_t laps, uint16_t rounds = 0) {
        return text([&](char *b, size_t n) { formatSummaryLine(b, n, static_cast<uint8_t>(t), laps, rounds); });
    };
    EXPECT_EQ(line(SessionType::Plain, 4), "4 laps");
    EXPECT_EQ(line(SessionType::Plain, 1), "1 lap");
    EXPECT_EQ(line(SessionType::Strength, 5), "3 sets");      // set, rest, set, rest, set
    EXPECT_EQ(line(SessionType::Strength, 2), "1 set");       // ended in the first rest
    EXPECT_EQ(line(SessionType::Emom, 10), "10 minutes");
    EXPECT_EQ(line(SessionType::Amrap, 8, 7), "7 rounds");    // the eighth lap is the part round
    EXPECT_EQ(line(SessionType::Tabata, 15), "8 of 8 rounds");
    EXPECT_EQ(line(SessionType::Tabata, 4), "2 of 8 rounds");
    EXPECT_EQ(line(SessionType::Custom, 11), "6 rounds");
    EXPECT_EQ(line(SessionType::Circuit, 16), "8 stations");
    EXPECT_EQ(line(SessionType::Circuit, 3), "1 station");
}

TEST(PlanText, EveryWriterStopsAtItsBuffer)
{
    char small[4];
    PlanConfig c;
    c.type = SessionType::Strength;
    EXPECT_EQ(formatPlanSummary(small, sizeof(small), c), 3u);
    EXPECT_STREQ(small, "3 s");
    EXPECT_EQ(formatStepName(small, sizeof(small), uint8_t(SessionType::Circuit), uint8_t(StepKind::Station), 8, 8, 0,
                             false), 3u);
    EXPECT_STREQ(small, "Wal");
    EXPECT_EQ(formatSummaryLine(nullptr, 0, 0, 1, 0), 0u);
}
