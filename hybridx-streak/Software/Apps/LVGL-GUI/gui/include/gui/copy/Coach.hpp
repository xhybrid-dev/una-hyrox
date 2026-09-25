/**
 ******************************************************************************
 * @file    Coach.hpp
 * @brief   Every sentence the home screen says, in the coach's voice.
 *
 * Encouraging, never guilt-tripping, British English (docs/DESIGN.md "Voice").
 * Pure C++ with fixed buffers and integer formatting, so the host tests can
 * check every line and its length: the bottom of a round screen is narrow,
 * and a line that fits at y = 120 is clipped at y = 210.
 ******************************************************************************
 */

#ifndef STREAK_COACH_HPP
#define STREAK_COACH_HPP

#include <cstddef>
#include <cstdint>

#include "StreakTypes.hpp"
#include "StreakView.hpp"

namespace Coach
{

/// The longest coach line, in characters, that fits the bottom of the screen
/// in Regular 14: the disc is about 170 px wide there, and 22 characters
/// lost their last letter to the edge in the simulator (DESIGN.md "Layout").
constexpr size_t kMaxCoachChars = 21;

/// The headline, set as a big lime number beside smaller words: "7" +
/// "week streak". With no streak the number is empty and the words say
/// "Your first week" (a trial week) or "Start a new streak".
struct Headline {
    char number[8];
    char words[24];
};
void headline(const Streak::HomeView& v, Headline& out);

/// "Ben Nevis · 8 weeks to go", "Everest again · 30 weeks to go".
void mountainLine(const Streak::HomeView& v, char* out, size_t size);

/// "1 more · 3 days left", "Last day: 1 to go", "Week banked. Rest up."
void coachLine(const Streak::HomeView& v, char* out, size_t size);

/// The kinds of session (Core StreakTypes.hpp); the Coach names them.
using Sport = Streak::Kind;

/// "+1 Run · 42 min"; a manual log (0 minutes) is "+1 Row · logged".
void sessionToast(Sport sport, uint16_t minutes, char* out, size_t size);

/// "Run", "Strength", ... ("Session" for Other).
const char* sportName(Sport sport);
/// "Runs only", "Rides only", ... for the goal's scope; "Everything" for any.
const char* scopeName(uint8_t scope);

/// Toasts for the moments that are not a session (all within kMaxCoachChars).
void badgeToast(uint8_t badge, char* out, size_t size);        ///< "Trailhead badge!"
void bestWeekToast(uint8_t sessions, char* out, size_t size);   ///< "Best week yet: 5!"
void shieldToast(uint8_t shields, char* out, size_t size);      ///< "+1 shield · 2 held"
void lastWeekToast(uint8_t count, uint8_t target, char* out, size_t size);   ///< "Last week: 2 of 3"

/// An app folder as the athlete knows it: "HybridXRace" -> "HybridX".
void appName(const char* folder, char* out, size_t size);
/// "Mon", "Tue", ... for a weekday (0 = Sunday).
const char* dayShort(uint8_t weekday);
/// "Monday", ... for a weekday (0 = Sunday).
const char* dayLong(uint8_t weekday);

} // namespace Coach

#endif // STREAK_COACH_HPP
