/**
 ******************************************************************************
 * @file    Encounter.hpp
 * @brief   What a place on the trail throws at the runner: which spirit, at
 *          what level, and how an obstacle goes.
 *
 * Pure and deterministic (all chance through the Rng). The engine is in
 * Encounter.cpp (service only); the chance of clearing an obstacle is here, so
 * the GUI can show it.
 ******************************************************************************
 */

#ifndef TRAILQUEST_ENCOUNTER_HPP
#define TRAILQUEST_ENCOUNTER_HPP

#include <cstdint>

#include "Beat.hpp"
#include "Rng.hpp"
#include "Types.hpp"
#include "data/Stages.hpp"

namespace TrailQuest
{

/// Each lap round the Ridge puts the spirits this many levels higher.
constexpr uint8_t kLapLevelStep = 5;

/// A boss is this many levels above its stage's band.
constexpr uint8_t kBossLevelAbove = 1;

/// Obstacle odds, per cent.
constexpr int32_t kObstacleEven     = 50;
constexpr int32_t kObstaclePerPoint = 5;
constexpr int32_t kObstacleMin      = 10;
constexpr int32_t kObstacleMax      = 95;

/// The beat's effect on an obstacle: a perfect jump helps a lot.
inline int32_t obstacleBeatBonus(BeatResult b)
{
    switch (b) {
        case BeatResult::Perfect: return 25;
        case BeatResult::Good:    return 10;
        default:                  return -10;
    }
}

/// The per cent chance of clearing @p ob with @p stat in the tested affinity.
inline uint8_t obstacleChance(const ObstacleDef& ob, uint8_t stat, BeatResult beat)
{
    int32_t c = kObstacleEven + kObstaclePerPoint * (static_cast<int32_t>(stat) - ob.difficulty)
              + obstacleBeatBonus(beat);
    c = c < kObstacleMin ? kObstacleMin : (c > kObstacleMax ? kObstacleMax : c);
    return static_cast<uint8_t>(c);
}

/// A spirit from @p stage's pool, by the time of day: night spirits only
/// outside full daylight. If nothing is out (a pool of night spirits at
/// noon), any of the pool.
SpeciesId pickSpecies(const StageDef& stage, TimeOfDay tod, Rng& rng);

/// A level in @p stage's band, raised for the lap, held to the maximum.
uint8_t pickLevel(const StageDef& stage, uint8_t lap, Rng& rng);

/// The stage boss's level for @p lap.
uint8_t bossLevel(const StageDef& stage, uint8_t lap);

/// A random consumable found on the trail: Gel half the time, Water three in
/// ten, a salt tab the rest.
ItemId pickFind(Rng& rng);

} // namespace TrailQuest

#endif // TRAILQUEST_ENCOUNTER_HPP
