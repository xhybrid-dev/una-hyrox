/**
 ******************************************************************************
 * @file    Coach.cpp
 * @brief   Every sentence the home screen says (see the header).
 ******************************************************************************
 */

#include "gui/copy/Coach.hpp"

#include <cstdio>
#include <cstring>

#include "Summits.hpp"

namespace Coach
{

namespace
{
/// U+00B7 MIDDLE DOT, which every text face carries.
constexpr const char* kDot = "\xC2\xB7";

unsigned u(uint32_t v) { return static_cast<unsigned>(v); }
} // namespace

void headline(const Streak::HomeView& v, Headline& out)
{
    if (v.streakWeeks > 0) {
        snprintf(out.number, sizeof(out.number), "%u", u(v.streakWeeks));
        snprintf(out.words, sizeof(out.words), "week streak");
    } else {
        out.number[0] = '\0';
        snprintf(out.words, sizeof(out.words), "%s",
                 v.mood == Streak::Mood::Trial ? "Your first week" : "Start a new streak");
    }
}

void mountainLine(const Streak::HomeView& v, char* out, size_t size)
{
    const Streak::ClimbPosition pos = Streak::climbFor(v.weeksAchieved);
    const Streak::Climb&        c   = Streak::kClimbs[pos.climb];
    const uint32_t              left = static_cast<uint32_t>(pos.steps - pos.stepsClimbed);
    const char*                 again = pos.ascent > 1 ? " again" : "";
    snprintf(out, size, "%s%s %s %u week%s to go", c.name, again, kDot, u(left), left == 1 ? "" : "s");
}

void coachLine(const Streak::HomeView& v, char* out, size_t size)
{
    const uint32_t left = v.sessions >= v.target ? 0u : static_cast<uint32_t>(v.target - v.sessions);

    if (left == 0) {
        const uint32_t extra = static_cast<uint32_t>(v.sessions - v.target);
        if (extra == 0) {
            snprintf(out, size, "Week banked. Rest up.");
        } else {
            snprintf(out, size, "Week banked, +%u bonus", u(extra));
        }
        return;
    }

    if (v.mood == Streak::Mood::Trial) {
        snprintf(out, size, "Week one, no pressure");
        return;
    }

    if (v.daysLeft <= 1) {
        snprintf(out, size, "Last day: %u to go", u(left));
    } else if (v.mood == Streak::Mood::AtRisk) {
        snprintf(out, size, "%u more in %u days. Go!", u(left), u(v.daysLeft));
    } else {
        snprintf(out, size, "%u more %s %u days left", u(left), kDot, u(v.daysLeft));
    }
}

const char* sportName(Sport sport)
{
    switch (sport) {
        case Sport::Run:      return "Run";
        case Sport::Ride:     return "Ride";
        case Sport::Walk:     return "Walk";
        case Sport::Strength: return "Strength";
        case Sport::Workout:  return "Workout";
        case Sport::Hybrid:   return "Hybrid";
        case Sport::Row:      return "Row";
        case Sport::Other:    return "Session";
    }
    return "Session";
}

void sessionToast(Sport sport, uint16_t minutes, char* out, size_t size)
{
    if (minutes == 0) {
        snprintf(out, size, "+1 %s %s logged", sportName(sport), kDot);
        return;
    }
    snprintf(out, size, "+1 %s %s %u min", sportName(sport), kDot, u(minutes));
}

const char* scopeName(uint8_t scope)
{
    switch (scope) {
        case static_cast<uint8_t>(Sport::Run):      return "Runs only";
        case static_cast<uint8_t>(Sport::Ride):     return "Rides only";
        case static_cast<uint8_t>(Sport::Walk):     return "Walks only";
        case static_cast<uint8_t>(Sport::Strength): return "Strength only";
        case static_cast<uint8_t>(Sport::Workout):  return "Workouts only";
        case static_cast<uint8_t>(Sport::Hybrid):   return "Hybrid only";
        case static_cast<uint8_t>(Sport::Row):      return "Rows only";
        case static_cast<uint8_t>(Sport::Other):    return "Other only";
        default:                                    return "Everything";
    }
}

void badgeToast(uint8_t badge, char* out, size_t size)
{
    const char* name = badge < sizeof(Streak::kSessionBadges) / sizeof(Streak::kSessionBadges[0])
                           ? Streak::kSessionBadges[badge].name
                           : "New";
    snprintf(out, size, "%s badge!", name);
}

void bestWeekToast(uint8_t sessions, char* out, size_t size)
{
    snprintf(out, size, "Best week yet: %u!", u(sessions));
}

void shieldToast(uint8_t shields, char* out, size_t size)
{
    snprintf(out, size, "+1 shield %s %u held", kDot, u(shields));
}

void lastWeekToast(uint8_t count, uint8_t target, char* out, size_t size)
{
    snprintf(out, size, "Last week: %u of %u", u(count), u(target));
}

void appName(const char* folder, char* out, size_t size)
{
    if (std::strcmp(folder, "HybridXRace") == 0) {
        snprintf(out, size, "HybridX");   // "HybridX Race" clips on the wheel's lower line
    } else {
        snprintf(out, size, "%s", folder);
    }
}

const char* dayShort(uint8_t weekday)
{
    static const char* const kDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return kDays[weekday % 7];
}

const char* dayLong(uint8_t weekday)
{
    static const char* const kDays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    return kDays[weekday % 7];
}

} // namespace Coach
