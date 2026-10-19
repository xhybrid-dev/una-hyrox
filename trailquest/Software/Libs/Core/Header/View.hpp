/**
 ******************************************************************************
 * @file    View.hpp
 * @brief   What the screens are shown: snapshots of the game the service
 *          sends after every command.
 *
 * Header-only plain structs (the GUI reads them; the GUI links none of the
 * engine). Each is complete on its own, never a change against the last, so a
 * message lost or arriving late can only make a screen briefly stale.
 *
 * Sizes matter: each travels inside one kernel message of at most 256 bytes,
 * 32 of them the message's header (Commands.hpp asserts both fit).
 ******************************************************************************
 */

#ifndef TRAILQUEST_VIEW_HPP
#define TRAILQUEST_VIEW_HPP

#include <cstdint>

#include "Duel.hpp"
#include "Run.hpp"
#include "RunState.hpp"

namespace TrailQuest
{

/// Why a view was sent: what the last command did.
enum class ViewCause : uint8_t {
    None = 0,       ///< No game yet: the title offers a new one.
    Loaded,         ///< A saved game, picked up where it was left.
    NewGame,
    Arrived,        ///< A step along the trail: see WorldView::arrive.
    ObstacleDone,   ///< See WorldView::obstacle.
    DuelTurn,       ///< A turn played, the duel goes on (DuelView::log).
    DuelOver,       ///< The last turn (DuelView::log) and how it ended (WorldView::duel).
    Joined,         ///< The offered spirit runs with you now.
    Declined,
    StageStarted,
    Refused,        ///< The command did not fit the moment: nothing changed.
};

struct WorldView {
    RunState       state;
    RunPhase       phase          = RunPhase::AtNode;
    ViewCause      cause          = ViewCause::None;
    uint8_t        hasGame        = 0;   ///< 0: no game, only "new game".
    uint8_t        playableStages = 1;   ///< Stages this build lets you reach.
    uint8_t        timeOfDay      = 0;   ///< TimeOfDay.
    uint8_t        dark           = 0;   ///< Night without a headtorch.
    uint8_t        odds[3]        = {};  ///< At an obstacle: per cent for a Miss, Good, Perfect beat.
    ArriveResult   arrive;
    ObstacleResult obstacle;
    DuelResult     duel;
    uint8_t        fromNode       = 0;   ///< Where the last step began (the map walks the line).
};

struct FighterView {
    uint16_t stamina     = 0;
    uint16_t max         = 0;
    uint8_t  effect      = 0;   ///< Effect.
    uint8_t  effectTurns = 0;
    uint8_t  level       = 1;
    uint8_t  formBoost   = 0;
};

struct DuelView {
    SpeciesId   species      = 0;
    MoveId      technique    = 0;   ///< What R1 plays.
    uint8_t     turn         = 0;
    DuelOutcome outcome      = DuelOutcome::Ongoing;
    uint8_t     terrain      = 0;   ///< Terrain.
    uint8_t     weather      = 0;   ///< Weather.
    uint8_t     dark         = 0;
    uint8_t     chanceRunner = 0;   ///< Per cent the runner's move lands now.
    uint8_t     chanceSpirit = 0;
    uint16_t    estimate[3]  = {};  ///< Damage now with a Good beat: Surge, Settle, Technique.
    FighterView runner;
    FighterView spirit;
    TurnLog     log;                ///< The last turn played (empty at the start).
};

} // namespace TrailQuest

#endif // TRAILQUEST_VIEW_HPP
