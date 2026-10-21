// The game's flow: a new game, travel, huts, caches, obstacles, duels won and
// lost, join offers and a full pack, bosses, stages and the next lap.

#include <gtest/gtest.h>

#include <set>
#include <utility>

#include "Encounter.hpp"
#include "Run.hpp"

using namespace TrailQuest;

namespace
{

/// Play the current duel to its end with one action.
void fight(TrailQuest::Run& run, Action a = Action::Surge, BeatResult beat = BeatResult::Good)
{
    for (int i = 0; i < 40 && run.phase() == RunPhase::InDuel; ++i) {
        PlayerTurn t;
        t.action = a;
        t.beat   = beat;
        run.duelTurn(t);
    }
}

/// A state standing at @p node of @p stage, at @p level, with nothing beaten.
RunState standingAt(uint8_t stage, uint8_t node, uint8_t level, uint32_t rng = 99)
{
    RunState s;
    s.rng     = rng;
    s.level   = level;
    s.stamina = runnerStats(level).stamina;
    s.stage   = stage;
    s.node    = node;
    s.minutes = 10 * 60;
    s.weather = static_cast<uint8_t>(kStages[stage].weather[0]);
    s.bag[Item::Gel] = 2;
    s.bag[Item::Water] = 1;
    return s;
}

} // namespace

TEST(Run, ANewGameStartsAtTheTrailhead)
{
    TrailQuest::Run run;
    run.newGame(42);
    const RunState& s = run.state();
    EXPECT_EQ(run.phase(), RunPhase::AtNode);
    EXPECT_EQ(s.stage, 0u);
    EXPECT_EQ(s.node, 0u);
    EXPECT_EQ(s.level, 1u);
    EXPECT_EQ(s.stamina, 40u);
    EXPECT_EQ(s.bag[Item::Gel], kStartGels);
    EXPECT_EQ(s.bag[Item::Water], kStartWater);
    EXPECT_EQ(s.minutes, kRunStartMinutes);
    EXPECT_TRUE(s.weather == static_cast<uint8_t>(kStages[0].weather[0])
             || s.weather == static_cast<uint8_t>(kStages[0].weather[1]));
    EXPECT_TRUE(run.wantsSave());
    run.saved();
    EXPECT_FALSE(run.wantsSave());
}

TEST(Run, TheSameSeedAndChoicesReplayTheSameGame)
{
    auto play = [](uint32_t seed) {
        TrailQuest::Run run;
        run.newGame(seed);
        for (int step = 0; step < 200 && run.phase() != RunPhase::Finished; ++step) {
            switch (run.phase()) {
                case RunPhase::AtNode:     run.travel(static_cast<uint8_t>(step % 2 < run.node().exitCount ? step % 2 : 0)); break;
                case RunPhase::Obstacle:   run.attemptObstacle(BeatResult::Good); break;
                case RunPhase::InDuel:     fight(run); break;
                case RunPhase::JoinOffer:  run.acceptJoin(0); break;
                case RunPhase::StageClear: run.nextStage(); break;
                default: break;
            }
        }
        return run.state();
    };
    for (uint32_t seed : {1u, 7u, 2026u}) {
        const RunState a = play(seed);
        const RunState b = play(seed);
        EXPECT_EQ(a.rng, b.rng);
        EXPECT_EQ(a.level, b.level);
        EXPECT_EQ(a.stage, b.stage);
        EXPECT_EQ(a.minutes, b.minutes);
        EXPECT_EQ(a.duelsWon, b.duelsWon);
        EXPECT_EQ(a.spiritsMet, b.spiritsMet);
    }
}

TEST(Run, TravelRefusesABadExitOrTheWrongPhase)
{
    TrailQuest::Run run;
    run.newGame(1);
    EXPECT_EQ(run.travel(5).what, Arrival::Refused);
    EXPECT_EQ(run.state().node, 0u);
    run.resume(standingAt(0, 0, 1));
    ASSERT_EQ(run.travel(0).what, Arrival::Duel);   // to the birch wood
    EXPECT_EQ(run.travel(0).what, Arrival::Refused);   // mid-duel
}

TEST(Run, TravellingTakesTrailTime)
{
    TrailQuest::Run run;
    run.resume(standingAt(0, 0, 1));
    const uint16_t before = run.state().minutes;
    run.travel(1);   // the stream path: 15 minutes, to the stepping stones
    EXPECT_EQ(run.state().minutes, before + 15u);
    EXPECT_EQ(run.phase(), RunPhase::Obstacle);
}

TEST(Run, AnEncounterStartsADuelAndRecordsTheSpirit)
{
    TrailQuest::Run run;
    run.resume(standingAt(0, 0, 1));
    const ArriveResult r = run.travel(0);
    ASSERT_EQ(r.what, Arrival::Duel);
    EXPECT_EQ(run.phase(), RunPhase::InDuel);
    const SpeciesId sp = run.duel().setup().species;
    EXPECT_EQ(kSpecies[sp].stage, 1u);
    EXPECT_NE(run.state().spiritsMet & (1u << sp), 0u);
    EXPECT_GE(run.duel().spirit().level, kStages[0].levelMin);
    EXPECT_LE(run.duel().spirit().level, kStages[0].levelMax);
}

TEST(Run, AnObstacleIsClearedOrCostsStaminaAndTime)
{
    int cleared = 0, failed = 0;
    for (uint32_t seed = 1; seed <= 200; ++seed) {
        TrailQuest::Run run;
        RunState s = standingAt(0, 0, 1, seed);
        run.resume(s);
        ASSERT_EQ(run.travel(1).what, Arrival::Obstacle);
        const uint8_t odds = run.obstacleOdds(BeatResult::Good);
        EXPECT_EQ(odds, 55u);   // form 10 against 11, a good beat: 50 - 5 + 10
        const uint16_t minutes = run.state().minutes;
        const ObstacleResult r = run.attemptObstacle(BeatResult::Good);
        ASSERT_TRUE(r.valid);
        EXPECT_EQ(run.phase(), RunPhase::AtNode);
        if (r.cleared) {
            ++cleared;
            EXPECT_EQ(run.state().minutes, minutes);
        } else {
            ++failed;
            EXPECT_EQ(r.staminaLost, kObstacles[Obstacle::SteppingStones].failStamina);
            EXPECT_EQ(run.state().minutes, minutes + kObstacles[Obstacle::SteppingStones].failMinutes);
        }
    }
    EXPECT_GT(cleared, 70);
    EXPECT_GT(failed, 70);
    TrailQuest::Run idle;
    idle.newGame(1);
    EXPECT_FALSE(idle.attemptObstacle(BeatResult::Good).valid);
}

TEST(Run, AnObstacleNeverSpendsTheRunner)
{
    for (uint32_t seed = 1; seed <= 100; ++seed) {
        TrailQuest::Run run;
        RunState s = standingAt(0, 0, 1, seed);
        s.stamina = 2;
        run.resume(s);
        run.travel(1);
        run.attemptObstacle(BeatResult::Miss);
        EXPECT_GE(run.state().stamina, 1u);
    }
}

TEST(Run, AHutRestoresAndIsWhereALossReturns)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 1, 2);   // birch wood: its exit goes to the hut
    s.stamina = 5;
    run.resume(s);
    const ArriveResult r = run.travel(0);
    EXPECT_EQ(r.what, Arrival::Hut);
    EXPECT_EQ(run.state().stamina, runnerStats(2).stamina);
    EXPECT_EQ(run.state().hutNode, 3u);
    EXPECT_TRUE(run.wantsSave());
}

TEST(Run, ACacheGivesItsItemOnce)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 3, 2);   // the hut: its first exit is the old gate
    run.resume(s);
    const ArriveResult r = run.travel(0);
    ASSERT_EQ(r.what, Arrival::Found);
    EXPECT_EQ(r.item, Item::Gel);
    EXPECT_EQ(run.state().bag[Item::Gel], 3u);
    // Back at the hut (as after a loss), the gate is empty this lap.
    RunState again = run.state();
    again.node = 3;
    run.resume(again);
    EXPECT_EQ(run.travel(0).what, Arrival::Nothing);
}

TEST(Run, ACacheWaitsIfTheBagIsFull)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 3, 2);
    s.bag[Item::Gel]   = 4;
    s.bag[Item::Water] = 2;   // six: full
    run.resume(s);
    const ArriveResult r = run.travel(0);
    EXPECT_TRUE(r.bagFull);
    EXPECT_EQ(run.state().cachesTaken, 0u);
}

TEST(Run, TheHeadtorchIsAKeyItemOutsideTheBag)
{
    TrailQuest::Run run;
    RunState s = standingAt(3, 3, 12);   // the ridge refuge: first exit, the rescue box
    s.bag[Item::Gel]   = 4;
    s.bag[Item::Water] = 2;   // the bag is full of consumables
    run.resume(s);
    const ArriveResult r = run.travel(0);
    EXPECT_EQ(r.item, Item::Headtorch);
    EXPECT_FALSE(r.bagFull);
    EXPECT_EQ(run.state().bag[Item::Headtorch], 1u);
}

TEST(Run, NightWithoutAHeadtorchIsDark)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 0, 1);
    s.minutes = 21 * 60;
    run.resume(s);
    EXPECT_TRUE(run.dark());
    s.bag[Item::Headtorch] = 1;
    run.resume(s);
    EXPECT_FALSE(run.dark());
    s.minutes = 12 * 60;
    s.bag[Item::Headtorch] = 0;
    run.resume(s);
    EXPECT_FALSE(run.dark());
}

TEST(Run, ALostDuelSendsYouBackToTheHut)
{
    // A level 1 runner against the Summit Wolf: certain to lose.
    TrailQuest::Run run;
    RunState s = standingAt(4, 6, 1);
    s.hutNode = 3;
    s.bag[Item::Gel]   = 3;
    s.bag[Item::Water] = 1;
    run.resume(s);
    ASSERT_EQ(run.travel(0).what, Arrival::Boss);
    const uint16_t minutes = run.state().minutes;
    fight(run, Action::Settle);
    ASSERT_EQ(run.lastDuel().outcome, DuelOutcome::Lost);
    EXPECT_EQ(run.phase(), RunPhase::AtNode);
    EXPECT_EQ(run.state().node, 3u);
    EXPECT_EQ(run.state().minutes, minutes + kLossMinutes);
    EXPECT_EQ(run.state().stamina, runnerStats(1).stamina);
    EXPECT_EQ(run.lastDuel().lostItem, Item::Gel);   // the most plentiful
    EXPECT_EQ(run.state().bag[Item::Gel], 2u);
    EXPECT_EQ(run.state().duelsLost, 1u);
    EXPECT_TRUE(run.wantsSave());
}

TEST(Run, ALossBeforeAnyHutReturnsToTheStageStart)
{
    TrailQuest::Run run;
    RunState s = standingAt(4, 0, 1);
    run.resume(s);
    ASSERT_EQ(run.travel(0).what, Arrival::Duel);
    fight(run, Action::Settle);
    ASSERT_EQ(run.lastDuel().outcome, DuelOutcome::Lost);
    EXPECT_EQ(run.state().node, 0u);
}

TEST(Run, AWonDuelGivesXpAndAnOfferToJoin)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 0, 20);   // far too strong for the valley
    run.resume(s);
    ASSERT_EQ(run.travel(0).what, Arrival::Duel);
    const SpeciesId sp = run.duel().setup().species;
    fight(run);
    ASSERT_EQ(run.lastDuel().outcome, DuelOutcome::Won);
    EXPECT_EQ(run.lastDuel().xp, 0u + xpForWin(kSpecies[sp], run.duel().spirit().level));
    EXPECT_TRUE(run.lastDuel().joinOffered);
    EXPECT_EQ(run.phase(), RunPhase::JoinOffer);
    EXPECT_EQ(run.offered(), sp);
    EXPECT_NE(run.state().spiritsBeaten & (1u << sp), 0u);
    EXPECT_TRUE(run.acceptJoin());
    EXPECT_EQ(run.state().partnerCount, 1u);
    EXPECT_EQ(run.state().partners[0], sp);
    EXPECT_EQ(run.phase(), RunPhase::AtNode);
}

TEST(Run, AFullPackMustNameWhoLeaves)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 0, 20);
    s.partners[0] = Spirit::WindSprite;
    s.partners[1] = Spirit::Raven;
    s.partners[2] = Spirit::SnowHare;
    s.partnerCount = 3;
    run.resume(s);
    ASSERT_EQ(run.travel(0).what, Arrival::Duel);
    const SpeciesId sp = run.duel().setup().species;
    fight(run);
    ASSERT_EQ(run.phase(), RunPhase::JoinOffer);
    EXPECT_FALSE(run.acceptJoin());       // no name given
    EXPECT_FALSE(run.acceptJoin(7));      // no such slot
    EXPECT_EQ(run.phase(), RunPhase::JoinOffer);
    EXPECT_TRUE(run.acceptJoin(1));
    EXPECT_EQ(run.state().partners[1], sp);
    EXPECT_EQ(run.state().partnerCount, 3u);
}

TEST(Run, DecliningKeepsThePackAsItWas)
{
    TrailQuest::Run run;
    run.resume(standingAt(0, 0, 20));
    run.travel(0);
    fight(run);
    ASSERT_EQ(run.phase(), RunPhase::JoinOffer);
    run.declineJoin();
    EXPECT_EQ(run.phase(), RunPhase::AtNode);
    EXPECT_EQ(run.state().partnerCount, 0u);
}

namespace
{
/// The Technique damage a duel set up like @p d would do with @p move.
uint16_t techniqueDamage(DuelSetup d, MoveId move)
{
    d.technique = move;
    Duel probe;
    probe.start(d);
    return probe.estimate(Action::Technique);
}
} // namespace

TEST(Run, APartnerLendsItsStat)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 0, 5);
    s.partners[0]  = Spirit::BrownBear;   // Grit
    s.partnerCount = 1;
    run.resume(s);
    EXPECT_EQ(runnerMaxWithPartners(run.state()).grit, runnerStats(5).grit + kPartnerBonus);
}

TEST(Run, ThePartnerBestPlacedForThisSpiritStepsUp)
{
    // R1 plays whichever partner does most against this spirit, or Trail
    // craft if none beats it: no lead to choose.
    for (uint32_t seed = 1; seed <= 40; ++seed) {
        TrailQuest::Run run;
        RunState s = standingAt(0, 0, 5, seed);
        s.partners[0]  = Spirit::BrownBear;      // a Grit move
        s.partners[1]  = Spirit::RedFox;    // a Pace move
        s.partners[2]  = Spirit::RubyBeetle;   // a Form move
        s.partnerCount = 3;
        run.resume(s);
        ASSERT_EQ(run.travel(0).what, Arrival::Duel);
        const DuelSetup& d = run.duel().setup();
        const uint16_t chosen = techniqueDamage(d, d.technique);
        EXPECT_GE(chosen, techniqueDamage(d, Move::TrailCraft)) << "seed " << seed;
        for (uint8_t i = 0; i < 3; ++i) {
            EXPECT_GE(chosen, techniqueDamage(d, signatureMove(s.partners[i]))) << "seed " << seed;
        }
    }
}

TEST(Run, WithNoPartnersTheTechniqueIsTrailCraft)
{
    TrailQuest::Run run;
    run.resume(standingAt(0, 0, 5));
    run.travel(0);
    EXPECT_EQ(run.duel().setup().technique, Move::TrailCraft);
}

TEST(Run, APartnerLendsAMoveOfItsOwnKind)
{
    // The Snow Hare is a Form spirit with a Form move and a stronger Pace one: it
    // lends the Form one. The Brown Bear lends the stronger of its two Grit moves.
    EXPECT_EQ(signatureMove(Spirit::SnowHare), Move::Chill);
    EXPECT_EQ(signatureMove(Spirit::BrownBear), Move::HitTheWall);
    for (SpeciesId sp = 0; sp < Spirit::Count; ++sp) {
        const SpeciesDef& d = kSpecies[sp];
        const bool hasOwn = moveDef(d.moves[0]).affinity == d.affinity || moveDef(d.moves[1]).affinity == d.affinity;
        if (hasOwn) {
            EXPECT_EQ(moveDef(signatureMove(sp)).affinity, d.affinity) << d.name;
        }
    }
}

TEST(Run, LevelsComeWithXpAndBringStamina)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 0, 1);
    s.level = 19;
    s.xp    = static_cast<uint16_t>(xpToNext(19) - 1);
    s.stamina = runnerStats(19).stamina - 10;
    run.resume(s);
    run.travel(0);
    fight(run);
    ASSERT_EQ(run.lastDuel().outcome, DuelOutcome::Won);
    EXPECT_EQ(run.state().level, 20u);
    EXPECT_EQ(run.lastDuel().levelsGained, 1u);
    EXPECT_EQ(run.state().xp, 0u);   // the top: no more to gain
}

TEST(Run, BeatingABossClearsTheStageAndTheNextBegins)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 6, 20);
    run.resume(s);
    ASSERT_EQ(run.travel(0).what, Arrival::Boss);
    EXPECT_EQ(run.duel().setup().species, Spirit::OldTortoise);
    fight(run);
    ASSERT_EQ(run.lastDuel().outcome, DuelOutcome::Won);
    EXPECT_TRUE(run.lastDuel().boss);
    EXPECT_FALSE(run.lastDuel().joinOffered);
    EXPECT_EQ(run.phase(), RunPhase::StageClear);
    EXPECT_TRUE(run.wantsSave());
    const uint16_t left = run.state().stamina;
    const uint16_t full = runnerStats(20).stamina;
    run.nextStage();
    EXPECT_EQ(run.state().stage, 1u);
    EXPECT_EQ(run.state().node, 0u);
    // On to the next stage: half of what was missing comes back (a new game or lap: all).
    EXPECT_EQ(run.state().stamina, left + (full - left) * kStageRestPct / 100u);
    EXPECT_EQ(run.phase(), RunPhase::AtNode);
}

TEST(Run, TheSummitFinishesTheLapAndTheNextIsHarder)
{
    // From the summit with every boss beaten (the flow, not the balance: the
    // fight itself is the balance simulator's business).
    TrailQuest::Run run;
    RunState s = standingAt(4, 8, 20);
    s.minutes      = 17 * 60;
    s.bossesBeaten = 0x0F;
    run.resume(s);
    ASSERT_EQ(run.phase(), RunPhase::AtNode);
    s.bossesBeaten = 0x1F;
    run.resume(s);
    ASSERT_EQ(run.phase(), RunPhase::Finished);
    run.nextStage();                          // not valid when finished: no change
    EXPECT_EQ(run.phase(), RunPhase::Finished);
    run.startNextLap();
    EXPECT_EQ(run.state().lap, 1u);
    EXPECT_EQ(run.state().stage, 0u);
    EXPECT_EQ(run.state().bossesBeaten, 0u);
    EXPECT_EQ(run.state().minutes % (24 * 60), kRunStartMinutes);   // next morning
    EXPECT_GT(run.state().minutes, 24u * 60u);
    run.travel(0);
    EXPECT_GE(run.duel().spirit().level, kStages[0].levelMin + kLapLevelStep);
}

TEST(Run, ResumingAtABeatenBossPicksUpTheStageClear)
{
    TrailQuest::Run run;
    RunState s = standingAt(1, 8, 10);
    s.bossesBeaten = 0x03;
    run.resume(s);
    EXPECT_EQ(run.phase(), RunPhase::StageClear);
    RunState top = standingAt(4, 8, 20);
    top.bossesBeaten = 0x1F;
    run.resume(top);
    EXPECT_EQ(run.phase(), RunPhase::Finished);
}

TEST(Run, EveryStepThatEndsOnTheMapAsksToSave)
{
    TrailQuest::Run run;
    run.resume(standingAt(0, 0, 3));
    EXPECT_FALSE(run.wantsSave());   // coming back is not a step

    run.travel(1);   // the stepping stones
    ASSERT_EQ(run.phase(), RunPhase::Obstacle);
    EXPECT_FALSE(run.wantsSave());   // never in the middle of an obstacle
    run.attemptObstacle(BeatResult::Good);
    EXPECT_EQ(run.phase(), RunPhase::AtNode);
    EXPECT_TRUE(run.wantsSave());
    run.saved();

    run.travel(1);   // the long meadow: a spirit
    ASSERT_EQ(run.phase(), RunPhase::InDuel);
    EXPECT_FALSE(run.wantsSave());   // nor of a duel
    run.duelTurn(PlayerTurn{});
    if (run.phase() == RunPhase::InDuel) {
        EXPECT_FALSE(run.wantsSave());
    }
    fight(run);
    EXPECT_TRUE(run.wantsSave());    // won (an offer) or lost (the hut): either way
    run.saved();

    if (run.phase() == RunPhase::JoinOffer) {
        run.declineJoin();
        EXPECT_TRUE(run.wantsSave());
        run.saved();
    }
    // An arrival with nothing to resolve saves too (here, the old gate's cache).
    TrailQuest::Run hut;
    hut.resume(standingAt(0, 3, 3));
    hut.travel(0);   // to the old gate (a cache)
    EXPECT_EQ(hut.phase(), RunPhase::AtNode);
    EXPECT_TRUE(hut.wantsSave());
}

TEST(Run, QuittingAtAnOfferComesBackToTheOffer)
{
    TrailQuest::Run run;
    run.resume(standingAt(0, 0, 20));
    run.travel(0);   // the birch wood: a spirit
    fight(run);
    ASSERT_EQ(run.phase(), RunPhase::JoinOffer);
    const RunState saved = run.state();
    EXPECT_NE(saved.offer, kNone);
    EXPECT_EQ(saved.offer, run.offered());

    TrailQuest::Run back;
    back.resume(saved);
    EXPECT_EQ(back.phase(), RunPhase::JoinOffer);
    EXPECT_EQ(back.offered(), saved.offer);
    EXPECT_TRUE(back.acceptJoin());
    EXPECT_EQ(back.state().offer, kNone);
    EXPECT_EQ(back.state().partnerCount, 1u);
    EXPECT_EQ(back.state().partners[0], saved.offer);
}

TEST(Run, AStirredResumeMeetsAFreshSpiritAndNoStirReplays)
{
    const RunState s = standingAt(0, 0, 3, 1234);
    TrailQuest::Run a, b;
    a.resume(s);
    b.resume(s);
    a.travel(0);
    b.travel(0);
    EXPECT_EQ(a.duel().setup().species, b.duel().setup().species);
    EXPECT_EQ(a.duel().setup().speciesLevel, b.duel().setup().speciesLevel);
    EXPECT_EQ(a.state().rng, b.state().rng);

    // Coming back after quitting mid-duel: the service stirs with its clock,
    // so the same stretch does not hold the same spirit every time.
    std::set<std::pair<SpeciesId, uint8_t>> met;
    for (uint32_t stir = 1; stir <= 40; ++stir) {
        TrailQuest::Run r;
        r.resume(s, stir * 7919u);
        EXPECT_NE(r.state().rng, s.rng);
        EXPECT_NE(r.state().rng, 0u);
        r.travel(0);
        met.insert({ r.duel().setup().species, r.duel().setup().speciesLevel });
    }
    EXPECT_GT(met.size(), 3u);
}

TEST(Run, PerfectBeatsAreCounted)
{
    TrailQuest::Run run;
    run.resume(standingAt(0, 0, 20));
    run.travel(0);
    PlayerTurn t;
    t.beat = BeatResult::Perfect;
    run.duelTurn(t);
    EXPECT_EQ(run.state().perfects, 1u);
}

TEST(Run, TheSkillListIsTrailCraftThenEachPartnersMove)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 0, 5);
    s.partners[0]  = Spirit::BrownBear;
    s.partners[1]  = Spirit::RedFox;
    s.partners[2]  = Spirit::RubyBeetle;
    s.partnerCount = 3;
    run.resume(s);
    ASSERT_EQ(run.travel(0).what, Arrival::Duel);
    const DuelSetup& d = run.duel().setup();
    ASSERT_EQ(d.skillCount, 4u);
    EXPECT_EQ(d.skills[0], Move::TrailCraft);
    for (uint8_t i = 0; i < 3; ++i) {
        EXPECT_EQ(d.skills[i + 1u], signatureMove(s.partners[i]));
    }

    TrailQuest::Run alone;
    alone.resume(standingAt(0, 0, 5));
    ASSERT_EQ(alone.travel(0).what, Arrival::Duel);
    EXPECT_EQ(alone.duel().setup().skillCount, 1u);
    EXPECT_EQ(alone.duel().setup().skills[0], Move::TrailCraft);
}

TEST(Run, FatigueCarriesOnAndOnlyFuelOrTheHutGivesItBack)
{
    // A stretch of trail is a breather and no more.
    TrailQuest::Run run;
    RunState s = standingAt(1, 0, 10);
    s.stamina  = 30;
    run.resume(s);
    const uint16_t max = runnerMaxWithPartners(run.state()).stamina;
    run.travel(0);   // the first bend (a duel): the walk gave back its small share
    EXPECT_EQ(run.duel().setup().runnerStamina, 30u + max * kTravelRecoverPct / 100u);
    EXPECT_LT(run.duel().setup().runnerStamina, max / 2u);

    // Nor is there a refill at the foot of a boss.
    TrailQuest::Run boss;
    RunState b = standingAt(0, 6, 5);
    b.stamina  = 20;
    boss.resume(b);
    ASSERT_EQ(boss.travel(0).what, Arrival::Boss);
    EXPECT_EQ(boss.duel().setup().runnerStamina, 20u + runnerMaxWithPartners(boss.state()).stamina * kTravelRecoverPct / 100u);

    // The hut fills it.
    TrailQuest::Run hut;
    RunState h = standingAt(0, 2, 5);
    h.stamina  = 10;
    hut.resume(h);
    ASSERT_EQ(hut.travel(0).what, Arrival::Hut);
    EXPECT_EQ(hut.state().stamina, runnerMaxWithPartners(hut.state()).stamina);
}

TEST(Run, TheHutIsADetourNotOnTheOnlyWay)
{
    // In every stage there is a way from the start to the boss that never stops at the hut,
    // and the hut can be reached from a fork on the way (a detour that costs trail time).
    for (const StageDef& st : kStages) {
        std::set<uint8_t> reach = { 0 };
        for (uint8_t n = 0; n < st.nodeCount; ++n) {
            if (st.nodes[n].kind == NodeKind::Hut) {
                continue;
            }
            if (reach.count(n) == 0) {
                continue;
            }
            for (uint8_t e = 0; e < st.nodes[n].exitCount; ++e) {
                if (st.nodes[st.nodes[n].exits[e].to].kind != NodeKind::Hut) {
                    reach.insert(st.nodes[n].exits[e].to);
                }
            }
        }
        bool bossReached = false;
        for (uint8_t n : reach) {
            bossReached = bossReached || st.nodes[n].kind == NodeKind::Boss;
        }
        EXPECT_TRUE(bossReached) << st.name << ": the boss is reachable without the hut";
        // A fork where the hut is one of two ways.
        bool fork = false;
        for (uint8_t n = 0; n < st.nodeCount; ++n) {
            bool hut = false, other = false;
            for (uint8_t e = 0; e < st.nodes[n].exitCount; ++e) {
                (st.nodes[st.nodes[n].exits[e].to].kind == NodeKind::Hut ? hut : other) = true;
            }
            fork = fork || (hut && other);
        }
        EXPECT_TRUE(fork) << st.name;
    }
}

TEST(Run, ALossBeforeAnyHutSendsTheRunnerBackToTheStagesStart)
{
    TrailQuest::Run run;
    RunState s = standingAt(0, 6, 1);
    s.stamina  = 1;
    s.hutNode  = 0;   // never reached the hut
    run.resume(s);
    ASSERT_EQ(run.travel(0).what, Arrival::Boss);
    for (int i = 0; i < 40 && run.phase() == RunPhase::InDuel; ++i) {
        PlayerTurn t;
        t.action = Action::Settle;
        t.beat   = BeatResult::Miss;
        run.duelTurn(t);
    }
    ASSERT_EQ(run.phase(), RunPhase::AtNode);
    EXPECT_EQ(run.state().node, 0u);
}
