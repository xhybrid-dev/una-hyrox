/**
 ******************************************************************************
 * @file    MapZoom.hpp
 * @brief   HybridX Trail: the run map's zoom levels, on one button (R2).
 *
 * Four fixed scales, metres from the runner to the edge of the round screen,
 * then the whole route. The scale bar names a round distance about a third
 * of the way across (300 m: a 200 m bar; 750 m: 500 m; 1.5 km: 1 km; 3 km:
 * 2 km). The level is kept in the Model, so it survives the action menu.
 ******************************************************************************
 */

#ifndef MAP_ZOOM_HPP
#define MAP_ZOOM_HPP

#include <cstdint>

namespace MapZoom
{

constexpr uint16_t kRadiiM[] = { 300, 750, 1500, 3000 };
constexpr uint8_t  kFixed    = sizeof(kRadiiM) / sizeof(kRadiiM[0]);
constexpr uint8_t  kWhole    = kFixed;       ///< the level after the fixed ones: the whole route
constexpr uint8_t  kLevels   = kFixed + 1;

constexpr uint8_t next(uint8_t level) { return static_cast<uint8_t>((level + 1u) % kLevels); }

} // namespace MapZoom

#endif // MAP_ZOOM_HPP
