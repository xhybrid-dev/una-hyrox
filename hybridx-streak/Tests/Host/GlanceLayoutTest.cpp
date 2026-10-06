/**
 * Host tests for the glance layout (PLAN 8): whatever area and control budget
 * the watch reports, every control fits inside it, the budget is kept, and
 * every text fits a glance text control: at most GLANCE_TEXT_SIZE bytes, plain
 * ASCII in a face that has letters, and no wider than its box in the real
 * Poppins widths (support/GlanceFontWidths.hpp).
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "GlanceFontWidths.hpp"
#include "GlanceLayout.hpp"
#include "Summits.hpp"

using Glance::Layout;
using Glance::Spec;
using Glance::State;

namespace
{

Streak::HomeView view(uint16_t streak, uint8_t sessions, uint8_t target, Streak::Mood mood, uint8_t flags = 0)
{
    Streak::HomeView v;
    v.streakWeeks   = streak;
    v.weeksAchieved = static_cast<uint16_t>(streak + 3);
    v.sessions      = sessions;
    v.target        = target;
    v.daysLeft      = 2;
    v.mood          = mood;
    v.flags         = flags;
    return v;
}

/// The text's width in its face, or -1 for a face these tests do not know.
int widthOf(const Spec& s)
{
    const uint8_t* table = nullptr;
    switch (s.font) {
        case GLANCE_FONT_POPPINS_REGULAR_18: table = GlanceFontWidths::kRegular18; break;
        case GLANCE_FONT_POPPINS_MEDIUM_18: table = GlanceFontWidths::kMedium18; break;
        case GLANCE_FONT_POPPINS_SEMIBOLD_20: table = GlanceFontWidths::kSemiBold20; break;
        default: return -1;
    }
    int w = 0;
    for (const char* c = s.text; *c; ++c) {
        w += (*c >= 0x20 && *c <= 0x7E) ? table[*c - 0x20] : 0;
    }
    return w;
}

void expectFits(const Layout& l, int16_t w, int16_t h, uint32_t maxControls, const std::string& what)
{
    EXPECT_LE(l.count, maxControls) << what;
    for (uint8_t i = 0; i < l.count; ++i) {
        const Spec& s = l.items[i];
        if (s.type == Spec::Type::Line) {
            EXPECT_TRUE(s.x >= 0 && s.x < w && s.x2 >= 0 && s.x2 < w) << what << " line " << int(i);
            EXPECT_TRUE(s.y >= 0 && s.y < h && s.y2 >= 0 && s.y2 < h) << what << " line " << int(i);
        } else {
            EXPECT_GE(s.x, 0) << what;
            EXPECT_GE(s.y, 0) << what;
            EXPECT_LE(s.x + s.w, w) << what << " control " << int(i);
            EXPECT_LE(s.y + s.h, h) << what << " control " << int(i) << " '" << s.text << "'";
        }
        if (s.type == Spec::Type::Text) {
            EXPECT_GT(std::strlen(s.text), 0u) << what;
            EXPECT_LE(std::strlen(s.text), static_cast<size_t>(GLANCE_TEXT_SIZE)) << what;
            for (const char* c = s.text; *c; ++c) {
                EXPECT_TRUE(*c >= 0x20 && *c <= 0x7E) << what << " non-ASCII in '" << s.text << "'";
            }
            // The 10 point face is digits only; words in it drew as '?'.
            EXPECT_NE(s.font, GLANCE_FONT_POPPINS_MEDIUM_10) << what << " '" << s.text << "'";
            const int tw = widthOf(s);
            EXPECT_GE(tw, 0) << what << " unmeasured face " << int(s.font);
            // A line too wide for its box is clipped. The full layout (the
            // watch's 240x60) must show every word; the others are fallbacks
            // for areas no watch has reported.
            if (l.kind == Layout::Kind::Full) {
                EXPECT_LE(tw, s.w) << what << " '" << s.text << "' is " << tw << " px in " << s.w;
            }
        }
    }
}

} // namespace

TEST(GlanceLayout, FitsEveryAreaAndBudget)
{
    const struct { int16_t w, h; } kAreas[] = {{240, 60}, {240, 80}, {200, 60}, {220, 50}, {160, 60},
                                               {240, 44}, {120, 40}, {240, 30}};
    const uint32_t kBudgets[] = {32, 8, 7, 4, 2, 1};
    const Streak::HomeView kViews[] = {
        view(7, 2, 3, Streak::Mood::Climbing),
        view(123, 7, 7, Streak::Mood::Done),
        view(0, 1, 3, Streak::Mood::Trial, Streak::HomeView::kTrialWeek),
        view(0, 0, 3, Streak::Mood::Climbing),
        view(40, 2, 4, Streak::Mood::AtRisk),
        view(9, 0, 3, Streak::Mood::Climbing, Streak::HomeView::kDecisionPending),
        view(9, 0, 3, Streak::Mood::Climbing, Streak::HomeView::kClockUnset),
    };
    for (const auto& a : kAreas) {
        for (const uint32_t b : kBudgets) {
            for (const auto& v : kViews) {
                for (const State st : {State::Normal, State::NoStreak}) {
                    Layout l;
                    Glance::layout(v, st, a.w, a.h, b, l);
                    expectFits(l, a.w, a.h, b,
                               std::to_string(a.w) + "x" + std::to_string(a.h) + " budget " + std::to_string(b));
                }
            }
        }
    }
}

/// The full layout's mountain: 6 blocks, the flag pole and the flag.
constexpr int kMountain = 8;

/// The full layout's bottom line: its last control.
const Spec& bottom(const Layout& l) { return l.items[l.count - 1]; }

TEST(GlanceLayout, FitsEveryTargetAndClimb)
{
    // Every target 1..7 at every session count, and every step of every climb
    // (plus two repeat Everests), in the watch's 240x60 area.
    for (uint8_t target = 1; target <= 7; ++target) {
        for (uint8_t sessions = 0; sessions <= 9; ++sessions) {
            for (uint8_t days = 1; days <= 7; ++days) {
                for (const auto mood : {Streak::Mood::Climbing, Streak::Mood::AtRisk}) {
                    Streak::HomeView v = view(99, sessions, target, mood);
                    v.daysLeft         = days;
                    Layout l;
                    Glance::layout(v, State::Normal, 240, 60, 32, l);
                    ASSERT_EQ(l.kind, Layout::Kind::Full);
                    // The mountain, the head, a bead per session of the target, the line.
                    ASSERT_EQ(l.count, kMountain + 2 + target);
                    EXPECT_EQ(l.items[kMountain + target].type, Spec::Type::Rect) << int(target);
                    EXPECT_EQ(l.items[kMountain + 1 + target].type, Spec::Type::Text) << int(target);
                    expectFits(l, 240, 60, 32, "target " + std::to_string(target) + " sessions "
                                                   + std::to_string(sessions) + " days " + std::to_string(days));
                }
            }
        }
    }
    for (uint32_t weeks = 0; weeks < Streak::kClimbs[Streak::kClimbCount - 1].summitAt + 2 * Streak::kRepeatSteps;
         ++weeks) {
        Streak::HomeView v = view(1, 1, 3, Streak::Mood::Climbing);
        v.weeksAchieved    = static_cast<uint16_t>(weeks);
        Layout l;
        Glance::layout(v, State::Normal, 240, 60, 32, l);
        expectFits(l, 240, 60, 32, "weeks achieved " + std::to_string(weeks));
    }
}

TEST(GlanceLayout, FullLayoutHasTheMountainBeadsAndALine)
{
    Layout l;
    Glance::layout(view(7, 2, 3, Streak::Mood::Climbing), State::Normal, 240, 60, 32, l);
    ASSERT_EQ(l.kind, Layout::Kind::Full);
    ASSERT_EQ(l.count, kMountain + 5);   // the mountain, the head, 3 beads, the line
    EXPECT_STREQ(l.items[kMountain].text, "7 week streak");
    for (int i = kMountain + 1; i < kMountain + 4; ++i) {
        EXPECT_EQ(l.items[i].type, Spec::Type::Rect);
        EXPECT_EQ(l.items[i].colour, i < kMountain + 3 ? GLANCE_COLOR_GREEN : GLANCE_COLOR_GRAY) << i;
    }
    EXPECT_STREQ(bottom(l).text, "Snowdon: 2 wks");   // 10 weeks: 2 steps left to 12
    EXPECT_EQ(bottom(l).font, GLANCE_FONT_POPPINS_REGULAR_18);

    Streak::HomeView one = view(7, 2, 3, Streak::Mood::Climbing);
    one.weeksAchieved    = 11;
    Glance::layout(one, State::Normal, 240, 60, 32, l);
    EXPECT_STREQ(bottom(l).text, "Snowdon: 1 wk");
}

TEST(GlanceLayout, TheFullLayoutDrawsNoLines)
{
    // On the watch the line control drew only a faint stub (NOTES S4.3);
    // text and filled rectangles are the shapes known to work.
    for (uint8_t target = 1; target <= 7; ++target) {
        Layout l;
        Glance::layout(view(7, 2, target, Streak::Mood::Climbing), State::Normal, 240, 60, 32, l);
        ASSERT_EQ(l.kind, Layout::Kind::Full);
        for (uint8_t i = 0; i < l.count; ++i) {
            EXPECT_NE(l.items[i].type, Spec::Type::Line) << int(i);
        }
    }
}

TEST(GlanceLayout, TheMountainNarrowsToACentredSummit)
{
    Layout l;
    Glance::layout(view(7, 2, 3, Streak::Mood::Climbing), State::Normal, 240, 60, 32, l);
    ASSERT_EQ(l.kind, Layout::Kind::Full);
    int16_t prevTop = 1000;
    int16_t prevW   = 1000;
    for (int i = 0; i < 6; ++i) {   // the blocks, foot to summit
        const Spec& b = l.items[i];
        ASSERT_EQ(b.type, Spec::Type::Rect) << i;
        EXPECT_LT(b.y, prevTop) << "each block sits above the last " << i;
        EXPECT_LT(b.w, prevW) << "and is narrower " << i;
        EXPECT_EQ(b.x + b.w / 2, 4 + 26) << "centred under the summit " << i;
        prevTop = b.y;
        prevW   = b.w;
    }
    EXPECT_EQ(l.items[5].colour, GLANCE_COLOR_WHITE);   // the cap
    // The flag stands on the summit.
    EXPECT_EQ(l.items[6].y + l.items[6].h, l.items[5].y);
}

TEST(GlanceLayout, FewControlsGiveTheCompactLayout)
{
    Layout l;
    Glance::layout(view(7, 2, 3, Streak::Mood::Climbing), State::Normal, 240, 60, 4, l);
    EXPECT_EQ(l.kind, Layout::Kind::Compact);
    EXPECT_EQ(l.count, 2);
    EXPECT_STREQ(l.items[1].text, "2 of 3 this week");
    // A target of 7 needs 17 controls for the full layout.
    Glance::layout(view(7, 2, 7, Streak::Mood::Climbing), State::Normal, 240, 60, 16, l);
    EXPECT_EQ(l.kind, Layout::Kind::Compact);
    Glance::layout(view(7, 2, 7, Streak::Mood::Climbing), State::Normal, 240, 60, 17, l);
    EXPECT_EQ(l.kind, Layout::Kind::Full);
    Glance::layout(view(7, 2, 3, Streak::Mood::Climbing), State::Normal, 240, 30, 32, l);
    EXPECT_EQ(l.kind, Layout::Kind::Tiny);
    EXPECT_STREQ(l.items[0].text, "7 wk streak, 2/3");
}

TEST(GlanceLayout, TheWordsForEachState)
{
    Layout l;
    Glance::layout(view(40, 2, 4, Streak::Mood::AtRisk), State::Normal, 240, 60, 32, l);
    EXPECT_STREQ(bottom(l).text, "2 more, 2 days left");
    EXPECT_EQ(bottom(l).colour, GLANCE_COLOR_YELLOW_DARK);

    Streak::HomeView last = view(40, 3, 4, Streak::Mood::AtRisk);
    last.daysLeft         = 1;
    Glance::layout(last, State::Normal, 240, 60, 32, l);
    EXPECT_STREQ(bottom(l).text, "Last day: 1 more");

    Glance::layout(view(9, 0, 3, Streak::Mood::Climbing, Streak::HomeView::kDecisionPending), State::Normal, 240, 60,
                   32, l);
    EXPECT_STREQ(bottom(l).text, "Shield? Open app");

    Glance::layout(view(0, 1, 3, Streak::Mood::Trial, Streak::HomeView::kTrialWeek), State::Normal, 240, 60, 32, l);
    EXPECT_STREQ(l.items[kMountain].text, "First week");

    Glance::layout(view(5, 3, 3, Streak::Mood::Done), State::Normal, 240, 60, 32, l);
    EXPECT_EQ(l.items[kMountain + 3].colour, GLANCE_COLOR_GREEN);   // every bead done
    EXPECT_STREQ(bottom(l).text, "Banked. Rest up.");
    EXPECT_EQ(bottom(l).colour, GLANCE_COLOR_GREEN);

    // No beads when the week means nothing yet.
    Glance::layout(view(5, 3, 3, Streak::Mood::Done, Streak::HomeView::kClockUnset), State::Normal, 240, 60, 32, l);
    EXPECT_EQ(l.count, kMountain + 2);
    EXPECT_STREQ(l.items[kMountain].text, "Set the time");
    EXPECT_STREQ(bottom(l).text, "in the UNA app");

    Glance::layout(Streak::HomeView {}, State::NoStreak, 240, 60, 32, l);
    EXPECT_EQ(l.count, kMountain + 2);
    EXPECT_STREQ(bottom(l).text, "Open it to start");
}
