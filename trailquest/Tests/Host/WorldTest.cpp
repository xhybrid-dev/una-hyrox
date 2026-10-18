// The Ridge's five stage graphs: well formed, loop-free, and every route ends
// at the boss.

#include <gtest/gtest.h>

#include "data/Stages.hpp"

using namespace TrailQuest;

namespace
{
/// Nodes reachable from @p from (bit i set for node i).
uint32_t reachable(const StageDef& s, uint8_t from)
{
    uint32_t seen = 1u << from;
    // Exits only go forward, so one pass in index order finds everything.
    for (uint8_t n = from; n < s.nodeCount; ++n) {
        if ((seen & (1u << n)) == 0) {
            continue;
        }
        for (uint8_t e = 0; e < s.nodes[n].exitCount; ++e) {
            seen |= 1u << s.nodes[n].exits[e].to;
        }
    }
    return seen;
}
} // namespace

TEST(World, StagesAreWithinTheirLimits)
{
    for (const StageDef& s : kStages) {
        EXPECT_GE(s.nodeCount, 6u) << s.name;
        EXPECT_LE(s.nodeCount, kMaxNodes) << s.name;
        EXPECT_GE(s.poolCount, 2u) << s.name;
        EXPECT_LE(s.poolCount, kMaxPool) << s.name;
        EXPECT_LE(s.levelMin, s.levelMax) << s.name;
        EXPECT_GE(s.parLevel, s.levelMin) << s.name;
        EXPECT_LE(s.parLevel, s.levelMax) << s.name;
        EXPECT_LT(static_cast<uint8_t>(s.terrain), static_cast<uint8_t>(Terrain::Count)) << s.name;
        for (Weather w : s.weather) {
            EXPECT_LT(static_cast<uint8_t>(w), static_cast<uint8_t>(Weather::Count)) << s.name;
        }
    }
}

TEST(World, StartFirstBossLastAndNowhereElse)
{
    for (const StageDef& s : kStages) {
        EXPECT_EQ(s.nodes[0].kind, NodeKind::Start) << s.name;
        EXPECT_EQ(s.nodes[s.nodeCount - 1].kind, NodeKind::Boss) << s.name;
        for (uint8_t n = 1; n + 1 < s.nodeCount; ++n) {
            EXPECT_NE(s.nodes[n].kind, NodeKind::Start) << s.name << " node " << int(n);
            EXPECT_NE(s.nodes[n].kind, NodeKind::Boss) << s.name << " node " << int(n);
        }
    }
}

TEST(World, ExitsGoForwardSoThereAreNoLoopsAndNoDeadEnds)
{
    for (const StageDef& s : kStages) {
        for (uint8_t n = 0; n < s.nodeCount; ++n) {
            const NodeDef& node = s.nodes[n];
            ASSERT_NE(node.name, nullptr) << s.name;
            EXPECT_LE(node.exitCount, kMaxExits) << node.name;
            if (node.kind == NodeKind::Boss) {
                EXPECT_EQ(node.exitCount, 0u) << node.name;
                continue;
            }
            EXPECT_GE(node.exitCount, 1u) << node.name << " is a dead end";
            for (uint8_t e = 0; e < node.exitCount; ++e) {
                EXPECT_GT(node.exits[e].to, n) << node.name;
                EXPECT_LT(node.exits[e].to, s.nodeCount) << node.name;
                EXPECT_GT(node.exits[e].minutes, 0u) << node.name;
                ASSERT_NE(node.exits[e].label, nullptr) << node.name;
            }
            if (node.exitCount == 2) {
                EXPECT_NE(node.exits[0].to, node.exits[1].to) << node.name << ": a fork to one place";
            }
        }
    }
}

TEST(World, EveryNodeIsReachableAndCanReachTheBoss)
{
    for (const StageDef& s : kStages) {
        const uint32_t fromStart = reachable(s, 0);
        for (uint8_t n = 0; n < s.nodeCount; ++n) {
            EXPECT_TRUE(fromStart & (1u << n)) << s.name << ": " << s.nodes[n].name << " cannot be reached";
            EXPECT_TRUE(reachable(s, n) & (1u << (s.nodeCount - 1)))
                << s.name << ": " << s.nodes[n].name << " cannot reach the boss";
        }
    }
}

TEST(World, EveryStageHasRealChoicesAHutAndSomethingToFind)
{
    for (const StageDef& s : kStages) {
        int forks = 0, huts = 0, finds = 0, encounters = 0;
        for (uint8_t n = 0; n < s.nodeCount; ++n) {
            forks      += s.nodes[n].exitCount == 2 ? 1 : 0;
            huts       += s.nodes[n].kind == NodeKind::Hut ? 1 : 0;
            finds      += (s.nodes[n].kind == NodeKind::Cache || s.nodes[n].kind == NodeKind::Trail) ? 1 : 0;
            encounters += s.nodes[n].kind == NodeKind::Encounter ? 1 : 0;
        }
        EXPECT_GE(forks, 2) << s.name;
        EXPECT_EQ(huts, 1) << s.name;
        EXPECT_GE(finds, 1) << s.name;
        EXPECT_GE(encounters, 2) << s.name;
    }
}

TEST(World, NodeParametersAreInRange)
{
    for (const StageDef& s : kStages) {
        for (uint8_t n = 0; n < s.nodeCount; ++n) {
            const NodeDef& node = s.nodes[n];
            switch (node.kind) {
                case NodeKind::Obstacle: EXPECT_LT(node.param, Obstacle::Count) << node.name; break;
                case NodeKind::Cache:    EXPECT_LT(node.param, Item::Count) << node.name; break;
                case NodeKind::Trail:    EXPECT_LE(node.param, 100u) << node.name; break;
                default: break;
            }
        }
    }
}

TEST(World, PoolsHoldOnlyRegularSpiritsAndAtLeastOneForDaylight)
{
    for (const StageDef& s : kStages) {
        int daylight = 0;
        for (uint8_t i = 0; i < s.poolCount; ++i) {
            const SpeciesId id = s.pool[i];
            ASSERT_LT(id, Spirit::Count) << s.name;
            EXPECT_FALSE(isBoss(id)) << s.name;
            daylight += (kSpecies[id].flags & kSpeciesNight) == 0 ? 1 : 0;
        }
        EXPECT_GE(daylight, 1) << s.name;
        EXPECT_TRUE(isBoss(s.boss)) << s.name;
    }
}

TEST(World, NodeLookupsAreBoundsChecked)
{
    EXPECT_EQ(&stageDef(99), &kStages[0]);
    EXPECT_EQ(nodeDef(0, 200).kind, NodeKind::Start);
}

TEST(World, TimeOfDayFollowsTheClock)
{
    EXPECT_EQ(timeOfDay(6 * 60), TimeOfDay::Dawn);
    EXPECT_EQ(timeOfDay(8 * 60), TimeOfDay::Day);
    EXPECT_EQ(timeOfDay(18 * 60), TimeOfDay::Dusk);
    EXPECT_EQ(timeOfDay(20 * 60), TimeOfDay::Night);
    EXPECT_EQ(timeOfDay(2 * 60), TimeOfDay::Night);
    EXPECT_EQ(timeOfDay(24 * 60 + 9 * 60), TimeOfDay::Day);   // the next day
}
