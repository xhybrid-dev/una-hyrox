/**
 * Host tests for the glance layout (PLAN 8): whatever area and control budget
 * the watch reports, every control fits inside it, the budget is kept, and
 * every text fits a glance text control.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "GlanceLayout.hpp"

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

TEST(GlanceLayout, FullLayoutHasTheMountainAndThreeLines)
{
    Layout l;
    Glance::layout(view(7, 2, 3, Streak::Mood::Climbing), State::Normal, 240, 60, 32, l);
    ASSERT_EQ(l.kind, Layout::Kind::Full);
    EXPECT_EQ(l.count, 8);
    EXPECT_STREQ(l.items[5].text, "7 week streak");
    EXPECT_STREQ(l.items[6].text, "2 of 3 this week");
    EXPECT_STREQ(l.items[7].text, "Snowdon: 2 weeks to go");   // 10 weeks: 2 steps left to 12
}

TEST(GlanceLayout, FewControlsGiveTheCompactLayout)
{
    Layout l;
    Glance::layout(view(7, 2, 3, Streak::Mood::Climbing), State::Normal, 240, 60, 4, l);
    EXPECT_EQ(l.kind, Layout::Kind::Compact);
    EXPECT_EQ(l.count, 2);
    Glance::layout(view(7, 2, 3, Streak::Mood::Climbing), State::Normal, 240, 30, 32, l);
    EXPECT_EQ(l.kind, Layout::Kind::Tiny);
    EXPECT_STREQ(l.items[0].text, "7 wk streak \xC2\xB7 2/3");
}

TEST(GlanceLayout, TheWordsForEachState)
{
    Layout l;
    Glance::layout(view(40, 2, 4, Streak::Mood::AtRisk), State::Normal, 240, 60, 32, l);
    EXPECT_STREQ(l.items[7].text, "2 more in 2 days");
    EXPECT_EQ(l.items[7].colour, GLANCE_COLOR_YELLOW_DARK);

    Glance::layout(view(9, 0, 3, Streak::Mood::Climbing, Streak::HomeView::kDecisionPending), State::Normal, 240, 60,
                   32, l);
    EXPECT_STREQ(l.items[7].text, "Open to use a shield");

    Glance::layout(view(0, 1, 3, Streak::Mood::Trial, Streak::HomeView::kTrialWeek), State::Normal, 240, 60, 32, l);
    EXPECT_STREQ(l.items[5].text, "First week");

    Glance::layout(view(5, 3, 3, Streak::Mood::Done), State::Normal, 240, 60, 32, l);
    EXPECT_EQ(l.items[6].colour, GLANCE_COLOR_GREEN);
    EXPECT_STREQ(l.items[7].text, "Week banked. Rest up.");

    Glance::layout(view(5, 3, 3, Streak::Mood::Done, Streak::HomeView::kClockUnset), State::Normal, 240, 60, 32, l);
    EXPECT_STREQ(l.items[5].text, "Set the time");

    Glance::layout(Streak::HomeView {}, State::NoStreak, 240, 60, 32, l);
    EXPECT_STREQ(l.items[6].text, "Open it to start");
}
