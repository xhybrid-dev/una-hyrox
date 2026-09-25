/**
 ******************************************************************************
 * @file    StreakView.hpp
 * @brief   What the home screen shows: plain data, no SDK types.
 *
 * The service computes it (from phase S2; the design demo fabricates it in
 * S0) and sends it to the GUI inside a message, so it must stay small and
 * trivially copyable.
 ******************************************************************************
 */

#ifndef STREAK_VIEW_HPP
#define STREAK_VIEW_HPP

#include <cstdint>
#include <type_traits>

namespace Streak
{

/// The tone of this week, which picks the coach's line and its colour.
enum class Mood : uint8_t {
    Trial,      ///< first week of a goal: counts if met, can't break the streak
    Climbing,   ///< in progress with time in hand
    AtRisk,     ///< not met, and the sessions left are close to the days left
    Done,       ///< target met this week
};

struct HomeView {
    uint16_t weeksAchieved = 0;   ///< cumulative, drives the mountain (Summits.hpp)
    uint16_t streakWeeks   = 0;   ///< consecutive, including this week once met
    uint8_t  target        = 3;   ///< sessions a week, 1..7
    uint8_t  sessions      = 0;   ///< qualifying sessions this week
    uint8_t  daysLeft      = 7;   ///< including today, 1..7
    uint8_t  shields       = 0;   ///< 0..2
    uint8_t  lastWeek      = 0;   ///< qualifying sessions last week, for the boundary screens
    Mood     mood          = Mood::Climbing;
    uint8_t  flags         = 0;   ///< kClockUnset | kDecisionPending | kTrialWeek

    static constexpr uint8_t kClockUnset     = 0x01;   ///< the watch has lost the time: nothing is judged
    static constexpr uint8_t kDecisionPending = 0x02;  ///< a shield offer is waiting for an answer
    static constexpr uint8_t kTrialWeek       = 0x04;  ///< this week is a trial (S9), met or not
};

static_assert(std::is_trivially_copyable<HomeView>::value, "HomeView travels inside a kernel message");

} // namespace Streak

#endif // STREAK_VIEW_HPP
