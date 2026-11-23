/**
 * Host tests for DashWords: the family dashboard's words (NOTES "S7"), and that
 * every label and tip fits its line, including the widest cases.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "LoadFormText.hpp"
#include "TodayFile.hpp"
#include "WeekMath.hpp"
#include "WeightUnit.hpp"
#include "GraphMath.hpp"
#include "gui/copy/Coach.hpp"
#include "gui/copy/DashWords.hpp"

using DashWords::kMaxLabelPx;
using DashWords::kMaxTipPx;
using DashWords::labelPx;
using DashWords::Row;
using DashWords::tipPx;
using Streak::FamilyView;
using Streak::Kind;
using Streak::PlanToday;
using Streak::SportSum;
using State = XTraining::TodayFile::State;
using PlanKind = XTraining::Kind;

namespace
{

SportSum sum(uint8_t sessions, uint16_t minutes, uint32_t metres)
{
    SportSum s;
    s.sessions = sessions;
    s.minutes  = minutes;
    s.metres   = metres;
    return s;
}

PlanToday planToday(State state, PlanKind kind, const char* what, const char* pace = "")
{
    PlanToday p;
    p.state = static_cast<uint8_t>(state);
    p.kind  = static_cast<uint8_t>(kind);
    std::snprintf(p.what, sizeof(p.what), "%s", what);
    std::snprintf(p.pace, sizeof(p.pace), "%s", pace);
    return p;
}

void expectFits(const Row& r)
{
    EXPECT_LE(labelPx(r.label), kMaxLabelPx) << r.label;
    EXPECT_LE(tipPx(r.tip), kMaxTipPx) << r.tip;
    EXPECT_GT(std::strlen(r.label), 0u);
    EXPECT_GT(std::strlen(r.tip), 0u) << r.label;
}

} // namespace

TEST(DashText, WidthsFromTheFonts)
{
    // Measured against what the wheel already shows: Training load's learning tip, 189 px, the
    // widest on the watch (NOTES "S7").
    EXPECT_EQ(tipPx("Learning: day 5 of 14"), 189);
    EXPECT_EQ(tipPx(""), 0);
    // The middle dot is its own glyph, narrower than a letter: two bytes, one width.
    EXPECT_EQ(tipPx("\xC2\xB7"), 4);
    EXPECT_EQ(labelPx("Training load"), 168);
    // A character the fonts lack counts as their widest.
    EXPECT_GE(tipPx("\xC3\xA9"), tipPx("W"));
}

TEST(DashText, Durations)
{
    char b[16];
    DashWords::duration(0, b, sizeof(b));
    EXPECT_STREQ(b, "0 min");
    DashWords::duration(45, b, sizeof(b));
    EXPECT_STREQ(b, "45 min");
    DashWords::duration(65, b, sizeof(b));
    EXPECT_STREQ(b, "1 h 05");
    DashWords::duration(750, b, sizeof(b));
    EXPECT_STREQ(b, "12 h 30");
    DashWords::duration(65535, b, sizeof(b));
    EXPECT_STREQ(b, "1092 h 15");
}

TEST(DashText, Distances)
{
    char b[16];
    DashWords::distance(14249, b, sizeof(b));
    EXPECT_STREQ(b, "14.2 km");
    DashWords::distance(14250, b, sizeof(b));
    EXPECT_STREQ(b, "14.3 km");
    DashWords::distance(400, b, sizeof(b));
    EXPECT_STREQ(b, "0.4 km");
    DashWords::distance(99949, b, sizeof(b));
    EXPECT_STREQ(b, "99.9 km");
    DashWords::distance(99950, b, sizeof(b));
    EXPECT_STREQ(b, "100 km");
    DashWords::distance(123456, b, sizeof(b));
    EXPECT_STREQ(b, "123 km");
}

TEST(DashText, SportTips)
{
    char b[64];
    // With a distance the noun does not fit beside it: the count stands alone.
    DashWords::sportTip(Kind::Run, sum(2, 65, 14200), b, sizeof(b));
    EXPECT_STREQ(b, "2 \xC2\xB7 1 h 05 \xC2\xB7 14.2 km");
    DashWords::sportTip(Kind::Ride, sum(1, 90, 30000), b, sizeof(b));
    EXPECT_STREQ(b, "1 \xC2\xB7 1 h 30 \xC2\xB7 30.0 km");
    DashWords::sportTip(Kind::Walk, sum(1, 9, 400), b, sizeof(b));
    EXPECT_STREQ(b, "1 \xC2\xB7 9 min \xC2\xB7 0.4 km");
    DashWords::sportTip(Kind::Strength, sum(3, 135, 0), b, sizeof(b));
    EXPECT_STREQ(b, "3 sessions \xC2\xB7 2 h 15");
    DashWords::sportTip(Kind::Workout, sum(1, 45, 0), b, sizeof(b));
    EXPECT_STREQ(b, "1 workout \xC2\xB7 45 min");
}

TEST(DashText, SportTipsAlwaysFit)
{
    char b[64];
    for (uint8_t k = 0; k < Streak::kKindCount; ++k) {
        for (const SportSum& s : {sum(1, 5, 0), sum(255, 65535, 0xFFFFFFFFu), sum(99, 5999, 999999), sum(7, 600, 99949)}) {
            DashWords::sportTip(static_cast<Kind>(k), s, b, sizeof(b));
            EXPECT_LE(tipPx(b), kMaxTipPx) << b;
        }
    }
}

TEST(DashText, AllAndMenuTips)
{
    char b[64];
    DashWords::allTip(sum(5, 200, 24500), b, sizeof(b));
    EXPECT_STREQ(b, "5 sessions \xC2\xB7 3 h 20");
    DashWords::allTip(sum(1, 45, 0), b, sizeof(b));
    EXPECT_STREQ(b, "1 session \xC2\xB7 45 min");

    Streak::WeekSum w;
    DashWords::menuTip(w, 0, b, sizeof(b));
    EXPECT_STREQ(b, "Nothing yet");
    DashWords::menuTip(w, 2, b, sizeof(b));   // manual logs only
    EXPECT_STREQ(b, "2 by hand");
    w.all = sum(5, 200, 24500);
    DashWords::menuTip(w, 1, b, sizeof(b));
    EXPECT_STREQ(b, "3 h 20 \xC2\xB7 24.5 km");
    w.all = sum(2, 90, 0);
    DashWords::menuTip(w, 1, b, sizeof(b));
    EXPECT_STREQ(b, "1 h 30");
    w.all = sum(255, 65535, 0xFFFFFFFFu);
    DashWords::menuTip(w, 1, b, sizeof(b));
    EXPECT_LE(tipPx(b), kMaxTipPx) << b;
    DashWords::allTip(w.all, b, sizeof(b));
    EXPECT_LE(tipPx(b), kMaxTipPx) << b;

    DashWords::manualTip(1, b, sizeof(b));
    EXPECT_STREQ(b, "1 session");
}

TEST(DashText, PlanRowEveryState)
{
    Row r;
    EXPECT_TRUE(DashWords::planRow(planToday(State::Session, PlanKind::Tempo, "30 min", "4:18/km"), r));
    EXPECT_STREQ(r.label, "Today's plan");
    EXPECT_STREQ(r.tip, "Tempo 30 min");
    EXPECT_TRUE(DashWords::planRow(planToday(State::Done, PlanKind::Long, "90 min"), r));
    EXPECT_STREQ(r.tip, "Done: Long");
    EXPECT_EQ(r.tone, 1);
    EXPECT_TRUE(DashWords::planRow(planToday(State::Skipped, PlanKind::Hills, "8 x 60 s"), r));
    EXPECT_STREQ(r.tip, "Skipped: Hills");
    EXPECT_TRUE(DashWords::planRow(planToday(State::Rest, PlanKind::Rest, ""), r));
    EXPECT_STREQ(r.tip, "Rest day");
    EXPECT_TRUE(DashWords::planRow(planToday(State::Paused, PlanKind::Rest, ""), r));
    EXPECT_STREQ(r.tip, "Plan paused");
    PlanToday before = planToday(State::BeforeStart, PlanKind::Rest, "");
    before.startDate = 20261207;
    EXPECT_TRUE(DashWords::planRow(before, r));
    EXPECT_STREQ(r.tip, "Starts 7 Dec");
    EXPECT_TRUE(DashWords::planRow(planToday(State::Complete, PlanKind::Rest, ""), r));
    EXPECT_STREQ(r.tip, "Plan complete");
    EXPECT_TRUE(DashWords::planRow(planToday(State::Stale, PlanKind::Rest, ""), r));
    EXPECT_STREQ(r.tip, "Open X Training");
    EXPECT_EQ(r.tone, 2);
    EXPECT_TRUE(DashWords::planRow(planToday(State::NoPlan, PlanKind::Rest, ""), r));
    EXPECT_STREQ(r.tip, "No plan yet");
    EXPECT_TRUE(DashWords::planRow(planToday(State::ClockUnset, PlanKind::Rest, ""), r));
    EXPECT_STREQ(r.tip, "Date not set");
    EXPECT_FALSE(DashWords::planRow(planToday(State::NoFile, PlanKind::Rest, ""), r));
}

TEST(DashText, PlanRowsFitWithTheLongestWords)
{
    // X Training's longest "what" is 19 characters (TodayDay::kWhatLen - 1): with a long name
    // the tip falls back to the name alone. Every state's row, with each kind, fits.
    for (uint8_t k = 1; k < XTraining::kKindCount; ++k) {
        Row r;
        PlanToday p = planToday(State::Session, static_cast<PlanKind>(k), "100 min, 20 fast xx", "12:34/mi");
        ASSERT_TRUE(DashWords::planRow(p, r));
        expectFits(r);
        ASSERT_TRUE(DashWords::paceRow(p, r));
        expectFits(r);
        p.state = static_cast<uint8_t>(State::Done);
        p.hasNext      = 1;
        p.nextKind     = k;
        p.nextTomorrow = 1;
        std::snprintf(p.nextWhat, sizeof(p.nextWhat), "%s", "100 min, 20 fast xx");
        ASSERT_TRUE(DashWords::nextRow(p, r));
        expectFits(r);
        p.nextTomorrow = 0;
        p.nextWeekday  = 2;
        ASSERT_TRUE(DashWords::nextRow(p, r));
        expectFits(r);
        for (State st : {State::Done, State::Skipped, State::Rest, State::Paused, State::BeforeStart,
                         State::Complete, State::Stale}) {
            p.state     = static_cast<uint8_t>(st);
            p.startDate = 20261231;
            ASSERT_TRUE(DashWords::planRow(p, r));
            expectFits(r);
        }
    }
}

TEST(DashText, WeightRowsFitInEveryUnit)
{
    FamilyView v;
    v.flags                = FamilyView::kWeight;
    v.clockOk              = 1;
    v.today                = 20000;
    v.weight.lastDay       = 19999;
    v.weight.hasWeekChange = 1;
    for (uint8_t unit = 0; unit < 3; ++unit) {
        for (int32_t trend : {4000, 12345, 49999}) {
            for (int32_t change : {-1234, -5, 0, 999}) {
                v.weight.unit       = unit;
                v.weight.trend      = trend;
                v.weight.weekChange = change;
                Row r;
                ASSERT_TRUE(DashWords::weightRow(v, r));
                expectFits(r);
                ASSERT_TRUE(DashWords::weightChangeRow(v, r));
                expectFits(r);
            }
        }
    }
    v.weight.lastDay = 20000 - 999;
    Row r;
    ASSERT_TRUE(DashWords::weightChangeRow(v, r));
    expectFits(r);
}

TEST(DashText, PaceAndNext)
{
    Row r;
    PlanToday p = planToday(State::Session, PlanKind::Tempo, "30 min", "4:18/km");
    EXPECT_TRUE(DashWords::paceRow(p, r));
    EXPECT_STREQ(r.tip, "4:18/km");
    EXPECT_FALSE(DashWords::nextRow(p, r));   // today's session comes first

    p = planToday(State::Rest, PlanKind::Rest, "");
    EXPECT_FALSE(DashWords::paceRow(p, r));
    p.hasNext      = 1;
    p.nextKind     = static_cast<uint8_t>(PlanKind::Easy);
    p.nextTomorrow = 1;
    std::snprintf(p.nextWhat, sizeof(p.nextWhat), "40 min");
    EXPECT_TRUE(DashWords::nextRow(p, r));
    EXPECT_STREQ(r.tip, "Tomorrow: Easy");   // "Tomorrow: Easy 40 min" is 209 px
    p.nextTomorrow = 0;
    p.nextWeekday  = 2;
    EXPECT_TRUE(DashWords::nextRow(p, r));
    EXPECT_STREQ(r.tip, "Wed: Easy 40 min");
    p.nextKind = static_cast<uint8_t>(PlanKind::EasyStrides);   // named "Easy", as on X Training's glance
    EXPECT_TRUE(DashWords::nextRow(p, r));
    EXPECT_STREQ(r.tip, "Wed: Easy 40 min");
}

TEST(DashText, Vo2AndWeight)
{
    FamilyView v;
    v.today   = 20000;
    v.clockOk = 1;
    Row r;
    EXPECT_FALSE(DashWords::vo2Row(v, r));
    EXPECT_FALSE(DashWords::weightRow(v, r));
    EXPECT_FALSE(DashWords::weightChangeRow(v, r));

    v.flags |= FamilyView::kVo2 | FamilyView::kWeight;
    v.vo2.x10  = 523;
    v.vo2.runs = 5;
    EXPECT_TRUE(DashWords::vo2Row(v, r));
    EXPECT_STREQ(r.label, "VO2max");
    EXPECT_STREQ(r.tip, "52.3 \xC2\xB7 5 runs");
    v.vo2.runs = 1;
    EXPECT_TRUE(DashWords::vo2Row(v, r));
    EXPECT_STREQ(r.tip, "52.3 \xC2\xB7 1 run");

    v.weight.trend         = 7843;
    v.weight.lastDay       = 19998;
    v.weight.hasWeekChange = 1;
    v.weight.weekChange    = -37;
    EXPECT_TRUE(DashWords::weightRow(v, r));
    EXPECT_STREQ(r.tip, "78.4 kg");
    EXPECT_TRUE(DashWords::weightChangeRow(v, r));
    EXPECT_STREQ(r.label, "Trend");
    EXPECT_STREQ(r.tip, "-0.4 kg this week");

    v.weight.unit = static_cast<uint8_t>(XWeighIn::WeightUnit::StLb);
    EXPECT_TRUE(DashWords::weightRow(v, r));
    EXPECT_STREQ(r.tip, "12st 4.9lb");
    EXPECT_TRUE(DashWords::weightChangeRow(v, r));
    EXPECT_STREQ(r.tip, "-0.8 lb this week");
    expectFits(r);

    v.weight.hasWeekChange = 0;
    EXPECT_FALSE(DashWords::weightChangeRow(v, r));

    v.weight.lastDay = 19988;   // twelve days ago
    EXPECT_TRUE(DashWords::weightChangeRow(v, r));
    EXPECT_STREQ(r.label, "Last weigh-in");
    EXPECT_STREQ(r.tip, "12 days ago");
    EXPECT_EQ(r.tone, 2);
}

TEST(DashText, TitlesFitTheTitleBox)
{
    EXPECT_LE(tipPx(DashWords::kSportTitle), DashWords::kMaxTitlePx);
    EXPECT_LE(tipPx(DashWords::kNumbersTitle), DashWords::kMaxTitlePx);
    EXPECT_GT(tipPx("Your numbers"), DashWords::kMaxTitlePx);   // why the screen is "Numbers"
    EXPECT_LE(labelPx("Your numbers"), kMaxLabelPx);            // the menu row keeps it
    EXPECT_LE(labelPx("By sport"), kMaxLabelPx);
}

TEST(DashText, NumbersRowsInOrder)
{
    Row        rows[DashWords::kMaxRows];
    FamilyView v;
    // Nothing from the family: a row for each app, saying which.
    ASSERT_EQ(DashWords::numbersRows(v, rows), 3);
    EXPECT_STREQ(rows[0].label, "Today's plan");
    EXPECT_STREQ(rows[0].tip, "Needs X Training");
    EXPECT_STREQ(rows[1].label, "VO2max");
    EXPECT_STREQ(rows[1].tip, "Needs X Run");
    EXPECT_STREQ(rows[2].label, "Weight");
    EXPECT_STREQ(rows[2].tip, "Needs X Weigh-in");
    for (uint8_t i = 0; i < 3; ++i) {
        expectFits(rows[i]);
    }

    v.flags   = FamilyView::kPlan | FamilyView::kVo2 | FamilyView::kWeight;
    v.today   = 20000;
    v.clockOk = 1;
    v.plan    = planToday(State::Session, PlanKind::Tempo, "30 min", "4:18/km");
    v.vo2.x10  = 523;
    v.vo2.runs = 5;
    v.weight.trend         = 7843;
    v.weight.lastDay       = 19999;
    v.weight.hasWeekChange = 1;
    v.weight.weekChange    = -37;
    const uint8_t n = DashWords::numbersRows(v, rows);
    ASSERT_EQ(n, 5);
    EXPECT_STREQ(rows[0].label, "Today's plan");
    EXPECT_STREQ(rows[1].label, "Pace");
    EXPECT_STREQ(rows[2].label, "VO2max");
    EXPECT_STREQ(rows[3].label, "Weight");
    EXPECT_STREQ(rows[4].label, "Trend");
    for (uint8_t i = 0; i < n; ++i) {
        expectFits(rows[i]);
    }
    // A day done shows the next session where a session today shows its pace: five rows at most.
    v.plan = planToday(State::Done, PlanKind::Tempo, "30 min");
    v.plan.hasNext = 1;
    EXPECT_EQ(DashWords::numbersRows(v, rows), 5);
    EXPECT_STREQ(rows[1].label, "Next");

    char tip[64];
    v.plan = planToday(State::Session, PlanKind::Tempo, "30 min", "4:18/km");
    DashWords::numbersMenuTip(v, tip, sizeof(tip));
    EXPECT_STREQ(tip, "Tempo 30 min");
    v.flags = FamilyView::kVo2;
    DashWords::numbersMenuTip(v, tip, sizeof(tip));
    EXPECT_STREQ(tip, "VO2max 52.3");
    v.flags = 0;
    DashWords::numbersMenuTip(v, tip, sizeof(tip));
    EXPECT_STREQ(tip, "From the X apps");

    // No plan: its row says so, and the menu names the next figure rather than "No plan yet".
    v.flags      = FamilyView::kPlan | FamilyView::kWeight;
    v.plan.state = static_cast<uint8_t>(State::NoPlan);
    ASSERT_EQ(DashWords::numbersRows(v, rows), 4);
    EXPECT_STREQ(rows[0].tip, "No plan yet");
    EXPECT_STREQ(rows[1].tip, "Needs X Run");
    EXPECT_STREQ(rows[2].label, "Weight");
    DashWords::numbersMenuTip(v, tip, sizeof(tip));
    EXPECT_STREQ(tip, "78.4 kg");
}

TEST(DashText, HomeNamesTodaysSessionOnABankedWeek)
{
    FamilyView v;
    char       b[48];
    EXPECT_FALSE(DashWords::bankedPlanLine(v, b, sizeof(b)));   // no X Training
    v.flags = FamilyView::kPlan;
    v.plan  = planToday(State::Session, PlanKind::Tempo, "30 min");
    ASSERT_TRUE(DashWords::bankedPlanLine(v, b, sizeof(b)));
    EXPECT_STREQ(b, "Banked. Tempo today");
    // Too long for the coach's line: the name alone.
    v.plan = planToday(State::Session, PlanKind::Intervals, "5 x 3 min");
    ASSERT_TRUE(DashWords::bankedPlanLine(v, b, sizeof(b)));
    EXPECT_STREQ(b, "Intervals today");
    for (uint8_t k = 1; k < XTraining::kKindCount; ++k) {
        v.plan = planToday(State::Session, static_cast<PlanKind>(k), "");
        ASSERT_TRUE(DashWords::bankedPlanLine(v, b, sizeof(b)));
        EXPECT_LE(std::strlen(b), Coach::kMaxCoachChars) << b;
    }
    // Done, skipped or a rest day: "Rest up." stands.
    for (State st : {State::Done, State::Skipped, State::Rest, State::Paused, State::Stale}) {
        v.plan = planToday(st, PlanKind::Tempo, "30 min");
        EXPECT_FALSE(DashWords::bankedPlanLine(v, b, sizeof(b)));
    }
}

// -- The training load graphs' words (NOTES "S8") ---------------------------------------------

TEST(DashText, GraphTitlesLegendsAndDatesFit)
{
    for (const char* t : DashWords::kGraphTitles) {
        EXPECT_LE(tipPx(t), DashWords::kMaxTitlePx) << t;   // titles are Italic 18, the tips' face
    }
    EXPECT_GT(tipPx("Fitness, fatigue"), DashWords::kMaxTitlePx);   // why page one is "Training load"

    char b[48];
    DashWords::fitnessLegend(9999, b, sizeof(b));
    EXPECT_STREQ(b, "Fitness 9999");
    EXPECT_LE(DashWords::legendPx(b), DashWords::kMaxLegendPx);
    DashWords::fatigueLegend(65535, b, sizeof(b));
    EXPECT_LE(DashWords::legendPx(b), DashWords::kMaxLegendPx) << b;
    DashWords::weekLoadLegend(0xFFFFFFFFu, b, sizeof(b));
    EXPECT_STREQ(b, "This week 99999");
    EXPECT_LE(DashWords::legendPx(b), DashWords::kMaxLegendPx);
    DashWords::todayLoadLegend(65535, b, sizeof(b));
    EXPECT_LE(DashWords::legendPx(b), DashWords::kMaxLegendPx);

    DashWords::formLegend(14, static_cast<uint8_t>(Streak::FormWord::Fresh), b, sizeof(b));
    EXPECT_STREQ(b, "Form +14, Fresh");
    // Too wide with the word: the number alone.
    DashWords::formLegend(-9999, static_cast<uint8_t>(Streak::FormWord::Overreaching), b, sizeof(b));
    EXPECT_STREQ(b, "Form -9999");
    for (int16_t form : {-9999, -300, -12, 0, 7, 120, 9999}) {
        for (uint8_t w = 0; w <= static_cast<uint8_t>(Streak::FormWord::Overreaching); ++w) {
            DashWords::formLegend(form, w, b, sizeof(b));
            EXPECT_LE(DashWords::legendPx(b), DashWords::kMaxLegendPx) << b;
        }
    }
    DashWords::formLegend(3, static_cast<uint8_t>(Streak::FormWord::Learning), b, sizeof(b));
    EXPECT_STREQ(b, "Form +3");   // no word while learning

    // The dates under the plot: the widest months.
    const int32_t sep30 = Streak::WeekMath::daysFromCivil(2026, 9, 30);
    DashWords::dayMonth(sep30, b, sizeof(b));
    EXPECT_STREQ(b, "30 Sep");
    DashWords::graphSpan(sep30 - 83, sep30, b, sizeof(b));
    EXPECT_STREQ(b, "9 Jul - 30 Sep");
    for (int32_t d = sep30; d < sep30 + 366; d += 7) {
        DashWords::graphSpan(d - 83, d, b, sizeof(b));
        EXPECT_LE(DashWords::smallPx(b), DashWords::kMaxSpanPx) << b;
    }

    // The scale's labels: whole points, a sign above zero on the form page.
    DashWords::scaleLabel(800, 10, false, b, sizeof(b));
    EXPECT_STREQ(b, "80");
    DashWords::scaleLabel(150, 10, true, b, sizeof(b));
    EXPECT_STREQ(b, "+15");
    DashWords::scaleLabel(-150, 10, true, b, sizeof(b));
    EXPECT_STREQ(b, "-15");
    DashWords::scaleLabel(0, 10, true, b, sizeof(b));
    EXPECT_STREQ(b, "0");
    // The widest each scale can reach: tenths up to 65535 give 0 to 8000 (or -8000 to +8000); a
    // daily load up to 65535 gives 80000.
    const uint16_t top[] = {65535};
    const uint16_t none[] = {0};
    const auto     fit   = Streak::GraphMath::fromZero(top, top, 1, 10, 100);
    const auto     form  = Streak::GraphMath::aroundZero(top, none, 1, 10, 50);
    const auto     load  = Streak::GraphMath::fromZero(top, nullptr, 1, 1, 50);
    DashWords::scaleLabel(fit.hi, 10, false, b, sizeof(b));
    EXPECT_STREQ(b, "8000");
    EXPECT_LE(DashWords::smallPx(b), DashWords::kMaxScalePx) << b;
    DashWords::scaleLabel(form.lo, 10, true, b, sizeof(b));
    EXPECT_STREQ(b, "-8000");
    EXPECT_LE(DashWords::smallPx(b), DashWords::kMaxScalePx) << b;
    DashWords::scaleLabel(load.hi, 1, false, b, sizeof(b));
    EXPECT_STREQ(b, "80000");
    EXPECT_LE(DashWords::smallPx(b), DashWords::kMaxScalePx) << b;
}
