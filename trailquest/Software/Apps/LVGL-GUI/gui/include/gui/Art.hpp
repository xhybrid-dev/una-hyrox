/**
 ******************************************************************************
 * @file    Art.hpp
 * @brief   Which sprite shows which spirit, and the scene's colours.
 *
 * Q2 draws the Valley Path's five; a spirit without its own art yet (stages
 * 2-5, Q4) shows as Stitch, which only a test build can reach.
 ******************************************************************************
 */

#ifndef TRAILQUEST_ART_HPP
#define TRAILQUEST_ART_HPP

#include <cstdint>

#include "Types.hpp"
#include "data/Species.hpp"
#include "gui/Sprites.hpp"

namespace Art
{

inline uint8_t spriteFor(TrailQuest::SpeciesId sp)
{
    using namespace TrailQuest;
    switch (sp) {
        case Spirit::Stitch:      return TRAILQUEST_SPRITE_STITCH;
        case Spirit::Bonk:        return TRAILQUEST_SPRITE_BONK;
        case Spirit::Blister:     return TRAILQUEST_SPRITE_BLISTER;
        case Spirit::Chafe:       return TRAILQUEST_SPRITE_CHAFE;
        case Spirit::OldMossback: return TRAILQUEST_SPRITE_MOSSBACK;
        default:                  return TRAILQUEST_SPRITE_STITCH;
    }
}

/// The sky over a duel, by weather and the dark.
inline uint32_t skyFor(uint8_t weather, bool dark)
{
    using TrailQuest::Weather;
    if (dark) {
        return 0x000040;
    }
    switch (static_cast<Weather>(weather)) {
        case Weather::Rain: return 0x404080;
        case Weather::Mist: return 0x808080;
        case Weather::Snow: return 0x8080C0;
        case Weather::Wind: return 0x4080C0;
        default:            return 0x0040C0;
    }
}

/// The map's ground, by stage and the time of day.
inline uint32_t groundFor(uint8_t stage, uint8_t timeOfDay)
{
    if (timeOfDay == static_cast<uint8_t>(TrailQuest::TimeOfDay::Night)) {
        return 0x000040;
    }
    static const uint32_t kGround[] = { 0x004000, 0x004000, 0x404040, 0x404000, 0x404080 };
    return kGround[stage < 5 ? stage : 0];
}

} // namespace Art

#endif // TRAILQUEST_ART_HPP
