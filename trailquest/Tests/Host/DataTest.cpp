// The game's tables: every move, spirit and item is complete and in range, and
// the levels and XP behave. If a table is edited into nonsense, these fail
// before anything reaches a watch.

#include <gtest/gtest.h>

#include <cstring>

#include "Progress.hpp"
#include "data/Items.hpp"
#include "data/Moves.hpp"
#include "data/Species.hpp"
#include "data/Stages.hpp"

using namespace TrailQuest;

TEST(Data, TheWheelGoesRound)
{
    EXPECT_EQ(wheel(Affinity::Pace, Affinity::Form), 1);
    EXPECT_EQ(wheel(Affinity::Form, Affinity::Grit), 1);
    EXPECT_EQ(wheel(Affinity::Grit, Affinity::Pace), 1);
    EXPECT_EQ(wheel(Affinity::Form, Affinity::Pace), -1);
    EXPECT_EQ(wheel(Affinity::Grit, Affinity::Form), -1);
    EXPECT_EQ(wheel(Affinity::Pace, Affinity::Grit), -1);
    for (uint8_t a = 0; a < kAffinityCount; ++a) {
        EXPECT_EQ(wheel(static_cast<Affinity>(a), static_cast<Affinity>(a)), 0);
    }
    EXPECT_EQ(wheel(Affinity::Count, Affinity::Pace), 0);
}

TEST(Data, EveryMoveIsComplete)
{
    for (MoveId m = 0; m < Move::Count; ++m) {
        const MoveDef& d = kMoves[m];
        ASSERT_NE(d.name, nullptr) << int(m);
        EXPECT_GT(std::strlen(d.name), 0u) << int(m);
        EXPECT_LT(static_cast<uint8_t>(d.affinity), kAffinityCount) << d.name;
        EXPECT_LT(static_cast<uint8_t>(d.effect), static_cast<uint8_t>(Effect::Count)) << d.name;
        EXPECT_LE(d.effectChance, 100u) << d.name;
        EXPECT_EQ(d.effect == Effect::None, d.effectChance == 0) << d.name;
        EXPECT_GT(d.power, 0u) << d.name;
    }
    EXPECT_STREQ(moveDef(200).name, "Surge");
}

TEST(Data, MoveNamesAreUnique)
{
    for (MoveId a = 0; a < Move::Count; ++a) {
        for (MoveId b = a + 1; b < Move::Count; ++b) {
            EXPECT_STRNE(kMoves[a].name, kMoves[b].name);
        }
    }
}

TEST(Data, EverySpiritHasTwoRealMovesThatAreNotTheRunners)
{
    for (SpeciesId s = 0; s < Spirit::Count; ++s) {
        const SpeciesDef& d = kSpecies[s];
        ASSERT_NE(d.name, nullptr);
        EXPECT_NE(d.kind, nullptr) << d.name;
        EXPECT_NE(d.pain, nullptr) << d.name;
        EXPECT_GE(d.stage, 1u) << d.name;
        EXPECT_LE(d.stage, kStageCount) << d.name;
        EXPECT_LT(static_cast<uint8_t>(d.affinity), kAffinityCount) << d.name;
        EXPECT_GT(d.staminaPct, 0u) << d.name;
        EXPECT_LE(d.firstMoveWeight, 100u) << d.name;
        for (MoveId m : d.moves) {
            EXPECT_LT(m, Move::Count) << d.name;
            EXPECT_GT(m, Move::TrailCraft) << d.name << ": uses a runner's move";
        }
        EXPECT_NE(d.moves[0], d.moves[1]) << d.name;
    }
}

TEST(Data, EachStageHasExactlyOneBossAndItIsTheStagesBoss)
{
    for (uint8_t st = 0; st < kStageCount; ++st) {
        int bosses = 0;
        for (SpeciesId s = 0; s < Spirit::Count; ++s) {
            if (isBoss(s) && kSpecies[s].stage == st + 1) {
                ++bosses;
                EXPECT_EQ(kStages[st].boss, s) << kStages[st].name;
            }
        }
        EXPECT_EQ(bosses, 1) << kStages[st].name;
    }
}

TEST(Data, SpiritNamesAreUnique)
{
    for (SpeciesId a = 0; a < Spirit::Count; ++a) {
        for (SpeciesId b = a + 1; b < Spirit::Count; ++b) {
            EXPECT_STRNE(kSpecies[a].name, kSpecies[b].name);
        }
    }
}

TEST(Data, EveryItemIsCompleteAndOnlyTheHeadtorchIsKey)
{
    for (ItemId i = 0; i < Item::Count; ++i) {
        ASSERT_NE(kItems[i].name, nullptr);
        EXPECT_EQ(kItems[i].key, i == Item::Headtorch) << kItems[i].name;
        if (!kItems[i].key) {
            EXPECT_TRUE(kItems[i].heal > 0 || kItems[i].formBoost > 0 || kItems[i].clears != Effect::None)
                << kItems[i].name << " does nothing";
        }
    }
}

TEST(Data, RunnerStatsGrowWithLevelAndAreHeldToTheRange)
{
    const Stats l1 = runnerStats(1);
    EXPECT_EQ(l1.stamina, 40u);
    EXPECT_EQ(l1.pace, 10u);
    const Stats l20 = runnerStats(20);
    EXPECT_EQ(l20.stamina, 40u + 4u * 19u);
    EXPECT_EQ(l20.form, 29u);
    EXPECT_EQ(runnerStats(0).stamina, l1.stamina);
    EXPECT_EQ(runnerStats(99).stamina, l20.stamina);
}

TEST(Data, SpiritStatsFollowTheRunnersAtTheirLevel)
{
    const SpeciesDef& wolf = kSpecies[Spirit::SummitWolf];
    const Stats s = speciesStats(wolf, 14);
    const Stats r = runnerStats(14);
    EXPECT_EQ(s.stamina, r.stamina * wolf.staminaPct / 100u);
    EXPECT_EQ(s.pace, r.pace + wolf.pace);
    // No stat drops below 1, whatever the offset.
    SpeciesDef weak = wolf;
    weak.pace = -100;
    EXPECT_EQ(speciesStats(weak, 1).pace, 1u);
}

TEST(Data, TheXpCurveRisesAndStopsAtTheTop)
{
    for (uint8_t l = kMinLevel; l < kMaxLevel; ++l) {
        EXPECT_GT(xpToNext(l), 0u) << int(l);
        EXPECT_GE(xpToNext(l + 1u == kMaxLevel ? l : static_cast<uint8_t>(l + 1u)), xpToNext(l)) << int(l);
    }
    EXPECT_EQ(xpToNext(kMaxLevel), 0u);
    EXPECT_GT(xpForWin(kSpecies[Spirit::OldTortoise], 3), xpForWin(kSpecies[Spirit::RedFox], 3));
}
