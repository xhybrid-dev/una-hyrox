/**
 ******************************************************************************
 * @file    Stages.hpp
 * @brief   The Ridge: five stages, each a small graph of places joined by
 *          paths, with the forks where the runner chooses.
 *
 * Header-only data (the GUI draws the map from it). Rules the tests hold every
 * stage to (Tests/Host/WorldTest.cpp):
 *   - node 0 is the Start, the last node is the Boss, and nothing else is;
 *   - every exit goes to a later node, so the graph has no loops and every
 *     route ends; every node but the Boss has at least one exit;
 *   - every node can reach the Boss, and every node is reachable from Start.
 *
 * The trail clock runs on from stage to stage (it does not reset), so a slow
 * or unlucky run reaches the summit at dusk. That is what the headtorch and the
 * night spirits are for. First draft; Q4 tunes the stages.
 ******************************************************************************
 */

#ifndef TRAILQUEST_DATA_STAGES_HPP
#define TRAILQUEST_DATA_STAGES_HPP

#include <cstdint>

#include "Types.hpp"
#include "data/Items.hpp"
#include "data/Species.hpp"

namespace TrailQuest
{

enum class NodeKind : uint8_t {
    Start = 0,
    Trail,       ///< Quiet: param is the per cent chance of a find.
    Encounter,   ///< A Trail Spirit from the stage's pool.
    Obstacle,    ///< param is the obstacle id.
    Hut,         ///< Rest (full stamina), and where a loss returns you.
    Cache,       ///< param is the item found.
    Boss,
};

constexpr uint8_t kMaxExits  = 2;
constexpr uint8_t kMaxNodes  = 12;
constexpr uint8_t kMaxPool   = 4;
constexpr uint8_t kStageCount = 5;

struct ExitDef {
    uint8_t     to;
    uint8_t     minutes;   ///< Trail time to get there.
    const char* label;
};

struct NodeDef {
    const char* name;
    NodeKind    kind;
    uint8_t     param;
    uint8_t     exitCount;
    ExitDef     exits[kMaxExits];
};

struct ObstacleDef {
    const char* name;
    Affinity    check;         ///< The stat it tests.
    uint8_t     difficulty;    ///< The stat that gives an even chance.
    uint8_t     failStamina;   ///< What a failure costs.
    uint8_t     failMinutes;
};

namespace Obstacle
{
enum : uint8_t {
    SteppingStones = 0, Bog, SwitchbackScramble, Windfall, ScreeSlide, BoulderField,
    Cornice, ScrambleStep, IcySlab, SummitRidge,
    Count
};
} // namespace Obstacle

inline constexpr ObstacleDef kObstacles[Obstacle::Count] = {
    { "Stepping stones",     Affinity::Form, 11,  6, 10 },
    { "The bog",             Affinity::Grit, 12,  8, 20 },
    { "Switchback scramble", Affinity::Grit, 15,  8, 15 },
    { "Windfall",            Affinity::Pace, 15,  6, 15 },
    { "Scree slide",         Affinity::Form, 19, 10, 15 },
    { "Boulder field",       Affinity::Pace, 19,  8, 20 },
    { "Cornice",             Affinity::Form, 23, 12, 20 },
    { "Scramble step",       Affinity::Grit, 23, 10, 15 },
    { "Icy slab",            Affinity::Form, 27, 12, 20 },
    { "Summit ridge",        Affinity::Grit, 27, 14, 25 },
};

inline const ObstacleDef& obstacleDef(uint8_t id)
{
    return kObstacles[id < Obstacle::Count ? id : 0u];
}

struct StageDef {
    const char* name;
    Terrain     terrain;
    Weather     weather[2];       ///< One is picked when the stage starts.
    uint8_t     levelMin;         ///< The spirits' level band.
    uint8_t     levelMax;
    uint8_t     parLevel;         ///< The runner's expected level here.
    SpeciesId   pool[kMaxPool];
    uint8_t     poolCount;
    SpeciesId   boss;
    uint8_t     nodeCount;
    NodeDef     nodes[kMaxNodes];
};

/// The trail clock when a new game starts: 06:00.
constexpr uint16_t kRunStartMinutes = 6 * 60;

inline constexpr StageDef kStages[kStageCount] = {
    { "Valley Path", Terrain::Path, { Weather::Clear, Weather::Rain }, 1, 2, 1,
      { Spirit::RedFox, Spirit::BrownBear, Spirit::RubyBeetle, Spirit::SandCrab }, 4, Spirit::OldTortoise,
      9, {
        { "Trailhead",         NodeKind::Start,     0, 2, { { 1, 20, "Woods path" }, { 2, 15, "The stream" } } },
        { "Birch wood",        NodeKind::Encounter, 0, 1, { { 3, 25, "To the hut" } } },
        { "Stepping stones",   NodeKind::Obstacle,  Obstacle::SteppingStones, 2, { { 3, 15, "The bank" }, { 4, 10, "The meadow" } } },
        { "Valley hut",        NodeKind::Hut,       0, 2, { { 5, 20, "Old track" }, { 4, 15, "The meadow" } } },
        { "Long meadow",       NodeKind::Encounter, 0, 1, { { 6, 20, "Field edge" } } },
        { "Old gate",          NodeKind::Cache,     Item::Gel, 1, { { 6, 15, "The gate" } } },
        { "Foot of the hill",  NodeKind::Encounter, 0, 2, { { 8, 25, "Straight" }, { 7, 15, "By the bog" } } },
        { "The bog",           NodeKind::Obstacle,  Obstacle::Bog, 1, { { 8, 10, "The pool" } } },
        { "Tortoise pool",     NodeKind::Boss,      0, 0, {} },
      } },
    { "Switchback Climb", Terrain::Climb, { Weather::Clear, Weather::Mist }, 4, 5, 4,
      { Spirit::FireNewt, Spirit::GnatSwarm, Spirit::BarnOwl, Spirit::PygmyShrew }, 4, Spirit::GreatStag,
      9, {
        { "Forest gate",       NodeKind::Start,     0, 2, { { 1, 30, "Zig-zags" }, { 2, 20, "Climb up" } } },
        { "First bend",        NodeKind::Encounter, 0, 1, { { 3, 30, "Zig-zag on" } } },
        { "The scramble",      NodeKind::Obstacle,  Obstacle::SwitchbackScramble, 2, { { 3, 20, "The path" }, { 4, 15, "Push on" } } },
        { "Halfway hut",       NodeKind::Hut,       0, 2, { { 5, 25, "Forest way" }, { 4, 20, "Steep bit" } } },
        { "Clearing",          NodeKind::Encounter, 0, 1, { { 6, 25, "The trees" } } },
        { "Woodcutter's path", NodeKind::Trail,     40, 1, { { 6, 20, "Back up" } } },
        { "Top bend",          NodeKind::Encounter, 0, 2, { { 8, 25, "The crag" }, { 7, 15, "Windfall" } } },
        { "Windfall",          NodeKind::Obstacle,  Obstacle::Windfall, 1, { { 8, 10, "The crag" } } },
        { "Stag's crag",       NodeKind::Boss,      0, 0, {} },
      } },
    { "Scree Fields", Terrain::Scree, { Weather::Mist, Weather::Clear }, 7, 9, 8,
      { Spirit::RockGoat, Spirit::MistWisp, Spirit::WallLizard, Spirit::Marmot }, 4, Spirit::RamKing,
      9, {
        { "Scree foot",        NodeKind::Start,     0, 2, { { 1, 30, "The cairns" }, { 2, 20, "Across" } } },
        { "Cairn line",        NodeKind::Encounter, 0, 1, { { 3, 30, "The bothy" } } },
        { "Scree slide",       NodeKind::Obstacle,  Obstacle::ScreeSlide, 2, { { 3, 20, "The bothy" }, { 4, 15, "Keep going" } } },
        { "Scree bothy",       NodeKind::Hut,       0, 2, { { 5, 25, "Sheep path" }, { 4, 25, "Boulders" } } },
        { "Boulder maze",      NodeKind::Encounter, 0, 1, { { 6, 25, "The gully" } } },
        { "Shepherd's cairn",  NodeKind::Cache,     Item::SaltTab, 1, { { 6, 20, "The gully" } } },
        { "Gully top",         NodeKind::Encounter, 0, 2, { { 8, 25, "The ledge" }, { 7, 15, "Boulders" } } },
        { "Boulder field",     NodeKind::Obstacle,  Obstacle::BoulderField, 1, { { 8, 10, "The ledge" } } },
        { "The ledge",   NodeKind::Boss,      0, 0, {} },
      } },
    { "Ridge Line", Terrain::Ridge, { Weather::Wind, Weather::Clear }, 10, 12, 11,
      { Spirit::WindSprite, Spirit::Raven, Spirit::SnowHare, kNone }, 3, Spirit::GoldenEagle,
      9, {
        { "Ridge foot",        NodeKind::Start,     0, 2, { { 1, 30, "Broad back" }, { 2, 20, "The crest" } } },
        { "Broad back",        NodeKind::Encounter, 0, 1, { { 3, 30, "The refuge" } } },
        { "Cornice",           NodeKind::Obstacle,  Obstacle::Cornice, 2, { { 3, 20, "The refuge" }, { 4, 15, "The crest" } } },
        { "Ridge refuge",      NodeKind::Hut,       0, 2, { { 5, 25, "Rescue box" }, { 4, 20, "The crest" } } },
        { "False summit",      NodeKind::Encounter, 0, 1, { { 6, 25, "Onwards" } } },
        { "Rescue box",        NodeKind::Cache,     Item::Headtorch, 1, { { 6, 25, "Back up" } } },
        { "The narrows",       NodeKind::Encounter, 0, 2, { { 8, 25, "Knife-edge" }, { 7, 15, "Scramble" } } },
        { "Scramble step",     NodeKind::Obstacle,  Obstacle::ScrambleStep, 1, { { 8, 10, "The edge" } } },
        { "Knife-edge",    NodeKind::Boss,      0, 0, {} },
      } },
    { "The Summit", Terrain::Snow, { Weather::Snow, Weather::Clear }, 13, 15, 14,
      { Spirit::GhostGoat, Spirit::SnowFox, Spirit::MistWisp, kNone }, 3, Spirit::SummitWolf,
      9, {
        { "Snow line",         NodeKind::Start,     0, 2, { { 1, 30, "Zig-zags" }, { 2, 20, "Direct" } } },
        { "Snowfield",         NodeKind::Encounter, 0, 1, { { 3, 30, "To the hut" } } },
        { "Icy slab",          NodeKind::Obstacle,  Obstacle::IcySlab, 2, { { 3, 20, "To the hut" }, { 4, 15, "Climb up" } } },
        { "Summit hut",        NodeKind::Hut,       0, 2, { { 5, 25, "Last cairn" }, { 4, 20, "The slope" } } },
        { "The final slope",   NodeKind::Encounter, 0, 1, { { 6, 25, "Onwards" } } },
        { "Last cairn",        NodeKind::Cache,     Item::Gel, 1, { { 6, 20, "Onwards" } } },
        { "Below the top",     NodeKind::Encounter, 0, 2, { { 8, 25, "To the top" }, { 7, 15, "The ridge" } } },
        { "Summit ridge",      NodeKind::Obstacle,  Obstacle::SummitRidge, 1, { { 8, 10, "To the top" } } },
        { "The summit",        NodeKind::Boss,      0, 0, {} },
      } },
};

/// How strong the stage's spirits are against the runner, per cent of their
/// own row's stamina (Species.hpp): the needle gets sharper stage by stage
/// (Beat.hpp, beatFor), so the later stages' spirits give a little. Tuned
/// with Tests/Host/tools/balance_sim (docs/BALANCE.md).
inline constexpr uint8_t kSpiritPct[kStageCount] = { 100, 94, 98, 82, 81 };

inline uint8_t spiritPct(uint8_t stage)
{
    return kSpiritPct[stage < kStageCount ? stage : 0u];
}

/// The stage, or the first for an index out of range.
inline const StageDef& stageDef(uint8_t stage)
{
    return kStages[stage < kStageCount ? stage : 0u];
}

/// The node, or the stage's Start for an index out of range.
inline const NodeDef& nodeDef(uint8_t stage, uint8_t node)
{
    const StageDef& s = stageDef(stage);
    return s.nodes[node < s.nodeCount && node < kMaxNodes ? node : 0u];
}

} // namespace TrailQuest

#endif // TRAILQUEST_DATA_STAGES_HPP
