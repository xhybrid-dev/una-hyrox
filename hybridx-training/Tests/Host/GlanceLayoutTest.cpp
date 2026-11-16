#include "GlanceFontWidths.hpp"
#include "GlanceLayout.hpp"
#include "PlanText.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <utility>

using namespace XTraining;
using namespace TrainingGlance;
using State = TodayFile::State;

namespace {

constexpr int16_t kW = 240, kH = 60;
constexpr uint32_t kBudget = 32;

View session(Kind kind, const char* what, const char* pace = "")
{
    View v;
    v.state = State::Session;
    v.kind = kind;
    std::snprintf(v.what, sizeof(v.what), "%s", what);
    std::snprintf(v.pace, sizeof(v.pace), "%s", pace);
    v.hasWeek = true;
    v.weekNo = 6;
    v.weeks = 10;
    const Pip week[7] = {Pip::Done, Pip::Rest, Pip::Today, Pip::Rest, Pip::ToCome, Pip::Rest, Pip::ToCome};
    for (int i = 0; i < 7; ++i) {
        v.pip[i] = week[i];
    }
    return v;
}

View after(State s, Kind kind, bool next, bool tomorrow, Kind nextKind, const char* nextWhat)
{
    View v = session(kind, "45 min");
    v.state = s;
    v.hasNext = next;
    v.nextTomorrow = tomorrow;
    v.nextWeekday = 2;
    v.nextKind = nextKind;
    std::snprintf(v.nextWhat, sizeof(v.nextWhat), "%s", nextWhat);
    return v;
}

int headWidth(const char* s) { return Fonts::width(s, Fonts::kSemiBold20); }
int lineWidth(const char* s) { return Fonts::width(s, Fonts::kRegular18); }

void expectAscii(const char* s)
{
    for (; *s != '\0'; ++s) {
        EXPECT_GE(static_cast<unsigned char>(*s), 0x20);
        EXPECT_LT(static_cast<unsigned char>(*s), 0x7F);
    }
}

void expectFits(const View& v, const char* label)
{
    Words w;
    words(v, w);
    EXPECT_LE(headWidth(w.head), kBandW) << label << ": " << w.head;
    EXPECT_LE(lineWidth(w.line), kBandW) << label << ": " << w.line;
    EXPECT_GT(std::strlen(w.head), 0u) << label;
    expectAscii(w.head);
    expectAscii(w.line);
}

}  // namespace

TEST(GlanceLayout, TheBriefsStates)
{
    Words w;
    words(session(Kind::Intervals, "4 x 4 min", "4:18/km"), w);
    EXPECT_STREQ(w.head, "Today: Intervals");
    EXPECT_STREQ(w.line, "4 x 4 min @ 4:18/km");
    EXPECT_EQ(w.headColour, GLANCE_COLOR_WHITE);

    words(after(State::Done, Kind::Intervals, true, false, Kind::Easy, "45 min"), w);
    EXPECT_STREQ(w.head, "Done: Intervals");
    EXPECT_STREQ(w.line, "Wed: Easy 45 min");
    EXPECT_EQ(w.headColour, GLANCE_COLOR_GREEN);

    View rest = after(State::Rest, Kind::Rest, true, true, Kind::Cruise, "3 x 10 min");
    words(rest, w);
    EXPECT_STREQ(w.head, "Rest day");
    EXPECT_STREQ(w.line, "Tomorrow: Cruise");   // the rest of the line would not fit

    View stale;
    stale.state = State::Stale;
    words(stale, w);
    EXPECT_STREQ(w.head, "X Training");
    EXPECT_STREQ(w.line, "Open the app");
    stale.state = State::NoPlan;
    words(stale, w);
    EXPECT_STREQ(w.line, "Set a plan in UNA");
}

TEST(GlanceLayout, EveryKindFitsInEveryStateWithTheLongestWords)
{
    const char* const paces[] = {"", "4:18/km", "12:34/mi", "16:05/mi"};
    const char* const whats[] = {"", "4 x 4 min", "100 min, 20 fast", "2 x 8 x 30/30", "10 x 400 m", "3 x 10 min", "21.1 km",
                                 "1234567890123456789"};
    for (uint8_t k = 0; k < kKindCount; ++k) {
        const Kind kind = static_cast<Kind>(k);
        for (const char* what : whats) {
            for (const char* pace : paces) {
                expectFits(session(kind, what, pace), PlanText::name(kind));
                for (uint8_t n = 0; n < kKindCount; ++n) {
                    const Kind nk = static_cast<Kind>(n);
                    for (bool tomorrow : {false, true}) {
                        for (State s : {State::Done, State::Skipped, State::Rest}) {
                            expectFits(after(s, kind, true, tomorrow, nk, what), PlanText::name(nk));
                        }
                    }
                }
            }
        }
        expectFits(after(State::Done, kind, false, false, Kind::Rest, ""), "done, nothing next");
        expectFits(after(State::Skipped, kind, false, false, Kind::Rest, ""), "skipped, nothing next");
    }
    for (State s : {State::NoFile, State::NoPlan, State::ClockUnset, State::Stale, State::Paused, State::BeforeStart,
                    State::Complete}) {
        View v;
        v.state = s;
        v.startDate = 20271231u;
        expectFits(v, "no session");
    }
}

TEST(GlanceLayout, AnEasyRunWithStridesIsJustEasyOnTheGlance)
{
    Words w;
    words(session(Kind::EasyStrides, "40 min + strides"), w);
    EXPECT_STREQ(w.head, "Today: Easy");
    EXPECT_STREQ(w.line, "40 min + strides");
    words(after(State::Done, Kind::Intervals, true, true, Kind::EasyStrides, "40 min + strides"), w);
    EXPECT_STREQ(w.line, "Tomorrow: Easy");   // the rest would not fit
    words(after(State::Done, Kind::Intervals, true, false, Kind::EasyStrides, "40 min"), w);
    EXPECT_STREQ(w.line, "Wed: Easy 40 min");
}

TEST(GlanceLayout, LongWordsGiveWayToShorterOnes)
{
    Words w;
    // A mile pace on a session with a long "what": the pace goes, then the what stays.
    words(session(Kind::Intervals, "2 x 8 x 30/30", "4:00/km"), w);
    EXPECT_STREQ(w.line, "2 x 8 x 30/30");
    words(session(Kind::Intervals, "4 x 4 min", "6:55/mi"), w);
    EXPECT_STREQ(w.line, "4 x 4 min @ 6:55/mi");
    // The longest name and label.
    words(after(State::Skipped, Kind::LongFinish, true, false, Kind::Easy, ""), w);
    EXPECT_STREQ(w.head, "Skipped: Long+fast");
    EXPECT_EQ(w.headColour, GLANCE_COLOR_YELLOW_DARK);
}

TEST(GlanceLayout, TheFullLayout)
{
    Layout l;
    layout(session(Kind::Intervals, "4 x 4 min", "4:18/km"), kW, kH, kBudget, l);
    ASSERT_EQ(l.kind, Layout::Kind::Full);
    ASSERT_EQ(l.count, kFullControls);
    EXPECT_LE(l.count, kBudget);
    EXPECT_EQ(l.items[0].type, Spec::Type::Text);
    EXPECT_EQ(l.items[0].font, GLANCE_FONT_POPPINS_SEMIBOLD_20);
    EXPECT_STREQ(l.items[0].text, "Today: Intervals");
    EXPECT_EQ(l.items[1].font, GLANCE_FONT_POPPINS_REGULAR_18);
    EXPECT_EQ(l.items[1].colour, GLANCE_COLOR_GRAY);
    // Seven pips, left to right, in the colours of the week: done, rest, today, rest, to come, rest, to come.
    const uint8_t colours[7] = {GLANCE_COLOR_GREEN, GLANCE_COLOR_GRAY_DARK, GLANCE_COLOR_WHITE, GLANCE_COLOR_GRAY_DARK,
                                GLANCE_COLOR_GRAY, GLANCE_COLOR_GRAY_DARK, GLANCE_COLOR_GRAY};
    for (int i = 0; i < 7; ++i) {
        const Spec& p = l.items[2 + i];
        EXPECT_EQ(p.type, Spec::Type::Rect) << i;
        EXPECT_EQ(p.colour, colours[i]) << i;
        if (i > 0) {
            const Spec& q = l.items[1 + i];
            EXPECT_GE(p.x, q.x + q.w + 2) << "a gap between pips";
            EXPECT_EQ(p.y, q.y);
        }
    }
    // The bar: a track and its fill from the left, 6 of 10 weeks.
    const Spec& track = l.items[9];
    const Spec& fill = l.items[10];
    EXPECT_EQ(track.type, Spec::Type::Rect);
    EXPECT_EQ(track.colour, GLANCE_COLOR_GRAY_DARK);
    EXPECT_EQ(fill.x, track.x);
    EXPECT_EQ(fill.w, track.w * 6 / 10);
    EXPECT_GE(track.x, l.items[8].x + l.items[8].w + 8) << "clear of the pips";
}

TEST(GlanceLayout, EverythingIsInsideTheAreaAndTheBand)
{
    const int16_t widths[] = {240, 230, 196, 190, 180};
    const int16_t heights[] = {60, 59, 55, 48, 40, 25, 20};
    for (const int16_t w : widths) {
        for (const int16_t h : heights) {
            for (const uint32_t budget : {32u, 12u, 11u, 10u, 3u, 2u}) {
                Layout l;
                layout(session(Kind::Intervals, "4 x 4 min", "4:18/km"), w, h, budget, l);
                EXPECT_LE(l.count, budget) << w << "x" << h << " " << budget;
                EXPECT_GT(l.count, 0);
                for (uint8_t i = 0; i < l.count; ++i) {
                    const Spec& s = l.items[i];
                    EXPECT_GE(s.x, 0) << w << "x" << h << " #" << int(i);
                    EXPECT_GE(s.y, 0) << w << "x" << h << " #" << int(i);
                    EXPECT_LE(s.x + s.w, w) << w << "x" << h << " #" << int(i);
                    if (h >= 25) {
                        EXPECT_LE(s.y + s.h, h) << w << "x" << h << " #" << int(i);
                    }
                    if (s.type == Spec::Type::Text) {
                        EXPECT_LE(s.w, kBandW);
                        EXPECT_NE(s.font, GLANCE_FONT_POPPINS_MEDIUM_10) << "that face holds digits alone";
                        EXPECT_LE(std::strlen(s.text), static_cast<size_t>(GLANCE_TEXT_SIZE));
                    }
                }
            }
        }
    }
}

TEST(GlanceLayout, ItAdaptsToWhatTheWatchGives)
{
    const View v = session(Kind::Intervals, "4 x 4 min", "4:18/km");
    Layout l;
    layout(v, kW, kH, kFullControls, l);
    EXPECT_EQ(l.kind, Layout::Kind::Full);
    layout(v, kW, kH, kFullControls - 1, l);
    EXPECT_EQ(l.kind, Layout::Kind::Compact) << "too few controls";
    EXPECT_EQ(l.count, 2);
    layout(v, kW, 50, kBudget, l);
    EXPECT_EQ(l.kind, Layout::Kind::Compact) << "a short area";
    layout(v, kBandW - 1, kH, kBudget, l);
    EXPECT_EQ(l.kind, Layout::Kind::Compact) << "a narrow area";
    layout(v, kW, 40, kBudget, l);
    EXPECT_EQ(l.kind, Layout::Kind::Tiny);
    EXPECT_EQ(l.count, 1);
    layout(v, kW, kH, 2, l);
    EXPECT_EQ(l.kind, Layout::Kind::Compact);
    layout(v, kW, kH, 1, l);
    EXPECT_EQ(l.kind, Layout::Kind::Tiny);
    // A state with no week is two lines, centred.
    View stale;
    stale.state = State::Stale;
    layout(stale, kW, kH, kBudget, l);
    EXPECT_EQ(l.kind, Layout::Kind::Compact);
    EXPECT_STREQ(l.items[0].text, "X Training");
    EXPECT_EQ(l.items[0].align, GLANCE_ALIGN_H_CENTER);
}

TEST(GlanceLayout, TheBarsFillIsTheWeeksBegunOfAll)
{
    View v = session(Kind::Easy, "40 min");
    Layout l;
    for (const auto& [week, weeks] : {std::pair<int, int>{1, 10}, {5, 10}, {10, 10}, {12, 10}, {1, 40}, {40, 40}}) {
        v.weekNo = static_cast<uint8_t>(week);
        v.weeks = static_cast<uint8_t>(weeks);
        layout(v, kW, kH, kBudget, l);
        ASSERT_EQ(l.count, kFullControls);
        const Spec& track = l.items[9];
        const Spec& fill = l.items[10];
        EXPECT_GE(fill.w, 2);
        EXPECT_LE(fill.w, track.w) << week << "/" << weeks;
        EXPECT_EQ(fill.w, std::max(2, track.w * std::min(week, weeks) / weeks));
    }
    // No weeks known: a track and no fill, and the budget is not wasted.
    v.weeks = 0;
    layout(v, kW, kH, kBudget, l);
    EXPECT_EQ(l.count, kFullControls - 1);
}
