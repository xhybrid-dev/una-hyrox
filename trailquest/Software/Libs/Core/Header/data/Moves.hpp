/**
 ******************************************************************************
 * @file    Moves.hpp
 * @brief   Every move in the game: the runner's three and the Trail Spirits'.
 *
 * Header-only data (the GUI names the moves). Numbers are the first draft;
 * the balance simulator (Tests/Host/tools/balance_sim) tunes them and
 * docs/BALANCE.md records the result.
 *
 * power    base damage before stats, the wheel, the beat and the terrain;
 * affinity which stat it scales on, and its place on the wheel;
 * selfCost stamina the user spends on it;
 * effect   a status it may cause, with effectChance per cent.
 ******************************************************************************
 */

#ifndef TRAILQUEST_DATA_MOVES_HPP
#define TRAILQUEST_DATA_MOVES_HPP

#include <cstdint>

#include "Types.hpp"

namespace TrailQuest
{

struct MoveDef {
    const char* name;
    uint8_t     power;
    Affinity    affinity;
    uint8_t     selfCost;
    Effect      effect;
    uint8_t     effectChance;
};

namespace Move
{
enum : MoveId {
    // The runner's own.
    Surge = 0,      ///< L1: a hard push.
    Settle,         ///< L2: steady rhythm, grit it out (Grit); also recovers kSettleRecover.
    TrailCraft,     ///< R1 with no partner: its affinity is the terrain's.
    // Stage 1.
    SideStitch, Pounce, HitTheWall, Lumber, RubRaw, Nip, SidewaysScuttle, Pinch,
    MossSlam, ShellSnap,
    // Stage 2.
    Burn, TailFlick, Seize, Swarm, Wheeze, Hoot, Nag, Scurry, AntlerCharge, LongClimb,
    // Stage 3.
    Leap, Rockslide, SecondGuess, Fog, Skid, Trip, AnkleSnap, Burrow, ShaleCrash, Landslide,
    // Stage 4.
    Gust, Headwind, Spiral, Peck, Chill, Bound, Stoop, Updraught,
    // Stage 5.
    Gasp, Fade, FrostBite, Flurry, Howl, LongChase,
    Count
};
} // namespace Move

/// Stamina Settle recovers.
constexpr uint8_t kSettleRecover = 6;

inline constexpr MoveDef kMoves[Move::Count] = {
    { "Surge",            14, Affinity::Pace, 4, Effect::None,   0  },
    { "Settle",            5, Affinity::Grit, 0, Effect::None,   0  },
    { "Trail craft",       9, Affinity::Form, 0, Effect::None,   0  },
    // Stage 1.
    { "Side stitch",       7, Affinity::Pace, 0, Effect::Drain,  40 },
    { "Pounce",            9, Affinity::Pace, 0, Effect::None,   0  },
    { "Hit the wall",     10, Affinity::Grit, 0, Effect::Slow,   30 },
    { "Lumber",            7, Affinity::Grit, 0, Effect::None,   0  },
    { "Rub raw",           5, Affinity::Form, 0, Effect::Drain,  50 },
    { "Nip",               7, Affinity::Form, 0, Effect::None,   0  },
    { "Sideways scuttle",  7, Affinity::Form, 0, Effect::Slow,   30 },
    { "Pinch",             8, Affinity::Form, 0, Effect::None,   0  },
    { "Moss slam",        11, Affinity::Grit, 0, Effect::None,   0  },
    { "Shell snap",        9, Affinity::Form, 0, Effect::None,   0  },
    // Stage 2.
    { "Burn",              9, Affinity::Grit, 0, Effect::Slow,   40 },
    { "Tail flick",        7, Affinity::Pace, 0, Effect::None,   0  },
    { "Seize",             7, Affinity::Pace, 0, Effect::Slow,   60 },
    { "Swarm",             6, Affinity::Pace, 0, Effect::Drain,  30 },
    { "Wheeze",            7, Affinity::Grit, 0, Effect::Rattle, 30 },
    { "Hoot",              9, Affinity::Grit, 0, Effect::None,   0  },
    { "Nag",               5, Affinity::Form, 0, Effect::Rattle, 50 },
    { "Scurry",            8, Affinity::Pace, 0, Effect::None,   0  },
    { "Antler charge",    11, Affinity::Pace, 0, Effect::None,   0  },
    { "Long climb",        8, Affinity::Grit, 0, Effect::Drain,  40 },
    // Stage 3.
    { "Leap",              8, Affinity::Pace, 0, Effect::None,   0  },
    { "Rockslide",         8, Affinity::Form, 0, Effect::None,   0  },
    { "Second-guess",      8, Affinity::Grit, 0, Effect::Rattle, 60 },
    { "Fog",               7, Affinity::Form, 0, Effect::Slow,   40 },
    { "Skid",              8, Affinity::Form, 0, Effect::None,   0  },
    { "Trip",              7, Affinity::Pace, 0, Effect::Slow,   30 },
    { "Ankle snap",       9, Affinity::Form, 0, Effect::Slow,   30 },
    { "Burrow",            7, Affinity::Grit, 0, Effect::None,   0  },
    { "Shale crash",      11, Affinity::Form, 0, Effect::None,   0  },
    { "Landslide",        9, Affinity::Form, 0, Effect::Slow,   50 },
    // Stage 4.
    { "Gust",              8, Affinity::Pace, 0, Effect::Slow,   40 },
    { "Headwind",         9, Affinity::Grit, 0, Effect::None,   0  },
    { "Spiral",            8, Affinity::Grit, 0, Effect::Rattle, 50 },
    { "Peck",              8, Affinity::Pace, 0, Effect::None,   0  },
    { "Chill",             7, Affinity::Form, 0, Effect::Drain,  50 },
    { "Bound",             8, Affinity::Pace, 0, Effect::None,   0  },
    { "Stoop",            11, Affinity::Pace, 0, Effect::None,   0  },
    { "Updraught",        9, Affinity::Grit, 0, Effect::Rattle, 50 },
    // Stage 5.
    { "Gasp",              8, Affinity::Grit, 0, Effect::Drain,  40 },
    { "Fade",              8, Affinity::Form, 0, Effect::Slow,   30 },
    { "Frost bite",       9, Affinity::Pace, 0, Effect::Slow,   40 },
    { "Flurry",            8, Affinity::Form, 0, Effect::None,   0  },
    { "Howl",              8, Affinity::Grit, 0, Effect::Rattle, 60 },
    { "Long chase",       10, Affinity::Pace, 0, Effect::Drain,  40 },
};

/// The move, or Surge for an id out of range (no MMU: never read past the table).
inline const MoveDef& moveDef(MoveId id)
{
    return kMoves[id < Move::Count ? id : static_cast<MoveId>(Move::Surge)];
}

} // namespace TrailQuest

#endif // TRAILQUEST_DATA_MOVES_HPP
