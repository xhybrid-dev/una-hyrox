/**
 * The simulated player shared by balance_sim and run_sim: how well they hit
 * the beat (a skill profile) and how they choose a move (a sensible policy).
 * Host-only tooling; not part of the app.
 */

#ifndef TRAILQUEST_TOOLS_POLICY_HPP
#define TRAILQUEST_TOOLS_POLICY_HPP

#include <cstdint>

#include "Beat.hpp"
#include "Duel.hpp"
#include "Rng.hpp"

namespace TrailQuest::Sim
{

/// How often a player lands each beat result, per cent.
struct Skill {
    const char* name;
    uint8_t     perfect;
    uint8_t     good;        // the rest are misses
    uint8_t     readsTell;   // per cent of a boss's tells the player answers (Settle)
};

/// perfect and good are the rates on stage 1's beat (beatFor(0, 0)); scaledFor()
/// moves them to another stage's.
inline constexpr Skill kSkills[] = {
    { "casual", 25, 45, 50 },
    { "good",   45, 40, 100 },
    { "menu",    0, 100, 0 },   // menu-only play: every beat counts as Good, and tells go unread
};
constexpr int kSkillCount = 3;

/// The rates on @p beat's needle, from the stage 1 rates: a faster needle costs
/// perfects in proportion to its sweep (a press is as late or early in ms, but
/// harder to place), and a narrower good zone costs good-or-better in
/// proportion to its width. Menu-only play (no beat) is unchanged.
inline Skill scaledFor(const Skill& s, const BeatConfig& beat)
{
    if (s.perfect == 0 && s.good == 100) {
        return s;
    }
    const BeatConfig base = beatFor(0, 0);
    Skill out = s;
    const uint32_t perfect  = static_cast<uint32_t>(s.perfect) * beat.sweepMs / base.sweepMs;
    uint32_t       goodPlus = static_cast<uint32_t>(s.perfect + s.good) * beat.goodMs / base.goodMs;
    goodPlus = goodPlus < perfect ? perfect : goodPlus;
    out.perfect = static_cast<uint8_t>(perfect > 100u ? 100u : perfect);
    out.good    = static_cast<uint8_t>((goodPlus > 100u ? 100u : goodPlus) - out.perfect);
    return out;
}

inline BeatResult sampleBeat(const Skill& s, Rng& r)
{
    const uint32_t x = r.below(100);
    return x < s.perfect ? BeatResult::Perfect : (x < s.perfect + s.good ? BeatResult::Good : BeatResult::Miss);
}

/// Fuel when under 35 % with a gel (or water, when drained); otherwise the
/// move worth most: its damage, less Surge's cost, plus Settle's recovery;
/// no Surge when it would leave the runner nearly spent.
inline PlayerTurn choose(const Duel& d, const uint8_t* bag, const Skill& skill, const BeatConfig& beat, Rng& r)
{
    PlayerTurn t;
    t.beat = sampleBeat(scaledFor(skill, beat), r);
    const Fighter& me = d.runner();
    if (static_cast<uint32_t>(me.stamina) * 100u < 35u * me.max.stamina) {
        if (me.effect == Effect::Drain && bag[Item::Water] > 0) {
            t.action = Action::Fuel;
            t.item   = Item::Water;
            return t;
        }
        if (bag[Item::Gel] > 0) {
            t.action = Action::Fuel;
            t.item   = Item::Gel;
            return t;
        }
    }
    // A boss's tell: a player who reads it Settles (braces a charge, and waits
    // out a guard).
    if (d.tell() != Twist::None && r.below(100) < skill.readsTell) {
        t.action = Action::Settle;
        return t;
    }
    // Value = damage, less what it costs, plus what it recovers (a sensible
    // player sees that Settle's recovery is worth as much as damage).
    int32_t best = -1000;
    t.action = Action::Settle;
    static const Action kMoves[] = { Action::Surge, Action::Settle, Action::Technique };
    for (Action a : kMoves) {
        int32_t value = d.estimate(a);
        if (a == Action::Surge) {
            if (me.stamina <= 8u) {
                continue;
            }
            value -= moveDef(Move::Surge).selfCost;
        } else if (a == Action::Settle) {
            const int32_t missing = static_cast<int32_t>(me.max.stamina) - me.stamina;
            value += missing < kSettleRecover ? missing : kSettleRecover;
        }
        if (value > best) {
            best     = value;
            t.action = a;
        }
    }
    return t;
}

} // namespace TrailQuest::Sim

#endif // TRAILQUEST_TOOLS_POLICY_HPP
