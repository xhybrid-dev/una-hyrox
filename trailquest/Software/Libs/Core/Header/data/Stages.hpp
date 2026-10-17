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
    Hut,         ///< Rest (full stamina), the save point, and where a loss returns you.
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
      { Spirit::Stitch, Spirit::Bonk, Spirit::Blister, Spirit::Chafe }, 4, Spirit::OldMossback,
      9, {
        { "Trailhead",         NodeKind::Start,     0, 2, { { 1, 20, "The woods path" }, { 2, 15, "The stream path" } } },
        { "Birch wood",        NodeKind::Encounter, 0, 1, { { 3, 25, "On to the hut" } } },
        { "Stepping stones",   NodeKind::Obstacle,  Obstacle::SteppingStones, 2, { { 3, 15, "Up the bank" }, { 4, 10, "Across the meadow" } } },
        { "Valley hut",        NodeKind::Hut,       0, 2, { { 5, 20, "The old track" }, { 4, 15, "Down to the meadow" } } },
        { "Long meadow",       NodeKind::Encounter, 0, 1, { { 6, 20, "Along the field edge" } } },
        { "Old gate",          NodeKind::Cache,     Item::Gel, 1, { { 6, 15, "Through the gate" } } },
        { "Foot of the hill",  NodeKind::Encounter, 0, 2, { { 8, 25, "The straight way" }, { 7, 15, "Round by the bog" } } },
        { "The bog",           NodeKind::Obstacle,  Obstacle::Bog, 1, { { 8, 10, "On to the pool" } } },
        { "Mossback's pool",   NodeKind::Boss,      0, 0, {} },
      } },
    { "Switchback Climb", Terrain::Climb, { Weather::Clear, Weather::Mist }, 4, 5, 4,
      { Spirit::Lactic, Spirit::Cramp, Spirit::Puff, Spirit::Niggle }, 4, Spirit::TheSteep,
      9, {
        { "Forest gate",       NodeKind::Start,     0, 2, { { 1, 30, "The zig-zags" }, { 2, 20, "Straight up" } } },
        { "First bend",        NodeKind::Encounter, 0, 1, { { 3, 30, "Keep zig-zagging" } } },
        { "The scramble",      NodeKind::Obstacle,  Obstacle::SwitchbackScramble, 2, { { 3, 20, "Rejoin the path" }, { 4, 15, "Push straight on" } } },
        { "Halfway hut",       NodeKind::Hut,       0, 2, { { 5, 25, "The forest track" }, { 4, 20, "The steep bit" } } },
        { "Clearing",          NodeKind::Encounter, 0, 1, { { 6, 25, "Into the trees" } } },
        { "Woodcutter's path", NodeKind::Trail,     40, 1, { { 6, 20, "Back to the climb" } } },
        { "Top bend",          NodeKind::Encounter, 0, 2, { { 8, 25, "Up to the crag" }, { 7, 15, "Over the windfall" } } },
        { "Windfall",          NodeKind::Obstacle,  Obstacle::Windfall, 1, { { 8, 10, "To the crag" } } },
        { "The Steep's crag",  NodeKind::Boss,      0, 0, {} },
      } },
    { "Scree Fields", Terrain::Scree, { Weather::Mist, Weather::Clear }, 7, 9, 8,
      { Spirit::ScreeHopper, Spirit::Doubt, Spirit::Slip, Spirit::AnkleBiter }, 4, Spirit::TheShaleKing,
      9, {
        { "Scree foot",        NodeKind::Start,     0, 2, { { 1, 30, "Along the cairns" }, { 2, 20, "Straight across" } } },
        { "Cairn line",        NodeKind::Encounter, 0, 1, { { 3, 30, "To the bothy" } } },
        { "Scree slide",       NodeKind::Obstacle,  Obstacle::ScreeSlide, 2, { { 3, 20, "To the bothy" }, { 4, 15, "Keep going" } } },
        { "Scree bothy",       NodeKind::Hut,       0, 2, { { 5, 25, "The shepherd's path" }, { 4, 25, "Over the boulders" } } },
        { "Boulder maze",      NodeKind::Encounter, 0, 1, { { 6, 25, "Up the gully" } } },
        { "Shepherd's cairn",  NodeKind::Cache,     Item::SaltTab, 1, { { 6, 20, "Rejoin the gully" } } },
        { "Gully top",         NodeKind::Encounter, 0, 2, { { 8, 25, "The ram's ledge" }, { 7, 15, "Across the boulders" } } },
        { "Boulder field",     NodeKind::Obstacle,  Obstacle::BoulderField, 1, { { 8, 10, "To the ledge" } } },
        { "The ram's ledge",   NodeKind::Boss,      0, 0, {} },
      } },
    { "Ridge Line", Terrain::Ridge, { Weather::Wind, Weather::Clear }, 10, 12, 11,
      { Spirit::Gale, Spirit::Vertigo, Spirit::Hypo, kNone }, 3, Spirit::TheKnifeEdge,
      9, {
        { "Ridge foot",        NodeKind::Start,     0, 2, { { 1, 30, "The broad back" }, { 2, 20, "The crest" } } },
        { "Broad back",        NodeKind::Encounter, 0, 1, { { 3, 30, "To the refuge" } } },
        { "Cornice",           NodeKind::Obstacle,  Obstacle::Cornice, 2, { { 3, 20, "To the refuge" }, { 4, 15, "Along the crest" } } },
        { "Ridge refuge",      NodeKind::Hut,       0, 2, { { 5, 25, "Down to the rescue box" }, { 4, 20, "Back on the crest" } } },
        { "False summit",      NodeKind::Encounter, 0, 1, { { 6, 25, "Onwards" } } },
        { "Rescue box",        NodeKind::Cache,     Item::Headtorch, 1, { { 6, 25, "Climb back up" } } },
        { "The narrows",       NodeKind::Encounter, 0, 2, { { 8, 25, "The knife-edge" }, { 7, 15, "The scramble step" } } },
        { "Scramble step",     NodeKind::Obstacle,  Obstacle::ScrambleStep, 1, { { 8, 10, "To the edge" } } },
        { "The knife-edge",    NodeKind::Boss,      0, 0, {} },
      } },
    { "The Summit", Terrain::Snow, { Weather::Snow, Weather::Clear }, 13, 15, 14,
      { Spirit::ThinAir, Spirit::Frostnip, Spirit::Doubt, kNone }, 3, Spirit::SummitWolf,
      9, {
        { "Snow line",         NodeKind::Start,     0, 2, { { 1, 30, "The zig-zags" }, { 2, 20, "The direct line" } } },
        { "Snowfield",         NodeKind::Encounter, 0, 1, { { 3, 30, "To the shelter" } } },
        { "Icy slab",          NodeKind::Obstacle,  Obstacle::IcySlab, 2, { { 3, 20, "To the shelter" }, { 4, 15, "Straight up" } } },
        { "Summit shelter",    NodeKind::Hut,       0, 2, { { 5, 25, "By the last cairn" }, { 4, 20, "The final slope" } } },
        { "The final slope",   NodeKind::Encounter, 0, 1, { { 6, 25, "Onwards" } } },
        { "Last cairn",        NodeKind::Cache,     Item::Gel, 1, { { 6, 20, "Onwards" } } },
        { "Below the top",     NodeKind::Encounter, 0, 2, { { 8, 25, "To the top" }, { 7, 15, "The summit ridge" } } },
        { "Summit ridge",      NodeKind::Obstacle,  Obstacle::SummitRidge, 1, { { 8, 10, "To the top" } } },
        { "The summit",        NodeKind::Boss,      0, 0, {} },
      } },
};

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
