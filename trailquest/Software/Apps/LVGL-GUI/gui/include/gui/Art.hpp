/**
 ******************************************************************************
 * @file    Art.hpp
 * @brief   Which sprite shows which spirit, and the scene's colours.
 *
 * The Valley Path's five (Q2) and the Switchback Climb's and Scree Fields'
 * ten (Q4); a spirit without its own art yet (stages 4 and 5) shows as the Red
 * Fox, which only a test build can reach.
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
        case Spirit::RedFox:      return TRAILQUEST_SPRITE_FOX;
        case Spirit::BrownBear:   return TRAILQUEST_SPRITE_BEAR;
        case Spirit::RubyBeetle:  return TRAILQUEST_SPRITE_BEETLE;
        case Spirit::SandCrab:    return TRAILQUEST_SPRITE_CRAB;
        case Spirit::OldTortoise: return TRAILQUEST_SPRITE_TORTOISE;
        case Spirit::FireNewt:    return TRAILQUEST_SPRITE_FIRE_NEWT;
        case Spirit::GnatSwarm:   return TRAILQUEST_SPRITE_GNAT_SWARM;
        case Spirit::BarnOwl:     return TRAILQUEST_SPRITE_BARN_OWL;
        case Spirit::PygmyShrew:  return TRAILQUEST_SPRITE_PYGMY_SHREW;
        case Spirit::GreatStag:   return TRAILQUEST_SPRITE_GREAT_STAG;
        case Spirit::RockGoat:    return TRAILQUEST_SPRITE_ROCK_GOAT;
        case Spirit::MistWisp:    return TRAILQUEST_SPRITE_MIST_WISP;
        case Spirit::WallLizard:  return TRAILQUEST_SPRITE_WALL_LIZARD;
        case Spirit::Marmot:      return TRAILQUEST_SPRITE_MARMOT;
        case Spirit::RamKing:     return TRAILQUEST_SPRITE_RAM_KING;
        default:                  return TRAILQUEST_SPRITE_FOX;
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
