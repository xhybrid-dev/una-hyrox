/**
 ******************************************************************************
 * @file    Duel.hpp
 * @brief   One duel between the runner and a Trail Spirit, turn by turn.
 *
 * Pure and deterministic: a duel's whole course follows from its setup, the
 * player's turns and the Rng, so the same inputs replay the same duel (the
 * host tests and the balance simulator rely on it). No heap; the duel is a
 * small value that can be copied.
 *
 * A turn: the player picks one of four actions (Surge, Settle, Technique,
 * Fuel), with the timing beat's result; the spirit picks its own. The faster
 * side (pace, after effects and weather) acts first. Each hit:
 *
 *   damage = power x (10 + attacker's stat) / (10 + defender's stat)
 *            x the wheel (3/2 if the move's affinity beats the spirit's, 3/4
 *              if it loses; the runner has no affinity, so a spirit's moves
 *              never get it)
 *            x the beat (runner only: Miss 1/2, Good 1, Perfect 3/2)
 *            + 2 power if the move suits the terrain, - 1 for Form in rain,
 *              - 1 for Pace in wind
 *            + attacker's grit / 2 when the defender is under a quarter
 *            x (9 + attacker's level) / 10, as stamina grows (Progress.hpp)
 *            and at least 1.
 *   Recovery (Settle, items) and drain scale with level the same way.
 *   The stat is the one the move's affinity names (Pace, Form or Grit).
 *
 * A move lands with 90 % + 2 % per point of pace over the defender, held to
 * 60-100 %, then 10 % less in mist, and 10 % less for the runner in the dark.
 * Effects (Drain, Slow, Rattle) last three turns, one per fighter at a time.
 * A boss rests once, when under 30 %. A duel ends when a side is spent; at
 * kMaxTurns the larger share of stamina wins (the spirit on a tie), so no duel
 * can go on for ever.
 *
 * The engine (Duel.cpp) runs in the service only; the GUI sees the results.
 ******************************************************************************
 */

#ifndef TRAILQUEST_DUEL_HPP
#define TRAILQUEST_DUEL_HPP

#include <cstdint>

#include "Beat.hpp"
#include "Rng.hpp"
#include "Types.hpp"
#include "data/Items.hpp"
#include "data/Moves.hpp"
#include "data/Species.hpp"

namespace TrailQuest
{

constexpr uint8_t kMaxTurns = 12;

/// Hit chance, per cent.
constexpr int32_t kHitBase       = 90;
constexpr int32_t kHitPerPace    = 2;
constexpr int32_t kHitMin        = 60;
constexpr int32_t kHitMax        = 100;
constexpr int32_t kHitMistOrDark = 10;

/// A boss rests once below this share of its stamina, recovering kBossRestPct.
constexpr uint32_t kBossRestBelowPct = 30;
constexpr uint32_t kBossRestPct      = 25;

enum class Side : uint8_t { Runner = 0, Spirit };

enum class Action : uint8_t {
    Surge = 0,   ///< L1
    Settle,      ///< L2
    Technique,   ///< R1: the lead partner's move, or Trail craft
    Fuel,        ///< R2: use an item
    Count
};

enum class DuelOutcome : uint8_t { Ongoing = 0, Won, Lost };

struct Fighter {
    Stats    max;               ///< Stamina at full, and the base stats.
    uint16_t stamina     = 0;   ///< Now.
    Affinity affinity    = Affinity::Count;   ///< Count: none (the runner).
    Effect   effect      = Effect::None;
    uint8_t  effectTurns = 0;
    uint8_t  formBoost   = 0;   ///< From poles, for the rest of the duel.
    uint8_t  level       = 1;
};

/// What the Run tells the duel about the runner and the place.
struct DuelSetup {
    uint8_t   runnerLevel   = 1;
    Stats     runnerMax;              ///< With partners' bonuses.
    uint16_t  runnerStamina = 0;      ///< Carried in from the trail.
    MoveId    technique     = Move::TrailCraft;
    SpeciesId species       = Spirit::RedFox;
    uint8_t   speciesLevel  = 1;
    Terrain   terrain       = Terrain::Path;
    Weather   weather       = Weather::Clear;
    bool      dark          = false;  ///< Night without a headtorch.
};

/// The player's choice for a turn.
struct PlayerTurn {
    Action     action = Action::Surge;
    ItemId     item   = kNone;              ///< For Fuel.
    BeatResult beat   = BeatResult::Good;   ///< Menu-only play counts as Good.
};

enum class EventKind : uint8_t {
    Hit = 0,     ///< amount = damage
    Missed,
    Inflicted,   ///< effect put on the other side
    Wore,        ///< effect ended
    Ticked,      ///< amount = stamina lost to drain or snow
    Healed,      ///< amount = stamina back (Settle, an item, a boss's rest)
    UsedItem,
    NoItem,      ///< Fuel with nothing to use: the turn is lost
    Rested,      ///< a boss's rest
};

struct DuelEvent {
    EventKind kind   = EventKind::Hit;
    Side      actor  = Side::Runner;   ///< Who did it (or who it happened to).
    MoveId    move   = kNone;
    uint16_t  amount = 0;
    Effect    effect = Effect::None;
    ItemId    item   = kNone;
};

constexpr uint8_t kMaxEvents = 12;

/// What happened in one turn, in order, for the GUI to play back.
struct TurnLog {
    DuelEvent events[kMaxEvents];
    uint8_t   count       = 0;
    bool      runnerFirst = true;

    void add(const DuelEvent& e)
    {
        if (count < kMaxEvents) {
            events[count++] = e;
        }
    }
};

class Duel
{
public:
    void start(const DuelSetup& setup);

    /// Play one turn. @p bag is the runner's item counts (Item::Count of them);
    /// Fuel takes from it. Does nothing once the duel is over.
    TurnLog play(const PlayerTurn& turn, Rng& rng, uint8_t* bag);

    DuelOutcome      outcome() const { return mOutcome; }
    uint8_t          turn() const { return mTurn; }
    const Fighter&   runner() const { return mRunner; }
    const Fighter&   spirit() const { return mSpirit; }
    const DuelSetup& setup() const { return mSetup; }

    /// The move an action uses (kNone for Fuel).
    MoveId moveFor(Action a) const;
    /// The affinity a runner's move is played in (Trail craft takes the terrain's).
    Affinity affinityFor(MoveId m) const;

    /// Damage a runner's action would do now, if it lands, with @p beat: no
    /// chance involved. For hints and the balance simulator's player.
    uint16_t estimate(Action a, BeatResult beat = BeatResult::Good) const;

    /// The per cent chance that @p by's move lands now.
    uint8_t chanceToLand(Side by) const { return hitChance(by); }

private:
    struct Eff {
        uint8_t pace, form, grit;
    };
    Eff effective(const Fighter& f) const;
    uint16_t damage(Side by, MoveId move, Affinity aff, BeatResult beat) const;
    uint8_t  hitChance(Side by) const;
    void act(Side by, MoveId move, BeatResult beat, Rng& rng, TurnLog& log);
    void runnerAct(const PlayerTurn& t, Rng& rng, uint8_t* bag, TurnLog& log);
    void spiritAct(Rng& rng, TurnLog& log);
    void endOfTurn(TurnLog& log);
    bool checkOver();

    Fighter&       fighter(Side s) { return s == Side::Runner ? mRunner : mSpirit; }
    const Fighter& fighter(Side s) const { return s == Side::Runner ? mRunner : mSpirit; }

    DuelSetup   mSetup;
    Fighter     mRunner;
    Fighter     mSpirit;
    uint8_t     mTurn      = 0;
    bool        mRested    = false;
    DuelOutcome mOutcome   = DuelOutcome::Ongoing;
};

} // namespace TrailQuest

#endif // TRAILQUEST_DUEL_HPP
