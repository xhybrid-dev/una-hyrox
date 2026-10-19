// The duel: hand-worked damage figures, the wheel, the beat, the terrain and
// weather, items, effects, a boss's rest, the turn cap, and determinism.
//
// The figures are worked by hand from the rule in Duel.hpp and the tables (the
// tuned numbers of docs/BALANCE.md). A level 1 runner has stamina 40 and pace,
// form and grit 10; at level 1 the level scale is exactly 1. At level 1:
//   Red Fox     (Pace) stamina 36, pace 11, form 10, grit 9
//   Brown Bear  (Grit) stamina 48, pace 8,  form 10, grit 11
//   Ruby Beetle (Form) stamina 38, pace 10, form 10, grit 9
// The runner's moves: Surge 14 Pace (costs 4), Settle 5 Grit, Trail craft 9
// in the terrain's affinity (Form on the Path, + 2 for suiting it).

#include <gtest/gtest.h>

#include "Duel.hpp"
#include "Progress.hpp"

using namespace TrailQuest;

namespace
{

DuelSetup setupFor(SpeciesId sp, uint8_t spLevel = 1, uint8_t runnerLevel = 1)
{
    DuelSetup s;
    s.runnerLevel   = runnerLevel;
    s.runnerMax     = runnerStats(runnerLevel);
    s.runnerStamina = s.runnerMax.stamina;
    s.species       = sp;
    s.speciesLevel  = spLevel;
    s.terrain       = Terrain::Path;   // favours Form
    s.weather       = Weather::Clear;
    return s;
}

/// Play a whole duel with one action, returning every event in order.
struct Played {
    DuelOutcome outcome;
    uint8_t     turns;
    uint32_t    events;
    uint32_t    checksum;
};

Played playOut(const DuelSetup& setup, uint32_t seed, Action action)
{
    Duel d;
    d.start(setup);
    Rng r(seed);
    uint8_t bag[Item::Count] = {};
    Played p {};
    while (d.outcome() == DuelOutcome::Ongoing) {
        PlayerTurn t;
        t.action = action;
        const TurnLog log = d.play(t, r, bag);
        for (uint8_t i = 0; i < log.count; ++i) {
            const DuelEvent& e = log.events[i];
            p.checksum = p.checksum * 31u + static_cast<uint32_t>(e.kind) * 7u + e.amount
                       + static_cast<uint32_t>(e.actor) * 3u + static_cast<uint32_t>(e.effect);
            ++p.events;
        }
    }
    p.outcome = d.outcome();
    p.turns   = d.turn();
    return p;
}

} // namespace

TEST(Duel, StartsFromTheSetupAndTheSpiritsRow)
{
    DuelSetup s = setupFor(Spirit::RedFox);
    s.runnerStamina = 25;
    Duel d;
    d.start(s);
    EXPECT_EQ(d.runner().stamina, 25u);
    EXPECT_EQ(d.runner().max.stamina, 40u);
    EXPECT_EQ(d.spirit().stamina, 36u);
    EXPECT_EQ(d.spirit().max.pace, 11u);
    EXPECT_EQ(d.spirit().affinity, Affinity::Pace);
    EXPECT_EQ(d.outcome(), DuelOutcome::Ongoing);
    EXPECT_EQ(d.turn(), 0u);
}

TEST(Duel, ARunnerCarriedInWithMoreThanFullIsHeldToFull)
{
    DuelSetup s = setupFor(Spirit::RedFox);
    s.runnerStamina = 999;
    Duel d;
    d.start(s);
    EXPECT_EQ(d.runner().stamina, 40u);
}

TEST(Duel, SurgeAgainstRedFoxWorkedByHand)
{
    // 14 x (10+10)/(10+11) = 13.3; no wheel (Pace on Pace); rounds to 13.
    Duel d;
    d.start(setupFor(Spirit::RedFox));
    EXPECT_EQ(d.estimate(Action::Surge), 13u);
    EXPECT_EQ(d.estimate(Action::Surge, BeatResult::Perfect), 20u);   // x 3/2: 19.99
    EXPECT_EQ(d.estimate(Action::Surge, BeatResult::Miss), 7u);        // x 1/2: 6.66
}

TEST(Duel, SettleIsGritAndBeatsPace)
{
    // Grit, not suited to the Path: 5. 5 x 20/19 = 5.26, x 3/2 (Grit beats Pace) = 7.9 -> 8.
    Duel d;
    d.start(setupFor(Spirit::RedFox));
    EXPECT_EQ(d.estimate(Action::Settle), 8u);
}

TEST(Duel, TrailCraftTakesTheTerrainsAffinity)
{
    // Form on the Path: 9 + 2 = 11. 11 x 20/20 = 11, x 3/4 (Form loses to Pace) = 8.25 -> 8.
    Duel d;
    d.start(setupFor(Spirit::RedFox));
    EXPECT_EQ(d.moveFor(Action::Technique), Move::TrailCraft);
    EXPECT_EQ(d.affinityFor(Move::TrailCraft), Affinity::Form);
    EXPECT_EQ(d.estimate(Action::Technique), 8u);

    DuelSetup ridge = setupFor(Spirit::RedFox);
    ridge.terrain = Terrain::Ridge;   // favours Pace
    d.start(ridge);
    EXPECT_EQ(d.affinityFor(Move::TrailCraft), Affinity::Pace);
}

TEST(Duel, APartnersTechniqueUsesItsMoveAndTheWheel)
{
    // The Brown Bear's Lumber is Grit (7); Grit beats Pace: 7 x 20/19 = 7.37, x 3/2 = 11.
    DuelSetup s = setupFor(Spirit::RedFox);
    s.technique = Move::Lumber;
    Duel d;
    d.start(s);
    EXPECT_EQ(d.moveFor(Action::Technique), Move::Lumber);
    EXPECT_EQ(d.estimate(Action::Technique), 11u);
}

TEST(Duel, AnInvalidTechniqueFallsBackToTrailCraft)
{
    DuelSetup s = setupFor(Spirit::RedFox);
    s.technique = 250;
    Duel d;
    d.start(s);
    EXPECT_EQ(d.moveFor(Action::Technique), Move::TrailCraft);
}

TEST(Duel, TheWheelRewardsTheRightMove)
{
    // Surge (Pace) against the Ruby Beetle (Form): 14 x 20/20 = 14, x 3/2 = 21.
    Duel d;
    d.start(setupFor(Spirit::RubyBeetle));
    EXPECT_EQ(d.estimate(Action::Surge), 21u);
}

TEST(Duel, RainWeakensFormMoves)
{
    // Trail craft (Form on the Path) against the Ruby Beetle (Form, no wheel):
    // 11 x 20/20 = 11 dry; in the rain 10.
    DuelSetup s = setupFor(Spirit::RubyBeetle);
    Duel d;
    d.start(s);
    EXPECT_EQ(d.estimate(Action::Technique), 11u);
    s.weather = Weather::Rain;
    d.start(s);
    EXPECT_EQ(d.estimate(Action::Technique), 10u);
    EXPECT_EQ(d.estimate(Action::Surge), 21u);   // Pace: unchanged
}

TEST(Duel, WindWeakensPaceMovesForBothSides)
{
    // Surge against the Ruby Beetle in the wind: 14 - 1 = 13, x 3/2 = 19.5 -> 20.
    DuelSetup s = setupFor(Spirit::RubyBeetle);
    s.weather = Weather::Wind;
    Duel d;
    d.start(s);
    EXPECT_EQ(d.estimate(Action::Surge), 20u);
    EXPECT_EQ(d.estimate(Action::Technique), 11u);   // Form: unchanged
}

TEST(Duel, HitChanceFollowsPaceMistAndTheDark)
{
    DuelSetup s = setupFor(Spirit::RedFox);
    Duel d;
    d.start(s);
    EXPECT_EQ(d.chanceToLand(Side::Runner), 88u);   // 90 + 2 x (10 - 11)
    EXPECT_EQ(d.chanceToLand(Side::Spirit), 92u);
    s.weather = Weather::Mist;
    s.dark    = true;
    d.start(s);
    EXPECT_EQ(d.chanceToLand(Side::Runner), 68u);   // less 10 for mist, 10 for the dark
    EXPECT_EQ(d.chanceToLand(Side::Spirit), 82u);   // mist only
}

TEST(Duel, TheFasterSideGoesFirstAndTheRunnerOnATie)
{
    uint8_t bag[Item::Count] = {};
    Rng r(5);
    Duel d;
    d.start(setupFor(Spirit::BrownBear));   // pace 8: slower than the runner
    EXPECT_TRUE(d.play(PlayerTurn{}, r, bag).runnerFirst);
    d.start(setupFor(Spirit::RedFox));   // pace 11: faster
    EXPECT_FALSE(d.play(PlayerTurn{}, r, bag).runnerFirst);
    DuelSetup level = setupFor(Spirit::RubyBeetle);   // pace 10: level
    d.start(level);
    EXPECT_TRUE(d.play(PlayerTurn{}, r, bag).runnerFirst);
    level.weather = Weather::Wind;   // wind weakens Pace moves, not the order
    d.start(level);
    EXPECT_TRUE(d.play(PlayerTurn{}, r, bag).runnerFirst);
}

TEST(Duel, TheFinishingPushAddsHalfTheRunnersGritBelowAQuarter)
{
    // A level 5 runner (14 each, level scale 14/10) against a level 1 Brown Bear (48).
    // Surge: 14 x (10+14)/(10+8) = 18.7, x 3/4 (Pace loses to Grit) = 14.0 -> 14,
    // x 1.4 = 19. Below a quarter (under 12) the runner adds 14 / 2 = 7 before
    // the scale: 21 x 1.4 = 29. If the Brown Bear has slowed the runner (pace 12):
    // 14 x 22/18 x 3/4 = 12.8 -> 13, + 7 = 20, x 1.4 = 28.
    Duel d;
    d.start(setupFor(Spirit::BrownBear, 1, 5));
    ASSERT_EQ(d.estimate(Action::Surge), 19u);
    Rng r(11);
    uint8_t bag[Item::Count] = {};
    bool sawFinish = false;
    for (int i = 0; i < 12 && d.outcome() == DuelOutcome::Ongoing; ++i) {
        if (static_cast<uint32_t>(d.spirit().stamina) * 4u < d.spirit().max.stamina) {
            EXPECT_EQ(d.estimate(Action::Surge), d.runner().effect == Effect::Slow ? 28u : 29u);
            sawFinish = true;
            break;
        }
        PlayerTurn t;   // a missed beat: 9 a hit (7 x 1.4), so 48, 39, 30, 21, 12, 3
        t.beat = BeatResult::Miss;
        d.play(t, r, bag);
    }
    EXPECT_TRUE(sawFinish);
}

TEST(Duel, StaminaAddsUpOverATurn)
{
    // Surge costs 4; the runner then loses exactly what the log says.
    Duel d;
    d.start(setupFor(Spirit::BrownBear));
    Rng r(3);
    uint8_t bag[Item::Count] = {};
    const uint16_t before = d.runner().stamina;
    const TurnLog log = d.play(PlayerTurn{}, r, bag);
    uint32_t lost = 4, healed = 0;
    for (uint8_t i = 0; i < log.count; ++i) {
        const DuelEvent& e = log.events[i];
        if (e.kind == EventKind::Hit && e.actor == Side::Spirit) {
            lost += e.amount;
        }
        if (e.kind == EventKind::Ticked && e.actor == Side::Runner) {
            lost += e.amount;
        }
        if (e.kind == EventKind::Healed && e.actor == Side::Runner) {
            healed += e.amount;
        }
    }
    EXPECT_EQ(static_cast<uint32_t>(d.runner().stamina), before - lost + healed);
}

TEST(Duel, SurgeNeverSpendsTheRunnerByItself)
{
    DuelSetup s = setupFor(Spirit::BrownBear);   // the runner acts first
    s.runnerStamina = 3;
    Duel d;
    d.start(s);
    Rng r(1);
    uint8_t bag[Item::Count] = {};
    const TurnLog log = d.play(PlayerTurn{}, r, bag);
    ASSERT_GT(log.count, 0u);
    EXPECT_EQ(log.events[0].actor, Side::Runner);   // it acted: the cost did not end it
}

TEST(Duel, FuelUsesAnItemFromTheBag)
{
    DuelSetup s = setupFor(Spirit::RedFox);
    s.runnerStamina = 10;
    Duel d;
    d.start(s);
    Rng r(2);
    uint8_t bag[Item::Count] = {};
    bag[Item::Gel] = 1;
    PlayerTurn t;
    t.action = Action::Fuel;
    t.item   = Item::Gel;
    const TurnLog log = d.play(t, r, bag);
    EXPECT_EQ(bag[Item::Gel], 0u);
    bool used = false;
    for (uint8_t i = 0; i < log.count; ++i) {
        if (log.events[i].kind == EventKind::UsedItem) {
            EXPECT_EQ(log.events[i].amount, 15u);
            used = true;
        }
    }
    EXPECT_TRUE(used);
}

TEST(Duel, FuelWithNothingUsableLosesTheTurn)
{
    for (ItemId item : { static_cast<ItemId>(Item::Gel), static_cast<ItemId>(Item::Headtorch), static_cast<ItemId>(99) }) {
        Duel d;
        d.start(setupFor(Spirit::BrownBear));
        Rng r(4);
        uint8_t bag[Item::Count] = {};
        bag[Item::Headtorch] = 1;
        PlayerTurn t;
        t.action = Action::Fuel;
        t.item   = item;
        const TurnLog log = d.play(t, r, bag);
        ASSERT_GT(log.count, 0u);
        EXPECT_EQ(log.events[0].kind, EventKind::NoItem) << int(item);
        EXPECT_EQ(bag[Item::Headtorch], 1u);
    }
    Duel d;
    d.start(setupFor(Spirit::BrownBear));
    Rng r(4);
    PlayerTurn t;
    t.action = Action::Fuel;
    t.item   = Item::Gel;
    EXPECT_EQ(d.play(t, r, nullptr).events[0].kind, EventKind::NoItem);
}

TEST(Duel, PolesAddFormForTheRestOfTheDuel)
{
    Duel d;
    d.start(setupFor(Spirit::BrownBear));
    const uint16_t before = d.estimate(Action::Technique);   // Trail craft: Form on the Path
    Rng r(8);
    uint8_t bag[Item::Count] = {};
    bag[Item::Poles] = 1;
    PlayerTurn t;
    t.action = Action::Fuel;
    t.item   = Item::Poles;
    d.play(t, r, bag);
    EXPECT_EQ(d.runner().formBoost, 2u);
    EXPECT_GT(d.estimate(Action::Technique), before);
}

TEST(Duel, EffectsLastThreeTurnsAndOneAtATime)
{
    // Many duels against spirits with effects; every effect put on the runner
    // must wear off exactly at the end of its third turn (or the duel ends).
    int seen = 0;
    for (uint32_t seed = 1; seed <= 200; ++seed) {
        for (SpeciesId sp : { static_cast<SpeciesId>(Spirit::RubyBeetle), static_cast<SpeciesId>(Spirit::GnatSwarm), static_cast<SpeciesId>(Spirit::MistWisp) }) {
            Duel d;
            d.start(setupFor(sp, 2, 3));
            Rng r(seed);
            uint8_t bag[Item::Count] = {};
            int inflictedAt = -1;
            while (d.outcome() == DuelOutcome::Ongoing) {
                const uint8_t turn = d.turn();
                PlayerTurn t;
                t.action = Action::Settle;
                const TurnLog log = d.play(t, r, bag);
                for (uint8_t i = 0; i < log.count; ++i) {
                    const DuelEvent& e = log.events[i];
                    if (e.kind == EventKind::Inflicted && e.actor == Side::Spirit) {
                        EXPECT_EQ(inflictedAt, -1) << "a second effect on top of the first";
                        inflictedAt = turn;
                        ++seen;
                    }
                    if (e.kind == EventKind::Wore && e.actor == Side::Runner) {
                        EXPECT_EQ(turn - inflictedAt, kEffectTurns - 1) << "seed " << seed;
                        inflictedAt = -1;
                    }
                }
                EXPECT_LE(d.runner().effectTurns, kEffectTurns);
            }
        }
    }
    EXPECT_GT(seen, 50);
}

TEST(Duel, ABossRestsOnceWhenLow)
{
    int rested = 0;
    for (uint32_t seed = 1; seed <= 50; ++seed) {
        Duel d;
        d.start(setupFor(Spirit::OldTortoise, 2, 6));   // a strong runner
        Rng r(seed);
        uint8_t bag[Item::Count] = {};
        int rests = 0;
        while (d.outcome() == DuelOutcome::Ongoing) {
            const TurnLog log = d.play(PlayerTurn{}, r, bag);
            for (uint8_t i = 0; i < log.count; ++i) {
                rests += log.events[i].kind == EventKind::Rested ? 1 : 0;
            }
        }
        EXPECT_LE(rests, 1) << "seed " << seed;
        rested += rests;
    }
    EXPECT_GT(rested, 10);   // it does happen
}

TEST(Duel, NoDuelOutlastsTheTurnCap)
{
    for (uint32_t seed = 1; seed <= 100; ++seed) {
        const Played p = playOut(setupFor(Spirit::SummitWolf, 14, 14), seed, Action::Settle);
        EXPECT_NE(p.outcome, DuelOutcome::Ongoing);
        EXPECT_LE(p.turns, kMaxTurns);
    }
}

TEST(Duel, AtTheCapTheLargerShareWins)
{
    // A slow fight: settling, beat missed, against the Summit Wolf at par
    // (about a third of these run to the cap). Whenever one does, the outcome
    // must follow the shares.
    int capped = 0;
    for (uint32_t seed = 1; seed <= 300; ++seed) {
        Duel d;
        d.start(setupFor(Spirit::SummitWolf, 14, 14));
        Rng r(seed);
        uint8_t bag[Item::Count] = {};
        while (d.outcome() == DuelOutcome::Ongoing) {
            PlayerTurn t;
            t.action = Action::Settle;
            t.beat   = BeatResult::Miss;
            d.play(t, r, bag);
        }
        if (d.turn() == kMaxTurns && d.runner().stamina > 0 && d.spirit().stamina > 0) {
            ++capped;
            const uint32_t runnerShare = static_cast<uint32_t>(d.runner().stamina) * d.spirit().max.stamina;
            const uint32_t spiritShare = static_cast<uint32_t>(d.spirit().stamina) * d.runner().max.stamina;
            EXPECT_EQ(d.outcome(), runnerShare > spiritShare ? DuelOutcome::Won : DuelOutcome::Lost);
        }
    }
    EXPECT_GT(capped, 0);
}

TEST(Duel, TheSameSeedAndTurnsReplayTheSameDuel)
{
    for (uint32_t seed = 1; seed <= 50; ++seed) {
        for (SpeciesId sp = 0; sp < Spirit::Count; sp += 3) {
            const DuelSetup s = setupFor(sp, kSpecies[sp].stage * 3, kSpecies[sp].stage * 3);
            const Played a = playOut(s, seed, Action::Surge);
            const Played b = playOut(s, seed, Action::Surge);
            ASSERT_EQ(a.checksum, b.checksum) << int(sp) << " seed " << seed;
            ASSERT_EQ(a.outcome, b.outcome);
            ASSERT_EQ(a.turns, b.turns);
        }
    }
}

TEST(Duel, AnEndedDuelIgnoresFurtherTurns)
{
    Duel d;
    d.start(setupFor(Spirit::RedFox, 1, 20));   // a level 20 runner against a level 1 spirit
    Rng r(1);
    uint8_t bag[Item::Count] = {};
    while (d.outcome() == DuelOutcome::Ongoing) {
        d.play(PlayerTurn{}, r, bag);
    }
    EXPECT_EQ(d.outcome(), DuelOutcome::Won);
    const uint8_t turns = d.turn();
    const TurnLog log = d.play(PlayerTurn{}, r, bag);
    EXPECT_EQ(log.count, 0u);
    EXPECT_EQ(d.turn(), turns);
}
