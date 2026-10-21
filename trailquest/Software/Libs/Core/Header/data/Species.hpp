/**
 ******************************************************************************
 * @file    Species.hpp
 * @brief   The Trail Spirits: who you meet on the Ridge.
 *
 * Header-only data (the GUI shows names and lines). Original creatures (brief
 * section 9: nothing borrowed), named for what they look like (a colour or a
 * word, and the animal: Gate Q2); the runner's jokes are in their lines and
 * their moves (the fox still gives you a stitch).
 *
 * A spirit's numbers are set *relative to the runner at the same level*: its
 * stamina is a percentage of the runner's, and its pace, form and grit are
 * offsets from the runner's. So a spirit stays the same kind of fight as both
 * level up, and the balance simulator tunes one row at a time. Tuned in Q1;
 * docs/BALANCE.md has the results and the history.
 ******************************************************************************
 */

#ifndef TRAILQUEST_DATA_SPECIES_HPP
#define TRAILQUEST_DATA_SPECIES_HPP

#include <cstdint>

#include "Types.hpp"
#include "data/Moves.hpp"

namespace TrailQuest
{

/// SpeciesDef::flags.
constexpr uint8_t kSpeciesBoss  = 0x01;   ///< A stage's boss: never joins.
constexpr uint8_t kSpeciesNight = 0x02;   ///< Met only outside full daylight (dawn, dusk, night).

/// A boss's twist: on every twistPeriod-th turn it tells (the GUI warns
/// first) and the turn is special (docs/DESIGN.md, "Boss twists").
enum class Twist : uint8_t {
    None = 0,
    Charge,      ///< Its attack hits double; the runner's Settle braces, and it hits half.
    Guard,       ///< The runner's damage is halved this turn.
    Alternate,   ///< Charge on the first tell, Guard on the second, and so on.
};

struct SpeciesDef {
    const char* name;
    const char* kind;          ///< What it looks like, for the art.
    const char* pain;          ///< One line: what it does to a runner.
    uint8_t     stage;         ///< 1-5: where it lives.
    Affinity    affinity;      ///< Its place on the wheel.
    uint8_t     staminaPct;    ///< Stamina, per cent of the runner's at its level.
    int8_t      pace;          ///< Offsets from the runner's at its level.
    int8_t      form;
    int8_t      grit;
    MoveId      moves[2];
    uint8_t     firstMoveWeight;   ///< Per cent of turns it uses moves[0].
    uint8_t     flags;
    Twist       twist       = Twist::None;   ///< A boss's twist.
    uint8_t     twistPeriod = 0;             ///< It tells every this many turns.
};

namespace Spirit
{
enum : SpeciesId {
    // Stage 1: Valley Path.
    RedFox = 0, BrownBear, RubyBeetle, SandCrab, OldTortoise,
    // Stage 2: Switchback Climb.
    FireNewt, GnatSwarm, BarnOwl, PygmyShrew, GreatStag,
    // Stage 3: Scree Fields.
    RockGoat, MistWisp, WallLizard, Marmot, RamKing,
    // Stage 4: Ridge Line.
    WindSprite, Raven, SnowHare, GoldenEagle,
    // Stage 5: The Summit.
    GhostGoat, SnowFox, SummitWolf,
    Count
};
} // namespace Spirit

inline constexpr SpeciesDef kSpecies[Spirit::Count] = {
    // Stage 1: Valley Path. Light; teaches the wheel.
    { "Red Fox", "fox", "A side stitch: sharp pain under the ribs.",
      1, Affinity::Pace, 90, 1, 0, -1, { Move::SideStitch, Move::Pounce }, 50, 0 },
    { "Brown Bear", "bear", "The bonk. Fuel runs out and the legs go.",
      1, Affinity::Grit, 120, -2, 0, 1, { Move::HitTheWall, Move::Lumber }, 40, 0 },
    { "Ruby Beetle", "beetle", "A hot spot in your sock, rubbing raw.",
      1, Affinity::Form, 90, 0, 0, -1, { Move::RubRaw, Move::Nip }, 50, 0 },
    { "Sand Crab", "crab", "Grit in your shoe, chafing every step.",
      1, Affinity::Form, 100, -1, 0, 0, { Move::SidewaysScuttle, Move::Pinch }, 50, 0 },
    { "Old Tortoise", "giant tortoise", "Late fatigue. Slow, steady, heavy.",
      1, Affinity::Grit, 102, -3, 0, 1, { Move::MossSlam, Move::ShellSnap }, 60, kSpeciesBoss, Twist::Guard, 4 },

    // Stage 2: Switchback Climb. Stamina matters.
    { "Fire Newt", "red newt", "Burning quads from the climb. Walk the steeps.",
      2, Affinity::Grit, 85, 0, -1, 2, { Move::Burn, Move::TailFlick }, 50, 0 },
    { "Gnat Swarm", "gnat swarm", "Calf cramp. Salt and a shorter stride.",
      2, Affinity::Pace, 120, 2, -1, 0, { Move::Seize, Move::Swarm }, 50, 0 },
    { "Barn Owl", "owl", "Doubt in the dark. Slow your breathing.",
      2, Affinity::Grit, 90, -1, 0, 2, { Move::Wheeze, Move::Hoot }, 50, kSpeciesNight },
    { "Pygmy Shrew", "shrew", "A niggle: small aches that wreck your focus.",
      2, Affinity::Form, 125, 1, 1, 0, { Move::Nag, Move::Scurry }, 50, 0 },
    { "The Great Stag", "great stag", "The long climb, taken too fast.",
      2, Affinity::Pace, 108, 0, 0, 1, { Move::AntlerCharge, Move::LongClimb }, 55, kSpeciesBoss, Twist::Charge, 3 },

    // Stage 3: Scree Fields. Technical; footing matters.
    { "Rock Goat", "mountain goat", "Loose rock punishes a long stride.",
      3, Affinity::Pace, 100, 1, 0, -1, { Move::Leap, Move::Rockslide }, 55, 0 },
    { "Mist Wisp", "mist", "Self-doubt in the mist. Stay on pace.",
      3, Affinity::Grit, 120, 0, 0, 2, { Move::SecondGuess, Move::Fog }, 50, 0 },
    { "Wall Lizard", "lizard", "Wet rock. One bad step and you slip.",
      3, Affinity::Form, 103, 1, 1, -1, { Move::Skid, Move::Trip }, 50, 0 },
    { "Marmot", "marmot", "Rolling stones. Watch your footing.",
      3, Affinity::Form, 125, 0, 1, 0, { Move::AnkleSnap, Move::Burrow }, 50, 0 },
    { "The Ram King", "stone ram", "Loose scree. Short steps.",
      3, Affinity::Form, 97, -2, 2, 0, { Move::ShaleCrash, Move::Landslide }, 55, kSpeciesBoss, Twist::Guard, 3 },

    // Stage 4: Ridge Line. Exposed; the jokes thin out.
    { "Wind Sprite", "wind spirit", "A headwind that costs you speed for hours.",
      4, Affinity::Pace, 90, 1, -1, 0, { Move::Gust, Move::Headwind }, 50, 0 },
    { "Raven", "raven", "Doubt on the edge. Watch the path, not the drop.",
      4, Affinity::Grit, 105, 1, 0, 1, { Move::Spiral, Move::Peck }, 50, 0 },
    { "Snow Hare", "snow hare", "Cold numbs you. Keep moving and keep eating.",
      4, Affinity::Form, 135, 1, 1, 0, { Move::Chill, Move::Bound }, 50, kSpeciesNight },
    { "The Golden Eagle", "eagle", "The exposed edge. Stay steady.",
      4, Affinity::Pace, 83, 0, 0, 0, { Move::Stoop, Move::Updraught }, 55, kSpeciesBoss, Twist::Charge, 3 },

    // Stage 5: The Summit. Quiet and serious.
    { "Ghost Goat", "ghost goat", "Thin air. Every breath gives less.",
      5, Affinity::Grit, 100, 0, 0, 1, { Move::Gasp, Move::Fade }, 50, 0 },
    { "Snow Fox", "snow fox", "Cold hands. Keep them moving.",
      5, Affinity::Pace, 115, 1, 1, 0, { Move::FrostBite, Move::Flurry }, 50, 0 },
    { "The Summit Wolf", "wolf", "Kept pace since the valley.",
      5, Affinity::Grit, 84, 0, 0, 1, { Move::Howl, Move::LongChase }, 50, kSpeciesBoss, Twist::Alternate, 3 },
};

/// The spirit, or the Red Fox for an id out of range (no MMU: never read past the table).
inline const SpeciesDef& speciesDef(SpeciesId id)
{
    return kSpecies[id < Spirit::Count ? id : static_cast<SpeciesId>(Spirit::RedFox)];
}

inline bool isBoss(SpeciesId id) { return (speciesDef(id).flags & kSpeciesBoss) != 0; }

/// The move a spirit lends as a partner, which R1's Technique plays: one of
/// its own affinity (so a Form partner lends a Form move), the stronger if
/// both are, the first on a tie; if neither is, the stronger of the two.
inline MoveId signatureMove(SpeciesId id)
{
    const SpeciesDef& d  = speciesDef(id);
    const MoveDef&    m0 = moveDef(d.moves[0]);
    const MoveDef&    m1 = moveDef(d.moves[1]);
    const bool own0 = m0.affinity == d.affinity;
    const bool own1 = m1.affinity == d.affinity;
    if (own0 != own1) {
        return own0 ? d.moves[0] : d.moves[1];
    }
    return m1.power > m0.power ? d.moves[1] : d.moves[0];
}

} // namespace TrailQuest

#endif // TRAILQUEST_DATA_SPECIES_HPP
