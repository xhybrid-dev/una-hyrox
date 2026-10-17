/**
 ******************************************************************************
 * @file    Encounter.cpp
 * @brief   Which spirit, what level, what is found (see Encounter.hpp).
 ******************************************************************************
 */

#include "Encounter.hpp"

#include "Progress.hpp"
#include "data/Items.hpp"
#include "data/Species.hpp"

namespace TrailQuest
{

namespace
{
bool isOut(SpeciesId id, TimeOfDay tod)
{
    return (speciesDef(id).flags & kSpeciesNight) == 0 || tod != TimeOfDay::Day;
}

uint8_t lapped(uint32_t level, uint8_t lap)
{
    const uint32_t v = level + static_cast<uint32_t>(kLapLevelStep) * lap;
    return clampLevel(static_cast<uint8_t>(v > kMaxLevel ? kMaxLevel : v));
}
} // namespace

SpeciesId pickSpecies(const StageDef& stage, TimeOfDay tod, Rng& rng)
{
    const uint8_t count = stage.poolCount <= kMaxPool ? stage.poolCount : kMaxPool;
    if (count == 0) {
        return Spirit::Stitch;
    }
    SpeciesId out[kMaxPool];
    uint8_t   n = 0;
    for (uint8_t i = 0; i < count; ++i) {
        if (stage.pool[i] < Spirit::Count && isOut(stage.pool[i], tod)) {
            out[n++] = stage.pool[i];
        }
    }
    if (n == 0) {
        for (uint8_t i = 0; i < count; ++i) {
            if (stage.pool[i] < Spirit::Count) {
                out[n++] = stage.pool[i];
            }
        }
    }
    return n == 0 ? static_cast<SpeciesId>(Spirit::Stitch) : out[rng.below(n)];
}

uint8_t pickLevel(const StageDef& stage, uint8_t lap, Rng& rng)
{
    const uint8_t lo = stage.levelMin;
    const uint8_t hi = stage.levelMax >= lo ? stage.levelMax : lo;
    return lapped(lo + rng.below(static_cast<uint32_t>(hi - lo) + 1u), lap);
}

uint8_t bossLevel(const StageDef& stage, uint8_t lap)
{
    return lapped(static_cast<uint32_t>(stage.levelMax) + kBossLevelAbove, lap);
}

ItemId pickFind(Rng& rng)
{
    const uint32_t r = rng.below(10);
    return r < 5 ? static_cast<ItemId>(Item::Gel) : (r < 8 ? static_cast<ItemId>(Item::Water) : static_cast<ItemId>(Item::SaltTab));
}

} // namespace TrailQuest
