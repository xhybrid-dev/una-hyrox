/**
 ******************************************************************************
 * @file    Run.hpp
 * @brief   The game: the runner's way across the Ridge, from a new game or a
 *          save to the summit and the next lap.
 *
 * A small state machine over RunState. Pure and deterministic: all chance
 * comes from the Rng whose state RunState keeps, so a save plus the player's
 * choices replay exactly. The engine (Run.cpp) runs in the service only.
 *
 *   AtNode     choose an exit (travel)
 *   Obstacle   attempt it, with the beat's result
 *   InDuel     play turns until it ends
 *   JoinOffer  the beaten spirit offers to run with you: accept or decline
 *   StageClear the boss is beaten: on to the next stage
 *   Finished   the summit: start the next lap, harder
 *
 * Each stretch of trail gives back kTravelRecoverPct of the runner's stamina,
 * and the runner faces a boss with what is left (no breather at the foot of it).
 *
 * Rules (Jon's, 17 October 2026): a lost duel sends the runner back to the
 * last hut of the stage (or its start), costing kLossMinutes and one item,
 * with stamina restored and partners kept. A beaten spirit offers to join; a
 * full pack means choosing who leaves. Bosses never join.
 *
 * In a duel, R1's Technique is played by the partner best placed against that
 * spirit (the most damage now), or Trail craft if none beats it: the pack
 * chips in by itself, so there is no lead to choose on a four-button watch.
 ******************************************************************************
 */

#ifndef TRAILQUEST_RUN_HPP
#define TRAILQUEST_RUN_HPP

#include <cstdint>

#include "Beat.hpp"
#include "Duel.hpp"
#include "Rng.hpp"
#include "RunState.hpp"
#include "data/Stages.hpp"

namespace TrailQuest
{

/// What a lost duel costs in trail time.
constexpr uint16_t kLossMinutes = 30;

/// Stamina recovered on each stretch of trail, per cent of the runner's full: a
/// breather and no more, so a fight's fatigue carries on to the next and the
/// way back is a Fuel (a Gel, Water, a Salt tab) or the hut (Jon, 21 October).
constexpr uint8_t kTravelRecoverPct = 8;

/// Moving on to the next stage gives back this per cent of what is missing.
constexpr uint8_t kStageRestPct = 50;

/// Chance of an item after a won duel, per cent.
constexpr uint8_t kDropChance = 35;

/// What a new game starts with.
constexpr uint8_t kStartGels  = 3;
constexpr uint8_t kStartWater = 1;

enum class RunPhase : uint8_t {
    AtNode = 0,
    Obstacle,
    InDuel,
    JoinOffer,
    StageClear,
    Finished,
};

/// What arriving somewhere led to.
enum class Arrival : uint8_t {
    Nothing = 0,   ///< A quiet stretch.
    Found,         ///< item (or bagFull)
    Duel,          ///< a spirit: phase InDuel
    Obstacle,      ///< phase Obstacle
    Hut,           ///< rested; where a loss returns you
    Boss,          ///< the stage's boss: phase InDuel
    Refused,       ///< not a valid move now (wrong phase or exit)
};

struct ArriveResult {
    Arrival what    = Arrival::Nothing;
    ItemId  item    = kNone;
    bool    bagFull = false;   ///< Something was there, but the bag had no room.
};

struct ObstacleResult {
    bool     valid   = false;  ///< False: not at an obstacle.
    bool     cleared = false;
    uint8_t  chance  = 0;      ///< The odds it was attempted at, per cent.
    uint16_t staminaLost = 0;
    uint16_t minutesLost = 0;
};

/// What the end of a duel gave or cost.
struct DuelResult {
    DuelOutcome outcome     = DuelOutcome::Ongoing;
    uint16_t    xp          = 0;
    uint8_t     levelsGained = 0;
    ItemId      item        = kNone;   ///< Dropped by the spirit.
    bool        joinOffered = false;
    bool        boss        = false;
    ItemId      lostItem    = kNone;   ///< Lost by losing.
};

class Run
{
public:
    /// A new game from @p seed.
    void newGame(uint32_t seed);

    /// Carry on from a saved state (already checked by the save codec). A
    /// non-zero @p stir mixes the saved chance with it, so coming back after
    /// quitting mid-duel meets a fresh spirit rather than the same one; 0
    /// replays exactly (the tests).
    void resume(const RunState& s, uint32_t stir = 0);

    const RunState& state() const { return mState; }
    RunPhase        phase() const { return mPhase; }

    const StageDef& stage() const { return stageDef(mState.stage); }
    const NodeDef&  node() const { return nodeDef(mState.stage, mState.node); }
    TimeOfDay       timeOfDay() const { return TrailQuest::timeOfDay(mState.minutes); }
    bool            dark() const;

    // -- AtNode --------------------------------------------------------------
    ArriveResult travel(uint8_t exitIndex);

    // -- Obstacle ------------------------------------------------------------
    uint8_t        obstacleOdds(BeatResult beat) const;
    ObstacleResult attemptObstacle(BeatResult beat);

    // -- InDuel --------------------------------------------------------------
    const Duel& duel() const { return mDuel; }
    TurnLog     duelTurn(const PlayerTurn& turn);
    /// The end of the last duel (valid once it is over).
    const DuelResult& lastDuel() const { return mLastDuel; }

    // -- JoinOffer -----------------------------------------------------------
    SpeciesId offered() const { return mState.offer; }
    /// Take the offer. With a full pack, @p replaceSlot names who leaves;
    /// otherwise pass kNone. False if that is not possible.
    bool acceptJoin(uint8_t replaceSlot = kNone);
    void declineJoin();

    // -- StageClear / Finished -----------------------------------------------
    void nextStage();
    void startNextLap();

    /// The service saves when this is set, then calls saved(). It is set after
    /// every step that ends in a resting phase (AtNode, JoinOffer, StageClear,
    /// Finished), never in the middle of an obstacle or a duel.
    bool wantsSave() const { return mWantsSave; }
    void saved() { mWantsSave = false; }

private:
    void startStage(uint8_t stage, bool fullRest = true);
    void rest(RunPhase phase);
    void startDuel(SpeciesId sp, uint8_t level);
    MoveId bestTechnique(const DuelSetup& base) const;
    void finishDuel();
    void gainXp(uint16_t xp, uint8_t& levelsGained);
    bool addItem(ItemId item);
    ItemId loseAnItem();

    Rng& rng() { return mRng; }
    void syncRng() { mState.rng = mRng.state(); }

    RunState   mState;
    Rng        mRng;
    RunPhase   mPhase     = RunPhase::AtNode;
    Duel       mDuel;
    DuelResult mLastDuel;
    bool       mWantsSave = false;
};

} // namespace TrailQuest

#endif // TRAILQUEST_RUN_HPP
