// The coach's lines: the right words, and short enough for a round screen.

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "gui/copy/Coach.hpp"

using Streak::HomeView;
using Streak::Mood;

namespace
{

std::string coach(const HomeView& v)
{
    char buf[64];
    Coach::coachLine(v, buf, sizeof(buf));
    return buf;
}

/// Characters as the eye counts them: the UTF-8 middle dot is one.
size_t visibleChars(const std::string& s)
{
    size_t n = 0;
    for (unsigned char c : s) {
        n += (c & 0xC0) != 0x80;
    }
    return n;
}

HomeView view(uint8_t target, uint8_t sessions, uint8_t daysLeft, Mood mood, uint16_t streak = 7)
{
    HomeView v;
    v.target      = target;
    v.sessions    = sessions;
    v.daysLeft    = daysLeft;
    v.mood        = mood;
    v.streakWeeks = streak;
    return v;
}

} // namespace

TEST(Coach, Headlines)
{
    Coach::Headline h;
    Coach::headline(view(3, 1, 4, Mood::Climbing, 7), h);
    EXPECT_STREQ(h.number, "7");
    EXPECT_STREQ(h.words, "week streak");
    Coach::headline(view(3, 1, 4, Mood::Climbing, 104), h);
    EXPECT_STREQ(h.number, "104");
    Coach::headline(view(3, 1, 4, Mood::Trial, 0), h);
    EXPECT_STREQ(h.number, "");
    EXPECT_STREQ(h.words, "Your first week");
    Coach::headline(view(3, 1, 4, Mood::Climbing, 0), h);
    EXPECT_STREQ(h.number, "");
    EXPECT_STREQ(h.words, "Start a new streak");
}

TEST(Coach, InProgress)
{
    EXPECT_EQ(coach(view(3, 2, 3, Mood::Climbing)), "1 more \xC2\xB7 3 days left");
    EXPECT_EQ(coach(view(4, 2, 2, Mood::AtRisk)), "2 more in 2 days. Go!");
    EXPECT_EQ(coach(view(3, 2, 1, Mood::AtRisk)), "Last day: 1 to go");
}

TEST(Coach, Done)
{
    EXPECT_EQ(coach(view(3, 3, 4, Mood::Done)), "Week banked. Rest up.");
    EXPECT_EQ(coach(view(3, 5, 1, Mood::Done)), "Week banked, +2 bonus");
}

TEST(Coach, Trial)
{
    EXPECT_EQ(coach(view(3, 1, 5, Mood::Trial, 0)), "Week one, no pressure");
}

TEST(Coach, EveryLineFitsTheBottomOfTheScreen)
{
    for (uint8_t target = 1; target <= 7; ++target) {
        for (uint8_t sessions = 0; sessions <= 9; ++sessions) {
            for (uint8_t days = 1; days <= 7; ++days) {
                for (Mood mood : { Mood::Trial, Mood::Climbing, Mood::AtRisk, Mood::Done }) {
                    const std::string s = coach(view(target, sessions, days, mood));
                    EXPECT_LE(visibleChars(s), Coach::kMaxCoachChars) << s;
                }
            }
        }
    }
}

TEST(Coach, MountainLine)
{
    char buf[48];
    HomeView v;
    v.weeksAchieved = 18;   // Ben Nevis, 6 of 14
    Coach::mountainLine(v, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Ben Nevis \xC2\xB7 8 weeks to go");
    v.weeksAchieved = 25;
    Coach::mountainLine(v, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Ben Nevis \xC2\xB7 1 week to go");
    v.weeksAchieved = 110;
    Coach::mountainLine(v, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Everest again \xC2\xB7 46 weeks to go");
}

TEST(Coach, Toast)
{
    char buf[32];
    Coach::sessionToast(Coach::Sport::Run, 42, buf, sizeof(buf));
    EXPECT_STREQ(buf, "+1 Run \xC2\xB7 42 min");
}

TEST(Coach, EveryToastFitsTheCoachLine)
{
    char buf[64];
    for (uint8_t k = 0; k < Streak::kKindCount; ++k) {
        Coach::sessionToast(static_cast<Coach::Sport>(k), 999, buf, sizeof(buf));
        EXPECT_LE(visibleChars(buf), Coach::kMaxCoachChars) << buf;
        Coach::sessionToast(static_cast<Coach::Sport>(k), 0, buf, sizeof(buf));
        EXPECT_LE(visibleChars(buf), Coach::kMaxCoachChars) << buf;
    }
    for (uint8_t b = 0; b < 4; ++b) {
        Coach::badgeToast(b, buf, sizeof(buf));
        EXPECT_LE(visibleChars(buf), Coach::kMaxCoachChars) << buf;
    }
    Coach::bestWeekToast(99, buf, sizeof(buf));
    EXPECT_LE(visibleChars(buf), Coach::kMaxCoachChars) << buf;
    Coach::shieldToast(2, buf, sizeof(buf));
    EXPECT_LE(visibleChars(buf), Coach::kMaxCoachChars) << buf;
    Coach::lastWeekToast(12, 7, buf, sizeof(buf));
    EXPECT_LE(visibleChars(buf), Coach::kMaxCoachChars) << buf;
    Coach::sessionToast(Coach::Sport::Row, 0, buf, sizeof(buf));
    EXPECT_STREQ(buf, "+1 Row \xC2\xB7 logged");
}

TEST(Coach, NamesForTheScreens)
{
    char buf[24];
    Coach::appName("HybridXRace", buf, sizeof(buf));
    EXPECT_STREQ(buf, "HybridX");
    Coach::appName("Running", buf, sizeof(buf));
    EXPECT_STREQ(buf, "Running");
    EXPECT_STREQ(Coach::scopeName(Streak::kScopeAny), "Everything");
    EXPECT_STREQ(Coach::scopeName(static_cast<uint8_t>(Streak::Kind::Run)), "Runs only");
    EXPECT_STREQ(Coach::dayShort(1), "Mon");
    EXPECT_STREQ(Coach::dayLong(0), "Sunday");
}
