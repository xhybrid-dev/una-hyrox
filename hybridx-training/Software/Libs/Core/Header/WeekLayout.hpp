/**
 ******************************************************************************
 * @file    WeekLayout.hpp
 * @brief   Placing a week's four slots on the runner's own days
 *          (PLANS.md section 4.2).
 *
 * The runner chooses the days they can run (3 to 6) and the long-run day.
 * place() decides which day holds which slot:
 *   1. the long run on its day (L);
 *   2. hard days never next to each other; quality never the day after L, and
 *      the day before L only when nothing else is possible;
 *   3. with 4 or more run days, the two quality days are the pair that keeps
 *      hard days furthest apart round the week (a tie: the pair whose first
 *      day comes soonest after L), never using the day before L; with no such
 *      pair there is one quality day and the second quality becomes an easy run
 *      with strides;
 *   4. with 3 run days, one quality day: the allowed run day furthest from L,
 *      holding the week's key session;
 *   5. the other days easy.
 *
 * sessions() then fills each day from a plan week.
 ******************************************************************************
 */

#ifndef XTRAINING_WEEK_LAYOUT_HPP
#define XTRAINING_WEEK_LAYOUT_HPP

#include "PlanTypes.hpp"

#include <cstdint>

namespace XTraining {

/// Bit d set: the runner can run on weekday d (0 Monday .. 6 Sunday).
using DayMask = uint8_t;

enum class Role : uint8_t {
    Rest = 0,
    Quality1,  ///< The first-quality slot.
    Quality2,  ///< The second-quality slot.
    KeyOnly,   ///< The one quality day of a week that has only one.
    Easy,      ///< The easy slot.
    Demoted,   ///< The second quality slot as an easy run with strides.
    Extra,     ///< An extra easy run (5 or 6 run days).
    Long,
};

struct Layout {
    bool ok = false;          ///< False if the days given cannot hold a plan.
    Role role[7] = {};
};

namespace WeekLayout {

constexpr uint8_t kMinDays = 3;
constexpr uint8_t kMaxDays = 6;
/// The minutes of an extra easy run, at level 2 (PlanBuilder scales it).
constexpr uint16_t kExtraEasyMinutes = 35;

constexpr uint8_t dayCount(DayMask m)
{
    uint8_t n = 0;
    for (uint8_t d = 0; d < 7; ++d) {
        n = static_cast<uint8_t>(n + ((m >> d) & 1u));
    }
    return n;
}

/// The default days: Tuesday, Wednesday, Friday and Sunday.
constexpr DayMask kDefaultDays = (1u << 1) | (1u << 2) | (1u << 4) | (1u << 6);
constexpr uint8_t kDefaultLongDay = 6;

/// True if @p days has 3 to 6 days, @p longDay among them.
bool valid(DayMask days, uint8_t longDay);

/// Which day holds which slot. With @p allowSeven, 7 days are accepted (a race
/// week, where the race day is added to the runner's days).
Layout place(DayMask days, uint8_t longDay, bool allowSeven = false);

/// Each day's session from @p week, by @p layout (index 0 is Monday). Rest
/// days are Kind::Rest.
void sessions(const Week& week, const Layout& layout, Session out[7]);

}  // namespace WeekLayout
}  // namespace XTraining

#endif  // XTRAINING_WEEK_LAYOUT_HPP
