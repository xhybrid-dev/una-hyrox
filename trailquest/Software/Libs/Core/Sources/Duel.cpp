/**
 ******************************************************************************
 * @file    Duel.cpp
 * @brief   One duel, turn by turn (see Duel.hpp for the rules).
 ******************************************************************************
 */

#include "Duel.hpp"

#include "Progress.hpp"

namespace TrailQuest
{

namespace
{

uint8_t lower(uint8_t v, uint8_t by)
{
    return v > by ? static_cast<uint8_t>(v - by) : 1u;
}

uint16_t take(uint16_t stamina, uint32_t amount)
{
    return amount >= stamina ? 0u : static_cast<uint16_t>(stamina - amount);
}

uint16_t give(uint16_t stamina, uint32_t amount, uint16_t max)
{
    const uint32_t v = static_cast<uint32_t>(stamina) + amount;
    return v > max ? max : static_cast<uint16_t>(v);
}

Side other(Side s) { return s == Side::Runner ? Side::Spirit : Side::Runner; }

/// @p dmg times num/den, at least 1.
uint32_t scaled(uint32_t dmg, uint32_t num, uint32_t den)
{
    const uint32_t v = (dmg * num + den / 2u) / den;
    return v < 1u ? 1u : v;
}

} // namespace

void Duel::start(const DuelSetup& setup)
{
    mSetup   = setup;
    mTurn    = 0;
    mRested  = false;
    mTurnTell = Twist::None;
    mBraced   = false;
    mOutcome = DuelOutcome::Ongoing;

    mRunner         = Fighter{};
    mRunner.max     = setup.runnerMax;
    if (mRunner.max.stamina == 0) {
        mRunner.max.stamina = 1;
    }
    mRunner.stamina = setup.runnerStamina == 0 ? 1u
                    : (setup.runnerStamina > mRunner.max.stamina ? mRunner.max.stamina : setup.runnerStamina);
    mRunner.level   = clampLevel(setup.runnerLevel);

    const SpeciesDef& sp = speciesDef(setup.species);
    mSpirit          = Fighter{};
    mSpirit.max      = speciesStats(sp, setup.speciesLevel);
    {
        const uint32_t st = static_cast<uint32_t>(mSpirit.max.stamina) * setup.staminaPct / 100u;
        mSpirit.max.stamina = static_cast<uint16_t>(st < 1u ? 1u : (st > 999u ? 999u : st));
    }
    mSpirit.stamina  = mSpirit.max.stamina;
    mSpirit.affinity = sp.affinity;
    mSpirit.level    = clampLevel(setup.speciesLevel);

    if (mSetup.technique >= Move::Count) {
        mSetup.technique = Move::TrailCraft;
    }
}

Duel::Eff Duel::effective(const Fighter& f) const
{
    Eff e;
    e.pace = f.max.pace;
    e.form = static_cast<uint8_t>(f.max.form + f.formBoost > 255 ? 255 : f.max.form + f.formBoost);
    e.grit = f.max.grit;
    if (f.effect == Effect::Slow) {
        e.pace = lower(e.pace, kEffectStatDrop);
    } else if (f.effect == Effect::Rattle) {
        e.grit = lower(e.grit, kEffectStatDrop);
    }
    return e;
}

Twist Duel::tell() const
{
    const SpeciesDef& sp = speciesDef(mSetup.species);
    if (mOutcome != DuelOutcome::Ongoing || (sp.flags & kSpeciesBoss) == 0 || sp.twist == Twist::None
        || sp.twistPeriod == 0) {
        return Twist::None;
    }
    const uint32_t n = static_cast<uint32_t>(mTurn) + 1u;   // the turn about to be played, counting from 1
    if (n % sp.twistPeriod != 0) {
        return Twist::None;
    }
    if (sp.twist == Twist::Alternate) {
        return (n / sp.twistPeriod) % 2u == 1u ? Twist::Charge : Twist::Guard;
    }
    return sp.twist;
}

MoveId Duel::moveFor(Action a) const
{
    switch (a) {
        case Action::Surge:     return Move::Surge;
        case Action::Settle:    return Move::Settle;
        case Action::Technique: return mSetup.technique;
        default:                return kNone;
    }
}

Affinity Duel::affinityFor(MoveId m) const
{
    return m == Move::TrailCraft ? favouredBy(mSetup.terrain) : moveDef(m).affinity;
}

namespace
{
uint8_t stat(uint8_t pace, uint8_t form, uint8_t grit, Affinity a)
{
    switch (a) {
        case Affinity::Pace: return pace;
        case Affinity::Form: return form;
        case Affinity::Grit: return grit;
        default:             return form;
    }
}
} // namespace

uint16_t Duel::damage(Side by, MoveId move, Affinity aff, BeatResult beat) const
{
    const Fighter& a  = fighter(by);
    const Fighter& d  = fighter(other(by));
    const Eff      ea = effective(a);
    const Eff      ed = effective(d);

    int32_t power = moveDef(move).power;
    if (aff == favouredBy(mSetup.terrain)) {
        power += 2;
    }
    if (mSetup.weather == Weather::Rain && aff == Affinity::Form) {
        power -= 1;
    }
    if (mSetup.weather == Weather::Wind && aff == Affinity::Pace) {
        power -= 1;
    }
    if (power < 1) {
        power = 1;
    }

    const uint32_t atk = stat(ea.pace, ea.form, ea.grit, aff);
    const uint32_t def = stat(ed.pace, ed.form, ed.grit, aff);
    // Hundredths, so the multipliers below round once, at the end.
    uint32_t x = static_cast<uint32_t>(power) * 100u * (10u + atk) / (10u + def);

    if (d.affinity != Affinity::Count) {
        const int8_t w = wheel(aff, d.affinity);
        if (w > 0) {
            x = x * 3u / 2u;
        } else if (w < 0) {
            x = x * 3u / 4u;
        }
    }
    if (by == Side::Runner) {
        if (beat == BeatResult::Miss) {
            x = x / 2u;
        } else if (beat == BeatResult::Perfect) {
            x = x * 3u / 2u;
        }
    }

    uint32_t dmg = (x + 50u) / 100u;
    if (static_cast<uint32_t>(d.stamina) * 4u < d.max.stamina) {
        dmg += ea.grit / 2u;   // the finishing push
    }
    dmg = levelScale(dmg, a.level);   // grows with stamina, so fights keep their length
    if (dmg < 1u) {
        dmg = 1u;
    }
    return static_cast<uint16_t>(dmg > 999u ? 999u : dmg);
}

uint8_t Duel::hitChance(Side by) const
{
    const Eff ea = effective(fighter(by));
    const Eff ed = effective(fighter(other(by)));
    int32_t c = kHitBase + kHitPerPace * (static_cast<int32_t>(ea.pace) - static_cast<int32_t>(ed.pace));
    c = c < kHitMin ? kHitMin : (c > kHitMax ? kHitMax : c);
    if (mSetup.weather == Weather::Mist) {
        c -= kHitMistOrDark;
    }
    if (by == Side::Runner && mSetup.dark) {
        c -= kHitMistOrDark;
    }
    return static_cast<uint8_t>(c < 0 ? 0 : c);
}

uint16_t Duel::estimate(Action a, BeatResult beat) const
{
    const MoveId m = moveFor(a);
    if (m == kNone) {
        return 0;
    }
    return damage(Side::Runner, m, affinityFor(m), beat);
}

void Duel::act(Side by, MoveId move, BeatResult beat, Rng& rng, TurnLog& log)
{
    const MoveDef& md  = moveDef(move);
    const Affinity aff = by == Side::Runner ? affinityFor(move) : md.affinity;
    Fighter&       target = fighter(other(by));

    DuelEvent e;
    e.actor = by;
    e.move  = move;
    if (!rng.chance(hitChance(by))) {
        e.kind = EventKind::Missed;
        log.add(e);
        return;
    }
    uint16_t dmg = damage(by, move, aff, beat);
    // A twist's turn: the boss's charge hits double (half if braced); its guard
    // halves the runner's hit. Said after the hit, so the playback shows it.
    EventKind twistEvent = EventKind::Hit;   // Hit: none
    Side      twistBy    = Side::Spirit;
    if (by == Side::Spirit && mTurnTell == Twist::Charge) {
        dmg        = static_cast<uint16_t>(mBraced ? scaled(dmg, kBraceNum, kBraceDen) : scaled(dmg, kChargeNum, kChargeDen));
        twistEvent = mBraced ? EventKind::Braced : EventKind::Charged;
        twistBy    = mBraced ? Side::Runner : Side::Spirit;
    } else if (by == Side::Runner && mTurnTell == Twist::Guard) {
        dmg        = static_cast<uint16_t>(scaled(dmg, kGuardNum, kGuardDen));
        twistEvent = EventKind::Guarded;
    }
    target.stamina = take(target.stamina, dmg);
    e.kind   = EventKind::Hit;
    e.amount = dmg;
    log.add(e);
    if (twistEvent != EventKind::Hit) {
        DuelEvent t;
        t.kind   = twistEvent;
        t.actor  = twistBy;
        t.move   = move;
        t.amount = dmg;
        log.add(t);
    }

    if (md.effect != Effect::None && target.effect == Effect::None && target.stamina > 0
        && rng.chance(md.effectChance)) {
        target.effect      = md.effect;
        target.effectTurns = kEffectTurns;
        DuelEvent fx;
        fx.kind   = EventKind::Inflicted;
        fx.actor  = by;
        fx.move   = move;
        fx.effect = md.effect;
        log.add(fx);
    }
}

void Duel::runnerAct(const PlayerTurn& t, Rng& rng, uint8_t* bag, TurnLog& log)
{
    switch (t.action) {
        case Action::Surge: {
            const uint8_t cost = moveDef(Move::Surge).selfCost;
            // The push costs stamina, but never spends the runner by itself.
            mRunner.stamina = mRunner.stamina > cost ? static_cast<uint16_t>(mRunner.stamina - cost) : 1u;
            act(Side::Runner, Move::Surge, t.beat, rng, log);
        } break;

        case Action::Settle: {
            act(Side::Runner, Move::Settle, t.beat, rng, log);
            const uint16_t before = mRunner.stamina;
            mRunner.stamina = give(mRunner.stamina, levelScale(kSettleRecover, mRunner.level), mRunner.max.stamina);
            DuelEvent e;
            e.kind   = EventKind::Healed;
            e.actor  = Side::Runner;
            e.move   = Move::Settle;
            e.amount = static_cast<uint16_t>(mRunner.stamina - before);
            log.add(e);
        } break;

        case Action::Technique:
            act(Side::Runner, mSetup.technique, t.beat, rng, log);
            break;

        case Action::Fuel:
        default: {
            DuelEvent e;
            e.actor = Side::Runner;
            e.item  = t.item;
            const bool usable = t.action == Action::Fuel && bag != nullptr && t.item < Item::Count
                             && !itemDef(t.item).key && bag[t.item] > 0;
            if (!usable) {
                e.kind = EventKind::NoItem;
                log.add(e);
                return;
            }
            const ItemDef& it = itemDef(t.item);
            --bag[t.item];
            const uint16_t before = mRunner.stamina;
            mRunner.stamina = give(mRunner.stamina, levelScale(it.heal, mRunner.level), mRunner.max.stamina);
            if (it.formBoost > 0) {
                const uint32_t boost = mRunner.formBoost + it.formBoost;
                mRunner.formBoost = static_cast<uint8_t>(boost > 6u ? 6u : boost);
            }
            e.kind   = EventKind::UsedItem;
            e.amount = static_cast<uint16_t>(mRunner.stamina - before);
            log.add(e);
            if (it.clears != Effect::None && mRunner.effect == it.clears) {
                DuelEvent w;
                w.kind   = EventKind::Wore;
                w.actor  = Side::Runner;
                w.effect = mRunner.effect;
                log.add(w);
                mRunner.effect      = Effect::None;
                mRunner.effectTurns = 0;
            }
        } break;
    }
}

void Duel::spiritAct(Rng& rng, TurnLog& log)
{
    const SpeciesDef& sp = speciesDef(mSetup.species);
    if ((sp.flags & kSpeciesBoss) != 0 && !mRested
        && static_cast<uint32_t>(mSpirit.stamina) * 100u < kBossRestBelowPct * mSpirit.max.stamina) {
        mRested = true;
        const uint16_t before = mSpirit.stamina;
        mSpirit.stamina = give(mSpirit.stamina, static_cast<uint32_t>(mSpirit.max.stamina) * kBossRestPct / 100u,
                               mSpirit.max.stamina);
        DuelEvent e;
        e.kind   = EventKind::Rested;
        e.actor  = Side::Spirit;
        e.amount = static_cast<uint16_t>(mSpirit.stamina - before);
        log.add(e);
        return;
    }
    const MoveId m = rng.below(100) < sp.firstMoveWeight ? sp.moves[0] : sp.moves[1];
    act(Side::Spirit, m, BeatResult::Good, rng, log);
}

void Duel::endOfTurn(TurnLog& log)
{
    static const Side kSides[] = { Side::Runner, Side::Spirit };
    for (Side s : kSides) {
        Fighter& f = fighter(s);
        uint32_t loss = 0;
        if (f.effect == Effect::Drain) {
            loss += kDrainPerTurn;
        }
        if (s == Side::Runner && mSetup.weather == Weather::Snow) {
            loss += 1;
        }
        loss = levelScale(loss, f.level);
        if (loss > 0) {
            f.stamina = take(f.stamina, loss);
            DuelEvent e;
            e.kind   = EventKind::Ticked;
            e.actor  = s;
            e.amount = static_cast<uint16_t>(loss);
            e.effect = f.effect;
            log.add(e);
        }
        if (f.effectTurns > 0) {
            --f.effectTurns;
            if (f.effectTurns == 0) {
                DuelEvent e;
                e.kind   = EventKind::Wore;
                e.actor  = s;
                e.effect = f.effect;
                log.add(e);
                f.effect = Effect::None;
            }
        }
    }
}

bool Duel::checkOver()
{
    if (mOutcome != DuelOutcome::Ongoing) {
        return true;
    }
    // The spirit first: if both are spent at once (end-of-turn drain), the
    // runner, who kept going, has it.
    if (mSpirit.stamina == 0) {
        mOutcome = DuelOutcome::Won;
    } else if (mRunner.stamina == 0) {
        mOutcome = DuelOutcome::Lost;
    }
    return mOutcome != DuelOutcome::Ongoing;
}

TurnLog Duel::play(const PlayerTurn& turn, Rng& rng, uint8_t* bag)
{
    TurnLog log;
    if (mOutcome != DuelOutcome::Ongoing) {
        return log;
    }

    mTurnTell = tell();
    mBraced   = turn.action == Action::Settle;
    log.runnerFirst = effective(mRunner).pace >= effective(mSpirit).pace;
    if (log.runnerFirst) {
        runnerAct(turn, rng, bag, log);
        if (!checkOver()) {
            spiritAct(rng, log);
        }
    } else {
        spiritAct(rng, log);
        if (!checkOver()) {
            runnerAct(turn, rng, bag, log);
        }
    }
    if (!checkOver()) {
        endOfTurn(log);
        checkOver();
    }

    ++mTurn;
    if (mOutcome == DuelOutcome::Ongoing && mTurn >= kMaxTurns) {
        // Out of time: the larger share of stamina left wins; a tie is the spirit's.
        const uint32_t runnerShare = static_cast<uint32_t>(mRunner.stamina) * mSpirit.max.stamina;
        const uint32_t spiritShare = static_cast<uint32_t>(mSpirit.stamina) * mRunner.max.stamina;
        mOutcome = runnerShare > spiritShare ? DuelOutcome::Won : DuelOutcome::Lost;
    }
    return log;
}

} // namespace TrailQuest
