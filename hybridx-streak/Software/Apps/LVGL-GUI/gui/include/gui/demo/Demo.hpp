/**
 ******************************************************************************
 * @file    Demo.hpp
 * @brief   The design demo's scenarios (HYBRIDXSTREAK_DEMO builds only).
 *
 * Each scenario is a moment in an athlete's year, chosen to show one part of
 * the design: the first week, a normal week, a banked week, a week at risk,
 * summit day, a missed week and a fresh start. In the demo L1/L2 step through
 * them and R1 plays the moment: a new session arriving, or the choice on the
 * shield screen.
 ******************************************************************************
 */

#ifndef STREAK_DEMO_HPP
#define STREAK_DEMO_HPP

#include <cstdint>

#include "StreakView.hpp"
#include "gui/copy/Coach.hpp"
#include "gui/screens/ScreenManager.hpp"

namespace Demo
{

struct Scenario {
    const char*      name;
    Streak::HomeView view;
    ScreenId         screen;
    Coach::Sport     sport;     ///< the session R1 brings in on the home screen
    uint16_t         minutes;
};

extern const Scenario kScenarios[];
extern const uint8_t  kScenarioCount;

} // namespace Demo

#endif // STREAK_DEMO_HPP
