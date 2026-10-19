/**
 ******************************************************************************
 * @file    Items.hpp
 * @brief   What the runner carries: fuel, kit and one key item.
 *
 * Header-only data. The bag holds kBagCapacity consumables in all; a key item
 * (the headtorch) is kept apart and never used up.
 ******************************************************************************
 */

#ifndef TRAILQUEST_DATA_ITEMS_HPP
#define TRAILQUEST_DATA_ITEMS_HPP

#include <cstdint>

#include "Types.hpp"

namespace TrailQuest
{

struct ItemDef {
    const char* name;
    uint8_t     heal;        ///< Stamina restored when used.
    Effect      clears;      ///< The status it ends, if the runner has it.
    uint8_t     formBoost;   ///< Form added for the rest of the duel.
    bool        key;         ///< Kept, never used up, not counted in the bag.
};

namespace Item
{
enum : ItemId {
    Gel = 0,
    Water,
    SaltTab,
    Poles,
    Headtorch,
    Count
};
} // namespace Item

/// Consumables the bag holds in all.
constexpr uint8_t kBagCapacity = 6;

inline constexpr ItemDef kItems[Item::Count] = {
    { "Gel",       15, Effect::None,   0, false },
    { "Water",      8, Effect::Drain,  0, false },
    { "Salt tab",   4, Effect::Slow,   0, false },
    { "Poles",      0, Effect::None,   2, false },
    { "Headtorch",  0, Effect::None,   0, true  },
};

/// The item, or Gel for an id out of range.
inline const ItemDef& itemDef(ItemId id)
{
    return kItems[id < Item::Count ? id : static_cast<ItemId>(Item::Gel)];
}

} // namespace TrailQuest

#endif // TRAILQUEST_DATA_ITEMS_HPP
