/**
 ******************************************************************************
 * @file    Progress.hpp
 * @brief   Levels: the runner's stats at each level, a spirit's stats at its
 *          level, the XP curve, and what partners add.
 *
 * Header-only (the GUI shows levels and stats). Integer maths. First-draft
 * numbers, tuned with Tests/Host/tools/run_sim and balance_sim.
 ******************************************************************************
 */

#ifndef TRAILQUEST_PROGRESS_HPP
#define TRAILQUEST_PROGRESS_HPP

#include <cstdint>

#include "Types.hpp"
#include "data/Species.hpp"

namespace TrailQuest
{

constexpr uint8_t kMinLevel = 1;
constexpr uint8_t kMaxLevel = 20;

/// The runner's base stamina and stats at level 1, and what each level adds.
constexpr uint16_t kRunnerStamina      = 40;
constexpr uint16_t kRunnerStaminaGrow  = 4;
constexpr uint8_t  kRunnerStat         = 10;
constexpr uint8_t  kRunnerStatGrow     = 1;

inline uint8_t clampLevel(uint8_t level)
{
    return level < kMinLevel ? kMinLevel : (level > kMaxLevel ? kMaxLevel : level);
}

/// Scale @p value for @p level by (9 + level) / 10, the same factor the
/// runner's stamina grows by (40 + 4(L-1) = 4 x (9 + L)). Damage, recovery and
/// drain all use it, so a fight takes about as many turns at level 14 as at
/// level 1. Exactly 1 at level 1. At least 1 for any value above 0.
inline uint32_t levelScale(uint32_t value, uint8_t level)
{
    if (value == 0u) {
        return 0u;
    }
    const uint32_t v = value * (9u + clampLevel(level)) / 10u;
    return v < 1u ? 1u : v;
}

/// The runner's stats at @p level (no partners).
inline Stats runnerStats(uint8_t level)
{
    const uint8_t l = clampLevel(level);
    Stats s;
    s.stamina = static_cast<uint16_t>(kRunnerStamina + kRunnerStaminaGrow * (l - 1u));
    s.pace    = static_cast<uint8_t>(kRunnerStat + kRunnerStatGrow * (l - 1u));
    s.form    = s.pace;
    s.grit    = s.pace;
    return s;
}

namespace detail
{
inline uint8_t offsetStat(uint8_t base, int8_t offset)
{
    const int v = static_cast<int>(base) + offset;
    return static_cast<uint8_t>(v < 1 ? 1 : (v > 255 ? 255 : v));
}
} // namespace detail

/// A spirit's stats at @p level: the runner's at that level, adjusted by the
/// spirit's row (Species.hpp).
inline Stats speciesStats(const SpeciesDef& sp, uint8_t level)
{
    const Stats r = runnerStats(level);
    Stats s;
    const uint32_t stamina = static_cast<uint32_t>(r.stamina) * sp.staminaPct / 100u;
    s.stamina = static_cast<uint16_t>(stamina < 1u ? 1u : (stamina > 999u ? 999u : stamina));
    s.pace    = detail::offsetStat(r.pace, sp.pace);
    s.form    = detail::offsetStat(r.form, sp.form);
    s.grit    = detail::offsetStat(r.grit, sp.grit);
    return s;
}

/// XP needed to go from @p level to the next; 0 at the top. Set so a stage's
/// three or so duels and its boss give about three levels, the step between
/// the stages' par levels (docs/BALANCE.md, the run simulator).
inline uint16_t xpToNext(uint8_t level)
{
    const uint8_t l = clampLevel(level);
    return l >= kMaxLevel ? 0u : static_cast<uint16_t>(8u + 4u * l);
}

/// XP for beating @p sp at @p level.
inline uint16_t xpForWin(const SpeciesDef& sp, uint8_t level)
{
    const uint16_t base = static_cast<uint16_t>(6u + 2u * clampLevel(level));
    return (sp.flags & kSpeciesBoss) != 0 ? static_cast<uint16_t>(base * 3u) : base;
}

/// XP for clearing an obstacle.
constexpr uint16_t kXpObstacle = 3;

/// Each partner adds this much to the stat of its own affinity.
constexpr uint8_t kPartnerBonus = 1;

} // namespace TrailQuest

#endif // TRAILQUEST_PROGRESS_HPP
