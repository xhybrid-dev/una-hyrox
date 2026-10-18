/**
 ******************************************************************************
 * @file    Run.cpp
 * @brief   The game's flow (see Run.hpp).
 ******************************************************************************
 */

#include "Run.hpp"

#include "Encounter.hpp"
#include "Progress.hpp"

namespace TrailQuest
{

namespace
{
constexpr uint16_t kDayMinutes = 24 * 60;

uint16_t addMinutes(uint16_t now, uint32_t add)
{
    const uint32_t v = static_cast<uint32_t>(now) + add;
    return v > 0xFFFFu ? 0xFFFFu : static_cast<uint16_t>(v);
}

uint8_t stageBit(uint8_t stage) { return static_cast<uint8_t>(1u << (stage < 8 ? stage : 7)); }

uint32_t speciesBit(SpeciesId sp) { return sp < 32 ? (1u << sp) : 0u; }
} // namespace

void Run::newGame(uint32_t seed)
{
    mState = RunState{};
    mRng.setState(mixSeed(seed));
    mState.level            = 1;
    mState.bag[Item::Gel]   = kStartGels;
    mState.bag[Item::Water] = kStartWater;
    mState.minutes          = kRunStartMinutes;
    mLastDuel = DuelResult{};
    startStage(0);
}

void Run::resume(const RunState& s, uint32_t stir)
{
    mState = s;
    mRng.setState(stir != 0 ? mixSeed(s.rng ^ stir) : s.rng);
    syncRng();
    mLastDuel  = DuelResult{};
    mWantsSave = false;
    mPhase     = RunPhase::AtNode;
    if (mState.offer != kNone) {
        mPhase = RunPhase::JoinOffer;
    } else if (node().kind == NodeKind::Boss && (mState.bossesBeaten & stageBit(mState.stage)) != 0) {
        mPhase = mState.stage + 1u >= kStageCount ? RunPhase::Finished : RunPhase::StageClear;
    }
}

void Run::rest(RunPhase phase)
{
    mPhase     = phase;
    mWantsSave = true;
}

void Run::startStage(uint8_t st)
{
    mState.stage   = st < kStageCount ? st : 0u;
    mState.node    = 0;
    mState.hutNode = 0;
    mState.weather = static_cast<uint8_t>(stage().weather[mRng.below(2)]);
    mState.stamina = runnerMaxWithPartners(mState).stamina;
    rest(RunPhase::AtNode);
    syncRng();
}

bool Run::dark() const
{
    return timeOfDay() == TimeOfDay::Night && mState.bag[Item::Headtorch] == 0;
}

bool Run::addItem(ItemId item)
{
    if (item >= Item::Count) {
        return false;
    }
    if (itemDef(item).key) {
        mState.bag[item] = 1;
        return true;
    }
    if (bagCount(mState) >= kBagCapacity) {
        return false;
    }
    ++mState.bag[item];
    return true;
}

ItemId Run::loseAnItem()
{
    ItemId  worst = kNone;
    uint8_t most  = 0;
    for (ItemId i = 0; i < Item::Count; ++i) {
        if (!itemDef(i).key && mState.bag[i] > most) {
            most  = mState.bag[i];
            worst = i;
        }
    }
    if (worst != kNone) {
        --mState.bag[worst];
    }
    return worst;
}

void Run::gainXp(uint16_t xp, uint8_t& levelsGained)
{
    if (mState.level >= kMaxLevel) {
        mState.xp = 0;
        return;
    }
    uint32_t total = static_cast<uint32_t>(mState.xp) + xp;
    while (mState.level < kMaxLevel && total >= xpToNext(mState.level)) {
        total -= xpToNext(mState.level);
        ++mState.level;
        ++levelsGained;
        // A new level's extra stamina comes with it.
        const uint16_t max = runnerMaxWithPartners(mState).stamina;
        const uint32_t s   = static_cast<uint32_t>(mState.stamina) + kRunnerStaminaGrow;
        mState.stamina = s > max ? max : static_cast<uint16_t>(s);
    }
    mState.xp = mState.level >= kMaxLevel ? 0u : static_cast<uint16_t>(total > 0xFFFFu ? 0xFFFFu : total);
}

ArriveResult Run::travel(uint8_t exitIndex)
{
    ArriveResult r;
    const NodeDef& here = node();
    if (mPhase != RunPhase::AtNode || exitIndex >= here.exitCount || exitIndex >= kMaxExits
        || here.exits[exitIndex].to >= stage().nodeCount) {
        r.what = Arrival::Refused;
        return r;
    }
    const ExitDef& e = here.exits[exitIndex];
    mState.minutes = addMinutes(mState.minutes, e.minutes);
    mState.node    = e.to;
    {
        const uint16_t max = runnerMaxWithPartners(mState).stamina;
        const uint32_t s   = static_cast<uint32_t>(mState.stamina) + static_cast<uint32_t>(max) * kTravelRecoverPct / 100u;
        mState.stamina = s > max ? max : static_cast<uint16_t>(s);
    }

    const NodeDef& at = node();
    switch (at.kind) {
        case NodeKind::Trail:
            if (mRng.chance(at.param)) {
                r.what    = Arrival::Found;
                r.item    = pickFind(mRng);
                r.bagFull = !addItem(r.item);
            }
            break;

        case NodeKind::Encounter: {
            const SpeciesId sp    = pickSpecies(stage(), timeOfDay(), mRng);
            const uint8_t   level = pickLevel(stage(), mState.lap, mRng);
            startDuel(sp, level);
            r.what = Arrival::Duel;
        } break;

        case NodeKind::Obstacle:
            mPhase = RunPhase::Obstacle;
            r.what = Arrival::Obstacle;
            break;

        case NodeKind::Hut:
            mState.stamina = runnerMaxWithPartners(mState).stamina;
            mState.hutNode = mState.node;
            r.what         = Arrival::Hut;
            break;

        case NodeKind::Cache:
            if ((mState.cachesTaken & stageBit(mState.stage)) == 0) {
                r.what    = Arrival::Found;
                r.item    = at.param;
                r.bagFull = !addItem(at.param);
                if (!r.bagFull) {
                    mState.cachesTaken = static_cast<uint8_t>(mState.cachesTaken | stageBit(mState.stage));
                }
            }
            break;

        case NodeKind::Boss:
            // The runner gathers themself at the foot of it: full stamina.
            mState.stamina = runnerMaxWithPartners(mState).stamina;
            startDuel(stage().boss, bossLevel(stage(), mState.lap));
            r.what = Arrival::Boss;
            break;

        case NodeKind::Start:
        default:
            break;
    }
    if (mPhase == RunPhase::AtNode) {
        rest(RunPhase::AtNode);   // nothing to resolve here: a step ended on the map
    }
    syncRng();
    return r;
}

uint8_t Run::obstacleOdds(BeatResult beat) const
{
    if (mPhase != RunPhase::Obstacle) {
        return 0;
    }
    const ObstacleDef& ob = obstacleDef(node().param);
    return obstacleChance(ob, statFor(runnerMaxWithPartners(mState), ob.check), beat);
}

ObstacleResult Run::attemptObstacle(BeatResult beat)
{
    ObstacleResult r;
    if (mPhase != RunPhase::Obstacle) {
        return r;
    }
    r.valid   = true;
    r.chance  = obstacleOdds(beat);
    r.cleared = mRng.chance(r.chance);
    if (r.cleared) {
        uint8_t levels = 0;
        gainXp(kXpObstacle, levels);
    } else {
        const ObstacleDef& ob = obstacleDef(node().param);
        // An obstacle hurts, but never spends the runner.
        const uint16_t before = mState.stamina;
        mState.stamina = mState.stamina > ob.failStamina ? static_cast<uint16_t>(mState.stamina - ob.failStamina) : 1u;
        mState.minutes = addMinutes(mState.minutes, ob.failMinutes);
        r.staminaLost  = static_cast<uint16_t>(before - mState.stamina);
        r.minutesLost  = ob.failMinutes;
    }
    rest(RunPhase::AtNode);
    syncRng();
    return r;
}

void Run::startDuel(SpeciesId sp, uint8_t level)
{
    DuelSetup s;
    s.runnerLevel   = mState.level;
    s.runnerMax     = runnerMaxWithPartners(mState);
    s.runnerStamina = mState.stamina;
    s.technique     = Move::TrailCraft;
    s.species       = sp;
    s.speciesLevel  = level;
    s.terrain       = stage().terrain;
    s.weather       = mState.weather < static_cast<uint8_t>(Weather::Count) ? static_cast<Weather>(mState.weather)
                                                                           : Weather::Clear;
    s.dark          = dark();
    s.technique     = bestTechnique(s);
    mDuel.start(s);
    mState.spiritsMet |= speciesBit(sp);
    mLastDuel = DuelResult{};
    mPhase    = RunPhase::InDuel;
}

MoveId Run::bestTechnique(const DuelSetup& base) const
{
    DuelSetup s     = base;
    s.technique     = Move::TrailCraft;
    Duel      probe;
    probe.start(s);
    MoveId    best    = Move::TrailCraft;
    uint16_t  bestDmg = probe.estimate(Action::Technique);
    for (uint8_t i = 0; i < mState.partnerCount && i < kMaxPartners; ++i) {
        s.technique = signatureMove(mState.partners[i]);
        probe.start(s);
        const uint16_t dmg = probe.estimate(Action::Technique);
        if (dmg > bestDmg) {   // first wins a tie: the earlier partner
            bestDmg = dmg;
            best    = s.technique;
        }
    }
    return best;
}

TurnLog Run::duelTurn(const PlayerTurn& turn)
{
    if (mPhase != RunPhase::InDuel) {
        return TurnLog{};
    }
    const TurnLog log = mDuel.play(turn, mRng, mState.bag);
    if (turn.beat == BeatResult::Perfect && turn.action != Action::Fuel && mState.perfects < 0xFFFFu) {
        ++mState.perfects;
    }
    if (mDuel.outcome() != DuelOutcome::Ongoing) {
        finishDuel();
    }
    syncRng();
    return log;
}

void Run::finishDuel()
{
    DuelResult      res;
    const SpeciesId sp = mDuel.setup().species;
    res.outcome = mDuel.outcome();
    res.boss    = isBoss(sp);

    if (res.outcome == DuelOutcome::Won) {
        if (mState.duelsWon < 0xFFFFu) {
            ++mState.duelsWon;
        }
        mState.spiritsBeaten |= speciesBit(sp);
        mState.stamina = mDuel.runner().stamina > 0 ? mDuel.runner().stamina : 1u;
        res.xp = xpForWin(speciesDef(sp), mDuel.spirit().level);
        gainXp(res.xp, res.levelsGained);
        if (res.boss) {
            mState.bossesBeaten = static_cast<uint8_t>(mState.bossesBeaten | stageBit(mState.stage));
            rest(RunPhase::StageClear);
        } else {
            if (mRng.chance(kDropChance)) {
                const ItemId item = pickFind(mRng);
                if (addItem(item)) {
                    res.item = item;
                }
            }
            if (!hasPartner(mState, sp)) {
                mState.offer    = sp;
                res.joinOffered = true;
                rest(RunPhase::JoinOffer);
            } else {
                rest(RunPhase::AtNode);
            }
        }
    } else {
        if (mState.duelsLost < 0xFFFFu) {
            ++mState.duelsLost;
        }
        res.lostItem   = loseAnItem();
        mState.minutes = addMinutes(mState.minutes, kLossMinutes);
        mState.node    = mState.hutNode;
        mState.stamina = runnerMaxWithPartners(mState).stamina;
        rest(RunPhase::AtNode);
    }
    mLastDuel = res;
}

bool Run::acceptJoin(uint8_t replaceSlot)
{
    const SpeciesId sp = mState.offer;
    if (mPhase != RunPhase::JoinOffer || sp >= Spirit::Count || isBoss(sp)) {
        return false;
    }
    if (mState.partnerCount < kMaxPartners) {
        mState.partners[mState.partnerCount++] = sp;
    } else if (replaceSlot < kMaxPartners) {
        mState.partners[replaceSlot] = sp;
    } else {
        return false;   // a full pack needs a name for who leaves
    }
    mState.offer = kNone;
    rest(RunPhase::AtNode);
    return true;
}

void Run::declineJoin()
{
    if (mPhase == RunPhase::JoinOffer) {
        mState.offer = kNone;
        rest(RunPhase::AtNode);
    }
}

void Run::nextStage()
{
    if (mPhase != RunPhase::StageClear) {
        return;
    }
    if (mState.stage + 1u >= kStageCount) {
        rest(RunPhase::Finished);
        return;
    }
    startStage(static_cast<uint8_t>(mState.stage + 1u));
}

void Run::startNextLap()
{
    if (mPhase != RunPhase::Finished) {
        return;
    }
    mState.lap          = mState.lap < kMaxLap ? static_cast<uint8_t>(mState.lap + 1u) : kMaxLap;
    mState.bossesBeaten = 0;
    mState.cachesTaken  = 0;
    // Morning of the next day. The clock is 16 bits (45 days); past that it
    // keeps the time of day and starts again from day 0.
    const uint32_t day = static_cast<uint32_t>(mState.minutes) / kDayMinutes + 1u;
    mState.minutes = static_cast<uint16_t>((day % 40u) * kDayMinutes + kRunStartMinutes);
    startStage(0);
}

} // namespace TrailQuest
