// The session the service runs: commands in, views out, refusals that change
// nothing, saves when the game asks, and stage 1 played start to finish by
// commands alone.

#include <gtest/gtest.h>

#include "SaveCodec.hpp"
#include "Session.hpp"

using namespace TrailQuest;

namespace
{

/// Answer whatever the game is waiting for, the way a player of stage 1 would.
/// Returns false when there is nothing left this build allows.
bool step(Session& s, uint32_t i)
{
    switch (s.phase()) {
        case RunPhase::AtNode: {
            const NodeDef& here = nodeDef(s.state().stage, s.state().node);
            return s.travel(static_cast<uint8_t>(i % here.exitCount));
        }
        case RunPhase::Obstacle:   return s.obstacle(BeatResult::Good);
        case RunPhase::InDuel: {
            PlayerTurn t;
            t.action = i % 3 == 0 ? Action::Technique : Action::Surge;
            t.beat   = i % 4 == 0 ? BeatResult::Perfect : BeatResult::Good;
            return s.duelTurn(t);
        }
        case RunPhase::JoinOffer:  return s.join(true, static_cast<uint8_t>(i % kMaxPartners));
        default:                   return false;
    }
}

} // namespace

TEST(Session, WithNoSaveThereIsNoGameUntilANewOne)
{
    Session s;
    s.begin(nullptr, 0);
    EXPECT_FALSE(s.hasGame());
    EXPECT_EQ(s.world().hasGame, 0u);
    EXPECT_EQ(s.world().cause, ViewCause::None);
    EXPECT_FALSE(s.travel(0));   // nothing to travel in
    EXPECT_EQ(s.world().cause, ViewCause::Refused);
    EXPECT_TRUE(s.newGame(5));
    EXPECT_TRUE(s.hasGame());
    const WorldView w = s.world();
    EXPECT_EQ(w.cause, ViewCause::NewGame);
    EXPECT_EQ(w.phase, RunPhase::AtNode);
    EXPECT_EQ(w.state.node, 0u);
    EXPECT_EQ(w.playableStages, Session::kPlayableStages);
    EXPECT_TRUE(s.wantsSave());
}

TEST(Session, ASavedGameIsPickedUpWhereItWasLeft)
{
    Session a;
    a.begin(nullptr, 0);
    a.newGame(11);
    for (uint32_t i = 0; i < 6; ++i) {
        step(a, i);
    }
    // Save the last state the game asked to save, as the service does.
    Session b;
    const RunState saved = a.state();
    b.begin(&saved, 0);
    EXPECT_TRUE(b.hasGame());
    EXPECT_EQ(b.world().cause, ViewCause::Loaded);
    EXPECT_EQ(b.state().node, saved.node);
    EXPECT_FALSE(b.wantsSave());
}

TEST(Session, ACommandThatDoesNotFitTheMomentChangesNothing)
{
    Session s;
    s.begin(nullptr, 0);
    s.newGame(3);
    s.saved();
    const RunState before = s.state();
    EXPECT_FALSE(s.obstacle(BeatResult::Good));           // not at an obstacle
    EXPECT_FALSE(s.duelTurn(PlayerTurn{}));                // not in a duel
    EXPECT_FALSE(s.join(true));                            // no offer
    EXPECT_FALSE(s.advance());                             // the stage is not cleared
    EXPECT_FALSE(s.travel(2));                             // the trailhead has two exits
    EXPECT_FALSE(s.travel(kNone));
    EXPECT_EQ(s.world().cause, ViewCause::Refused);
    EXPECT_EQ(s.state().rng, before.rng);
    EXPECT_EQ(s.state().node, before.node);
    EXPECT_EQ(s.state().minutes, before.minutes);
    EXPECT_FALSE(s.wantsSave());
}

TEST(Session, ATurnAsksOnlyForWhatIsInTheBag)
{
    Session s;
    s.begin(nullptr, 0);
    s.newGame(3);
    ASSERT_TRUE(s.travel(0));   // the birch wood: a spirit
    ASSERT_EQ(s.phase(), RunPhase::InDuel);
    ASSERT_TRUE(s.showsDuel());
    EXPECT_EQ(s.duel().log.count, 0u);   // nothing played yet
    const RunState before = s.state();

    PlayerTurn t;
    t.action = Action::Fuel;
    t.item   = Item::SaltTab;   // a new game has none
    EXPECT_FALSE(s.duelTurn(t));
    t.item = Item::Headtorch;   // a key item, never used up
    EXPECT_FALSE(s.duelTurn(t));
    t.item = 200;
    EXPECT_FALSE(s.duelTurn(t));
    t.action = static_cast<Action>(9);
    EXPECT_FALSE(s.duelTurn(t));
    PlayerTurn badBeat;
    badBeat.beat = static_cast<BeatResult>(7);
    EXPECT_FALSE(s.duelTurn(badBeat));
    EXPECT_EQ(s.state().rng, before.rng);
    EXPECT_EQ(s.duel().turn, 0u);

    t.action = Action::Fuel;
    t.item   = Item::Gel;
    ASSERT_TRUE(s.duelTurn(t));
    EXPECT_EQ(s.state().bag[Item::Gel], before.bag[Item::Gel] - 1);
    const DuelView d = s.duel();
    EXPECT_GE(d.log.count, 1u);
    EXPECT_EQ(d.log.events[0].kind, EventKind::UsedItem);
}

TEST(Session, TheDuelViewShowsBothSidesAndWhatR1Plays)
{
    Session s;
    s.begin(nullptr, 0);
    s.newGame(3);
    ASSERT_TRUE(s.travel(0));
    const DuelView d = s.duel();
    EXPECT_LT(d.species, Spirit::Count);
    EXPECT_FALSE(isBoss(d.species));
    EXPECT_EQ(d.technique, Move::TrailCraft);   // no partners yet
    EXPECT_EQ(d.runner.max, 40u);
    EXPECT_EQ(d.runner.stamina, s.state().stamina);
    EXPECT_GT(d.spirit.max, 0u);
    EXPECT_EQ(d.spirit.stamina, d.spirit.max);
    EXPECT_GT(d.estimate[0], 0u);
    EXPECT_GT(d.chanceRunner, 0u);
    EXPECT_EQ(d.outcome, DuelOutcome::Ongoing);
    EXPECT_EQ(s.world().arrive.what, Arrival::Duel);
    EXPECT_EQ(s.world().fromNode, 0u);
}

TEST(Session, StageOneCanBePlayedToTheEndByCommands)
{
    // Many games, start to the stage clear, every command through the session,
    // every save it asks for valid.
    for (uint32_t seed = 1; seed <= 40; ++seed) {
        Session s;
        s.begin(nullptr, 0);
        s.newGame(seed);
        uint32_t i = 0;
        for (; i < 600 && s.phase() != RunPhase::StageClear; ++i) {
            if (s.wantsSave()) {
                EXPECT_TRUE(SaveCodec::valid(s.state())) << seed;
                s.saved();
            }
            ASSERT_TRUE(step(s, i + seed)) << "seed " << seed << " stuck in phase " << int(s.phase());
            const WorldView w = s.world();
            EXPECT_NE(w.cause, ViewCause::Refused);
            if (s.showsDuel()) {
                EXPECT_TRUE(w.phase == RunPhase::InDuel || w.cause == ViewCause::DuelOver);
            }
        }
        ASSERT_EQ(s.phase(), RunPhase::StageClear) << seed;
        EXPECT_TRUE(s.wantsSave());
        EXPECT_EQ(s.world().cause, ViewCause::DuelOver);
        EXPECT_TRUE(s.world().duel.boss);
        // On to the Switchback Climb: its first place, with its own weather.
        ASSERT_TRUE(s.advance());
        EXPECT_EQ(s.world().cause, ViewCause::StageStarted);
        EXPECT_EQ(s.state().stage, 1u);
        EXPECT_EQ(s.state().node, 0u);
        EXPECT_EQ(s.phase(), RunPhase::AtNode);
    }
}

TEST(Session, AllFiveStagesCanBePlayedToTheSummitAndOnToASecondLap)
{
    // A steady player: the skill when it is a partner's move, Settle when low.
    for (uint32_t seed = 1; seed <= 6; ++seed) {
        Session s;
        s.begin(nullptr, 0);
        ASSERT_TRUE(s.newGame(seed * 7919u));
        uint32_t i = 0;
        for (uint8_t stage = 0; stage < Session::kPlayableStages; ++stage) {
            for (; i < 12000 && s.phase() != RunPhase::StageClear; ++i) {
                if (s.phase() == RunPhase::InDuel) {
                    PlayerTurn t;
                    t.action = s.state().stamina * 100u < 40u * runnerMaxWithPartners(s.state()).stamina
                                   ? Action::Settle
                                   : (i % 2 == 0 ? Action::Technique : Action::Surge);
                    t.beat = BeatResult::Good;
                    ASSERT_TRUE(s.duelTurn(t));
                } else {
                    ASSERT_TRUE(step(s, i + seed)) << "seed " << seed << " stage " << int(stage) << " stuck";
                }
            }
            ASSERT_EQ(s.phase(), RunPhase::StageClear) << "seed " << seed << " stage " << int(stage);
            EXPECT_EQ(s.state().stage, stage);
            ASSERT_TRUE(s.advance());
            if (stage + 1u < Session::kPlayableStages) {
                EXPECT_EQ(s.state().stage, stage + 1u);
            }
        }
        // The summit's clear card leads to the finale; "Run again" starts lap 2.
        EXPECT_EQ(s.phase(), RunPhase::Finished);
        EXPECT_EQ(s.state().lap, 0u);
        ASSERT_TRUE(s.advance());
        EXPECT_EQ(s.state().lap, 1u);
        EXPECT_EQ(s.state().stage, 0u);
        EXPECT_EQ(s.phase(), RunPhase::AtNode);
        EXPECT_EQ(s.world().cause, ViewCause::StageStarted);
        EXPECT_LT(beatFor(0, 0).sweepMs, 1600u + 1u);
        EXPECT_LT(beatFor(0, 1).sweepMs, beatFor(0, 0).sweepMs);
    }
}

TEST(Session, ALostDuelEndsBackAtTheHutAndSays)
{
    // A level-1 runner with no fuel against the Old Tortoise, playing badly, loses.
    RunState st;
    st.rng     = 77;
    st.level   = 1;
    st.stamina = 40;
    st.stage   = 0;
    st.node    = 6;   // the foot of the hill, by the straight way to the boss
    st.hutNode = 3;
    st.minutes = 9 * 60;
    Session s;
    s.begin(&st, 0);
    ASSERT_TRUE(s.travel(0));
    ASSERT_EQ(s.phase(), RunPhase::InDuel);
    ASSERT_TRUE(isBoss(s.duel().species));
    for (int i = 0; i < 40 && s.phase() == RunPhase::InDuel; ++i) {
        PlayerTurn t;
        t.action = Action::Settle;
        t.beat   = BeatResult::Miss;
        s.duelTurn(t);
    }
    ASSERT_EQ(s.phase(), RunPhase::AtNode);
    const WorldView w = s.world();
    EXPECT_EQ(w.cause, ViewCause::DuelOver);
    EXPECT_EQ(w.duel.outcome, DuelOutcome::Lost);
    EXPECT_EQ(w.state.node, 3u);
    EXPECT_TRUE(s.showsDuel());   // the last turn still plays back
    EXPECT_EQ(s.duel().outcome, DuelOutcome::Lost);
    // The next command is about the map again.
    ASSERT_TRUE(s.travel(0));
    EXPECT_EQ(s.world().cause, ViewCause::Arrived);
}

TEST(Session, AnOfferIsAnsweredOnceAndAFullPackNamesWhoLeaves)
{
    RunState st;
    st.rng          = 5;
    st.level        = 20;
    st.stamina      = runnerStats(20).stamina;
    st.partnerCount = 3;
    st.partners[0]  = Spirit::FireNewt;
    st.partners[1]  = Spirit::GnatSwarm;
    st.partners[2]  = Spirit::BarnOwl;
    st.minutes      = 9 * 60;
    Session s;
    s.begin(&st, 0);
    ASSERT_TRUE(s.travel(0));   // the birch wood
    for (int i = 0; i < 40 && s.phase() == RunPhase::InDuel; ++i) {
        s.duelTurn(PlayerTurn{});
    }
    ASSERT_EQ(s.phase(), RunPhase::JoinOffer);
    const SpeciesId offered = s.state().offer;
    EXPECT_FALSE(s.join(true));        // a full pack must say who leaves
    EXPECT_EQ(s.phase(), RunPhase::JoinOffer);
    EXPECT_TRUE(s.join(true, 1));
    EXPECT_EQ(s.world().cause, ViewCause::Joined);
    EXPECT_EQ(s.state().partners[1], offered);
    EXPECT_EQ(s.state().offer, kNone);
    EXPECT_FALSE(s.join(false));       // already answered
}

TEST(Session, TheDuelViewListsTheSkillsAndATurnCanPickOne)
{
    // A runner with a three-creature pack meets a spirit: R1's choices are in the view.
    RunState st;
    st.rng          = 123;
    st.level        = 6;
    st.stamina      = runnerStats(6).stamina + 3;
    st.partners[0]  = Spirit::BrownBear;
    st.partners[1]  = Spirit::RedFox;
    st.partners[2]  = Spirit::RubyBeetle;
    st.partnerCount = 3;
    st.stage        = 0;
    st.node         = 0;
    st.hutNode      = 0;
    st.minutes      = 9 * 60;
    st.bag[Item::Gel] = 2;
    Session s;
    s.begin(&st, 0);
    ASSERT_TRUE(s.travel(0));
    ASSERT_EQ(s.phase(), RunPhase::InDuel);
    DuelView v = s.duel();
    ASSERT_EQ(v.skillCount, 4u);
    EXPECT_EQ(v.skill[0], Move::TrailCraft);
    EXPECT_EQ(v.skill[1], signatureMove(Spirit::BrownBear));
    EXPECT_EQ(v.skill[3], signatureMove(Spirit::RubyBeetle));
    ASSERT_LT(v.skillBest, v.skillCount);
    EXPECT_EQ(v.skill[v.skillBest], v.technique);
    for (uint8_t i = 0; i < v.skillCount; ++i) {
        EXPECT_GT(v.skillDmg[i], 0u) << int(i);
    }
    PlayerTurn t;
    t.action = Action::Technique;
    t.skill  = 3;
    ASSERT_TRUE(s.duelTurn(t));
    bool beetle = false;
    for (uint8_t i = 0; i < s.duel().log.count; ++i) {
        const DuelEvent& e = s.duel().log.events[i];
        if (e.actor == Side::Runner && (e.kind == EventKind::Hit || e.kind == EventKind::Missed)) {
            EXPECT_EQ(e.move, signatureMove(Spirit::RubyBeetle));
            beetle = true;
        }
    }
    EXPECT_TRUE(beetle);
}
