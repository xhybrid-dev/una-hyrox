/**
 ******************************************************************************
 * @file    Demo.cpp
 * @brief   The design demo's scenarios (see the header).
 ******************************************************************************
 */

#include "gui/demo/Demo.hpp"

namespace Demo
{

namespace
{
using Streak::HomeView;
using Streak::Mood;

// weeksAchieved, streakWeeks, target, sessions, daysLeft, shields, lastWeek, mood
constexpr HomeView view(uint16_t weeks, uint16_t streak, uint8_t target, uint8_t sessions,
                        uint8_t daysLeft, uint8_t shields, uint8_t lastWeek, Mood mood)
{
    HomeView v {};
    v.weeksAchieved = weeks;
    v.streakWeeks   = streak;
    v.target        = target;
    v.sessions      = sessions;
    v.daysLeft      = daysLeft;
    v.shields       = shields;
    v.lastWeek      = lastWeek;
    v.mood          = mood;
    return v;
}
} // namespace

const Scenario kScenarios[] = {
    // At the foot of Arthur's Seat, one run in.
    { "First week",   view(0,  0, 3, 1, 5, 0, 0, Mood::Trial),    ScreenId::Home,       Coach::Sport::Run,      32 },
    // Snowdon, a week in hand: R1 brings the session that banks the week.
    { "Mid-week",     view(7,  7, 3, 2, 3, 1, 3, Mood::Climbing), ScreenId::Home,       Coach::Sport::Strength, 45 },
    // Ben Nevis, week already banked: R1 is a bonus session.
    { "Week banked",  view(18, 11, 3, 3, 2, 2, 3, Mood::Done),    ScreenId::Home,       Coach::Sport::Ride,     64 },
    // Mont Blanc, two to go and two days left: amber, but encouraging.
    { "At risk",      view(40, 23, 4, 2, 2, 2, 4, Mood::AtRisk),  ScreenId::Home,       Coach::Sport::Run,      28 },
    // One step below the top of Ben Nevis: R1 banks the week and summits.
    { "Summit day",   view(25, 20, 3, 2, 2, 2, 3, Mood::Climbing),ScreenId::Home,       Coach::Sport::Hybrid,   71 },
    // Deep into Everest, the long climb: a banked week.
    { "Everest",      view(80, 61, 4, 4, 3, 2, 4, Mood::Done),    ScreenId::Home,       Coach::Sport::Workout,  50 },
    // A week missed with shields in hand.
    { "Missed week",  view(30,  9, 3, 0, 7, 2, 1, Mood::Climbing),ScreenId::Shield,     Coach::Sport::Run,      30 },
    // The streak ended; the climb is safe.
    { "Fresh start",  view(18,  0, 3, 0, 7, 0, 1, Mood::Climbing),ScreenId::FreshStart, Coach::Sport::Run,      30 },
};

const uint8_t kScenarioCount = sizeof(kScenarios) / sizeof(kScenarios[0]);

} // namespace Demo
