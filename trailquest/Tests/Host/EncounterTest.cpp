// Which spirit, at what level, what is found, and an obstacle's odds.

#include <gtest/gtest.h>

#include "Encounter.hpp"
#include "Progress.hpp"

using namespace TrailQuest;

TEST(Encounter, NightSpiritsStayHiddenInDaylight)
{
    const StageDef& climb = kStages[1];   // has the Barn Owl, a night spirit
    Rng r(3);
    int owlsByDay = 0, owlsByNight = 0;
    for (int i = 0; i < 4000; ++i) {
        const SpeciesId day = pickSpecies(climb, TimeOfDay::Day, r);
        const SpeciesId night = pickSpecies(climb, TimeOfDay::Night, r);
        owlsByDay   += day == Spirit::BarnOwl ? 1 : 0;
        owlsByNight += night == Spirit::BarnOwl ? 1 : 0;
        bool inPool = false;
        for (uint8_t p = 0; p < climb.poolCount; ++p) {
            inPool = inPool || climb.pool[p] == day;
        }
        ASSERT_TRUE(inPool);
    }
    EXPECT_EQ(owlsByDay, 0);
    EXPECT_GT(owlsByNight, 800);   // about a quarter of 4000
}

TEST(Encounter, APoolOfOnlyNightSpiritsStillGivesOneByDay)
{
    StageDef s = kStages[1];
    s.pool[0]   = Spirit::BarnOwl;
    s.poolCount = 1;
    Rng r(1);
    EXPECT_EQ(pickSpecies(s, TimeOfDay::Day, r), Spirit::BarnOwl);
}

TEST(Encounter, LevelsStayInTheBandAndRiseWithTheLap)
{
    for (const StageDef& s : kStages) {
        Rng r(7);
        for (int i = 0; i < 500; ++i) {
            const uint8_t l = pickLevel(s, 0, r);
            ASSERT_GE(l, s.levelMin) << s.name;
            ASSERT_LE(l, s.levelMax) << s.name;
            const uint8_t l1 = pickLevel(s, 1, r);
            ASSERT_GE(l1, s.levelMin + kLapLevelStep > kMaxLevel ? kMaxLevel : s.levelMin + kLapLevelStep) << s.name;
            ASSERT_LE(l1, kMaxLevel);
        }
        EXPECT_EQ(bossLevel(s, 0), s.levelMax + kBossLevelAbove) << s.name;
        EXPECT_LE(bossLevel(s, 9), kMaxLevel) << s.name;
    }
}

TEST(Encounter, ObstacleOddsWorkedByHand)
{
    const ObstacleDef& stones = kObstacles[Obstacle::SteppingStones];   // Form, 11
    EXPECT_EQ(obstacleChance(stones, 11, BeatResult::Good), 60u);       // 50 + 0 + 10
    EXPECT_EQ(obstacleChance(stones, 11, BeatResult::Perfect), 75u);
    EXPECT_EQ(obstacleChance(stones, 11, BeatResult::Miss), 40u);
    EXPECT_EQ(obstacleChance(stones, 13, BeatResult::Good), 70u);       // + 5 a point
    EXPECT_EQ(obstacleChance(stones, 40, BeatResult::Perfect), 95u);    // held
    EXPECT_EQ(obstacleChance(stones, 1, BeatResult::Miss), 10u);
}

TEST(Encounter, FindsAreMostlyGels)
{
    Rng r(5);
    int counts[Item::Count] = {};
    for (int i = 0; i < 10000; ++i) {
        ++counts[pickFind(r)];
    }
    EXPECT_NEAR(counts[Item::Gel], 5000, 250);
    EXPECT_NEAR(counts[Item::Water], 3000, 250);
    EXPECT_NEAR(counts[Item::SaltTab], 2000, 250);
    EXPECT_EQ(counts[Item::Headtorch], 0);
    EXPECT_EQ(counts[Item::Poles], 0);
}
