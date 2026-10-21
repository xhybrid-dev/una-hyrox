/**
 ******************************************************************************
 * @file    Session.cpp
 * @brief   The game as the service runs it (see Session.hpp).
 ******************************************************************************
 */

#include "Session.hpp"

namespace TrailQuest
{

namespace
{

FighterView fighterView(const Fighter& f)
{
    FighterView v;
    v.stamina     = f.stamina;
    v.max         = f.max.stamina;
    v.effect      = static_cast<uint8_t>(f.effect);
    v.effectTurns = f.effectTurns;
    v.level       = f.level;
    v.formBoost   = f.formBoost;
    return v;
}

} // namespace

void Session::begin(const RunState* saved, uint32_t stir)
{
    mShowDuel = false;
    mArrive   = ArriveResult{};
    mObstacle = ObstacleResult{};
    mLog      = TurnLog{};
    if (saved != nullptr) {
        mRun.resume(*saved, stir);
        mHasGame = true;
        mCause   = ViewCause::Loaded;
    } else {
        mHasGame = false;
        mCause   = ViewCause::None;
    }
    mFromNode = mRun.state().node;
}

bool Session::refuse()
{
    mCause = ViewCause::Refused;
    return false;
}

bool Session::newGame(uint32_t seed)
{
    mRun.newGame(seed);
    mHasGame  = true;
    mShowDuel = false;
    mArrive   = ArriveResult{};
    mObstacle = ObstacleResult{};
    mLog      = TurnLog{};
    mFromNode = 0;
    mCause    = ViewCause::NewGame;
    return true;
}

bool Session::travel(uint8_t exitIndex)
{
    if (!mHasGame || mRun.phase() != RunPhase::AtNode) {
        return refuse();
    }
    const uint8_t      from = mRun.state().node;
    const ArriveResult r    = mRun.travel(exitIndex);
    if (r.what == Arrival::Refused) {
        return refuse();
    }
    mFromNode = from;
    mArrive   = r;
    mObstacle = ObstacleResult{};
    mLog      = TurnLog{};
    mShowDuel = mRun.phase() == RunPhase::InDuel;
    mCause    = ViewCause::Arrived;
    return true;
}

bool Session::obstacle(BeatResult beat)
{
    if (!mHasGame || mRun.phase() != RunPhase::Obstacle || static_cast<uint8_t>(beat) > static_cast<uint8_t>(BeatResult::Perfect)) {
        return refuse();
    }
    mObstacle = mRun.attemptObstacle(beat);
    mShowDuel = false;
    mCause    = ViewCause::ObstacleDone;
    return true;
}

bool Session::duelTurn(const PlayerTurn& turn)
{
    if (!mHasGame || mRun.phase() != RunPhase::InDuel || static_cast<uint8_t>(turn.action) >= static_cast<uint8_t>(Action::Count)
        || static_cast<uint8_t>(turn.beat) > static_cast<uint8_t>(BeatResult::Perfect)) {
        return refuse();
    }
    if (turn.action == Action::Fuel) {
        // Only something in the bag, and never the headtorch: a bad request is
        // refused rather than costing the turn.
        if (turn.item >= Item::Count || itemDef(turn.item).key || mRun.state().bag[turn.item] == 0) {
            return refuse();
        }
    }
    mLog      = mRun.duelTurn(turn);
    mShowDuel = true;
    mCause    = mRun.phase() == RunPhase::InDuel ? ViewCause::DuelTurn : ViewCause::DuelOver;
    return true;
}

bool Session::join(bool accept, uint8_t slot)
{
    if (!mHasGame || mRun.phase() != RunPhase::JoinOffer) {
        return refuse();
    }
    mShowDuel = false;
    if (!accept) {
        mRun.declineJoin();
        mCause = ViewCause::Declined;
        return true;
    }
    if (!mRun.acceptJoin(slot)) {
        return refuse();
    }
    mCause = ViewCause::Joined;
    return true;
}

bool Session::advance()
{
    if (!mHasGame) {
        return refuse();
    }
    mShowDuel = false;
    if (mRun.phase() == RunPhase::StageClear) {   // at the summit this ends the lap: Finished
        mRun.nextStage();
    } else if (mRun.phase() == RunPhase::Finished) {
        mRun.startNextLap();
    } else {
        return refuse();
    }
    mFromNode = 0;
    mArrive   = ArriveResult{};
    mCause    = ViewCause::StageStarted;
    return true;
}

WorldView Session::world() const
{
    WorldView v;
    v.state          = mRun.state();
    v.phase          = mRun.phase();
    v.cause          = mCause;
    v.hasGame        = mHasGame ? 1u : 0u;
    v.playableStages = kPlayableStages;
    v.timeOfDay      = static_cast<uint8_t>(mRun.timeOfDay());
    v.dark           = mRun.dark() ? 1u : 0u;
    if (mRun.phase() == RunPhase::Obstacle) {
        v.odds[0] = mRun.obstacleOdds(BeatResult::Miss);
        v.odds[1] = mRun.obstacleOdds(BeatResult::Good);
        v.odds[2] = mRun.obstacleOdds(BeatResult::Perfect);
    }
    v.arrive   = mArrive;
    v.obstacle = mObstacle;
    v.duel     = mRun.lastDuel();
    v.fromNode = mFromNode;
    return v;
}

DuelView Session::duel() const
{
    DuelView    v;
    const Duel& d  = mRun.duel();
    v.species      = d.setup().species;
    v.technique    = d.setup().technique;
    v.turn         = d.turn();
    v.outcome      = d.outcome();
    v.terrain      = static_cast<uint8_t>(d.setup().terrain);
    v.weather      = static_cast<uint8_t>(d.setup().weather);
    v.dark         = d.setup().dark ? 1u : 0u;
    v.chanceRunner = d.chanceToLand(Side::Runner);
    v.chanceSpirit = d.chanceToLand(Side::Spirit);
    v.tell         = static_cast<uint8_t>(d.tell());
    v.estimate[0]  = d.estimate(Action::Surge);
    v.estimate[1]  = d.estimate(Action::Settle);
    v.estimate[2]  = d.estimate(Action::Technique);
    v.skillCount   = d.setup().skillCount < kMaxSkills ? d.setup().skillCount : kMaxSkills;
    for (uint8_t i = 0; i < v.skillCount; ++i) {
        v.skill[i]    = d.setup().skills[i];
        v.skillDmg[i] = d.estimateSkill(i);
        if (v.skill[i] == d.setup().technique && v.skill[v.skillBest] != d.setup().technique) {
            v.skillBest = i;   // the first with the default's move
        }
    }
    v.runner       = fighterView(d.runner());
    v.spirit       = fighterView(d.spirit());
    v.log          = mLog;
    return v;
}

} // namespace TrailQuest
