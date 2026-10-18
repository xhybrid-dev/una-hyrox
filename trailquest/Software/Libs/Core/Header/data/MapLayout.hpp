/**
 ******************************************************************************
 * @file    MapLayout.hpp
 * @brief   Where each place of a stage sits on the map screen.
 *
 * Header-only (the GUI draws the map). All five stages share one shape (nine
 * places: two ways from the start, a hut in the middle, two ways to the boss),
 * so one layout serves them all; MapLayoutTest checks that every stage's paths
 * match it, every place sits inside the round face clear of the text rows,
 * and no path runs over a place it does not join.
 *
 * Positions are screen pixels (240 x 240), the trail running left to right.
 ******************************************************************************
 */

#ifndef TRAILQUEST_DATA_MAP_LAYOUT_HPP
#define TRAILQUEST_DATA_MAP_LAYOUT_HPP

#include <cstdint>

namespace TrailQuest::MapLayout
{

struct Point {
    int16_t x;
    int16_t y;
};

constexpr uint8_t kPlaces   = 9;
constexpr int16_t kIconHalf = 7;   ///< Each place's icon is drawn in a 15 x 15 box.

inline constexpr Point kPoints[kPlaces] = {
    {  40, 110 },   // 0 the start
    {  75,  72 },   // 1
    {  80, 148 },   // 2
    { 115, 104 },   // 3 the hut
    { 132, 150 },   // 4
    { 150,  70 },   // 5
    { 170, 116 },   // 6
    { 186, 156 },   // 7
    { 205,  98 },   // 8 the boss
};

/// The place's position, or the start's for an index out of range.
inline Point point(uint8_t node)
{
    return kPoints[node < kPlaces ? node : 0u];
}

} // namespace TrailQuest::MapLayout

#endif // TRAILQUEST_DATA_MAP_LAYOUT_HPP
