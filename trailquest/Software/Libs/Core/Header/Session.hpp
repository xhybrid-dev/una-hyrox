/**
 ******************************************************************************
 * @file    Session.hpp
 * @brief   The game as the service runs it: commands in, views out, and when
 *          to save.
 *
 * A thin layer over Run, so that the service is only wiring (messages, the
 * file system, the buzzer) and everything with a rule in it is host-tested.
 * A command that does not fit the moment (the wrong phase, an exit that is not
 * there, an item not in the bag) is refused: nothing changes, and the view
 * says so (ViewCause::Refused).
 *
 * The engine runs in the service only (Session.cpp).
 ******************************************************************************
 */

#ifndef TRAILQUEST_SESSION_HPP
#define TRAILQUEST_SESSION_HPP

#include <cstdint>

#include "Run.hpp"
#include "View.hpp"

namespace TrailQuest
{

class Session
{
public:
    /// How far this build lets you go: Q4 drop 1 opens stages 1-3; the rest follow.
    static constexpr uint8_t kPlayableStages = 5;

    /// Start up with a saved game (or none). @p stir is mixed into the saved
    /// chance (Run::resume), so coming back after quitting mid-duel meets a
    /// fresh spirit; 0 replays exactly.
    void begin(const RunState* saved, uint32_t stir);

    bool hasGame() const { return mHasGame; }

    // Commands. Each returns false when refused.
    bool newGame(uint32_t seed);
    bool travel(uint8_t exitIndex);
    bool obstacle(BeatResult beat);
    bool duelTurn(const PlayerTurn& turn);
    /// Answer an offer to join. With a full pack, @p slot names who leaves.
    bool join(bool accept, uint8_t slot = kNone);
    /// From a stage clear to the next stage (if this build has it), or from the
    /// summit to the next lap.
    bool advance();

    /// The world now, and why (the last command's result).
    WorldView world() const;
    /// True while there is a duel to show: one in progress, or the one the
    /// last command just ended.
    bool showsDuel() const { return mShowDuel; }
    DuelView duel() const;

    bool            wantsSave() const { return mRun.wantsSave(); }
    void            saved() { mRun.saved(); }
    const RunState& state() const { return mRun.state(); }
    RunPhase        phase() const { return mRun.phase(); }

private:
    bool refuse();

    Run            mRun;
    bool           mHasGame  = false;
    bool           mShowDuel = false;
    ViewCause      mCause    = ViewCause::None;
    ArriveResult   mArrive;
    ObstacleResult mObstacle;
    TurnLog        mLog;
    uint8_t        mFromNode = 0;
};

} // namespace TrailQuest

#endif // TRAILQUEST_SESSION_HPP
