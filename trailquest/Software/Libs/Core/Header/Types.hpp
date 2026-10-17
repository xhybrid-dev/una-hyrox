/**
 ******************************************************************************
 * @file    Types.hpp
 * @brief   The small shared vocabulary of the game: affinities and the wheel,
 *          stats, status effects, terrain, weather, time of day, and ids.
 *
 * Header-only (the GUI shows these). Plain integers, no floating point.
 ******************************************************************************
 */

#ifndef TRAILQUEST_TYPES_HPP
#define TRAILQUEST_TYPES_HPP

#include <cstdint>

namespace TrailQuest
{

/// "None" for any 8-bit id or index.
constexpr uint8_t kNone = 0xFF;

using SpeciesId = uint8_t;
using MoveId    = uint8_t;
using ItemId    = uint8_t;

/// What a move (or a Trail Spirit) leans on. The wheel: Pace beats Form, Form
/// beats Grit, Grit beats Pace. A move's damage scales on the matching stat of
/// both sides, so each stat matters for its own kind of move.
enum class Affinity : uint8_t {
    Pace = 0,
    Form,
    Grit,
    Count
};

constexpr uint8_t kAffinityCount = static_cast<uint8_t>(Affinity::Count);

/// +1 if @p attack beats @p defend on the wheel, -1 if it loses, 0 otherwise.
inline int8_t wheel(Affinity attack, Affinity defend)
{
    const uint8_t a = static_cast<uint8_t>(attack);
    const uint8_t d = static_cast<uint8_t>(defend);
    if (a >= kAffinityCount || d >= kAffinityCount || a == d) {
        return 0;
    }
    // Pace(0) beats Form(1), Form(1) beats Grit(2), Grit(2) beats Pace(0).
    return ((a + 1u) % kAffinityCount == d) ? 1 : -1;
}

/// The four numbers of a fighter.
struct Stats {
    uint16_t stamina = 0;   ///< Health: at 0 you are spent.
    uint8_t  pace    = 0;   ///< Who acts first, how often a move lands, Pace moves.
    uint8_t  form    = 0;   ///< Form moves.
    uint8_t  grit    = 0;   ///< Grit moves, and the finishing bonus.
};

/// The stat a move of @p a scales on.
inline uint8_t statFor(const Stats& s, Affinity a)
{
    switch (a) {
        case Affinity::Pace: return s.pace;
        case Affinity::Form: return s.form;
        case Affinity::Grit: return s.grit;
        default:             return 0;
    }
}

/// A status effect. Lasts kEffectTurns turns; one at a time per fighter.
enum class Effect : uint8_t {
    None = 0,
    Drain,     ///< Loses kDrainPerTurn stamina at the end of each turn.
    Slow,      ///< Pace down by kEffectStatDrop.
    Rattle,    ///< Grit down by kEffectStatDrop.
    Count
};

constexpr uint8_t kEffectTurns   = 3;
constexpr uint8_t kDrainPerTurn  = 2;
constexpr uint8_t kEffectStatDrop = 2;

/// The ground a stage is run on. Each favours one affinity.
enum class Terrain : uint8_t {
    Path = 0,   ///< Valley paths: Form.
    Climb,      ///< Switchbacks: Grit.
    Scree,      ///< Loose rock: Form.
    Ridge,      ///< Exposed ridge: Pace.
    Snow,       ///< The summit: Grit.
    Count
};

inline Affinity favouredBy(Terrain t)
{
    switch (t) {
        case Terrain::Path:  return Affinity::Form;
        case Terrain::Climb: return Affinity::Grit;
        case Terrain::Scree: return Affinity::Form;
        case Terrain::Ridge: return Affinity::Pace;
        case Terrain::Snow:  return Affinity::Grit;
        default:             return Affinity::Form;
    }
}

/// Weather for a stage. It changes the runner's numbers in a duel.
enum class Weather : uint8_t {
    Clear = 0,
    Rain,    ///< Form moves are 1 weaker for both sides.
    Mist,    ///< Every move is 10 % less likely to land.
    Wind,    ///< Pace moves are 1 weaker for both sides.
    Snow,    ///< The runner loses 1 stamina a turn.
    Count
};

/// Time of day, from the stage's clock.
enum class TimeOfDay : uint8_t {
    Dawn = 0,
    Day,
    Dusk,
    Night,
};

/// Minutes since midnight at which each part of the day starts.
constexpr uint16_t kDayStarts   = 8 * 60;
constexpr uint16_t kDuskStarts  = 18 * 60;
constexpr uint16_t kNightStarts = 20 * 60;

inline TimeOfDay timeOfDay(uint16_t minutes)
{
    const uint16_t m = static_cast<uint16_t>(minutes % (24 * 60));
    if (m >= kNightStarts || m < 5 * 60) {
        return TimeOfDay::Night;
    }
    if (m >= kDuskStarts) {
        return TimeOfDay::Dusk;
    }
    if (m >= kDayStarts) {
        return TimeOfDay::Day;
    }
    return TimeOfDay::Dawn;
}

} // namespace TrailQuest

#endif // TRAILQUEST_TYPES_HPP
