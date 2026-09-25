/**
 ******************************************************************************
 * @file    Goal.hpp
 * @brief   The athlete's weekly goal (PLAN 6.3). Header-only: the GUI uses it.
 ******************************************************************************
 */

#ifndef STREAK_GOAL_HPP
#define STREAK_GOAL_HPP

#include <cstdint>
#include <type_traits>

#include "StreakTypes.hpp"

namespace Streak
{

constexpr uint8_t kMaxMinMinutes = 120;

struct Goal {
    uint8_t target     = 3;           ///< sessions a week, 1..7 (S1)
    uint8_t weekStart  = 1;           ///< 0 = Sunday .. 6; default Monday (S1)
    uint8_t scope      = kScopeAny;   ///< kScopeAny or a Kind (S2)
    uint8_t minMinutes = 10;          ///< 0 = off (S13)
    bool    onePerDay  = false;       ///< S14

    bool operator==(const Goal& o) const
    {
        return target == o.target && weekStart == o.weekStart && scope == o.scope && minMinutes == o.minMinutes
               && onePerDay == o.onePerDay;
    }
    bool operator!=(const Goal& o) const { return !(*this == o); }
    /// Clamp every field into range (settings can arrive from a hand-edited file).
    Goal sane() const
    {
        Goal g       = *this;
        g.target     = target < 1 ? 1 : (target > 7 ? 7 : target);
        g.weekStart  = static_cast<uint8_t>(weekStart % 7);
        g.scope      = (scope == kScopeAny || scope < kKindCount) ? scope : kScopeAny;
        g.minMinutes = minMinutes > kMaxMinMinutes ? kMaxMinMinutes : minMinutes;
        return g;
    }
};

static_assert(std::is_trivially_copyable<Goal>::value, "Goal travels inside kernel messages");

} // namespace Streak

#endif // STREAK_GOAL_HPP
