/**
 ******************************************************************************
 * @file    MapZoom.hpp
 * @brief   HybridX Trail: the run map's zoom levels, stepped with UP and DOWN.
 *
 * Nine fixed scales, in steps of roughly 1.6x, from 60 m to 3.5 km (metres
 * from the runner to the edge of the round screen), then the whole route.
 * On the map, UP (L1) zooms in one step and DOWN (L2) zooms out one, and
 * both stop at the ends. It starts at 150 m: a few junctions ahead, close
 * enough to see the path. The scale bar names a round distance about a third
 * of the way across, so the level is always readable from the map itself.
 * The level is kept in the Model, so it survives the action menu.
 ******************************************************************************
 */

#ifndef TRAIL_MAP_ZOOM_HPP
#define TRAIL_MAP_ZOOM_HPP

#include <cstdint>

namespace MapZoom
{

constexpr uint16_t kRadiiM[] = { 60, 100, 150, 250, 400, 700, 1200, 2000, 3500 };
constexpr uint8_t  kFixed    = sizeof(kRadiiM) / sizeof(kRadiiM[0]);
constexpr uint8_t  kWhole    = kFixed;       ///< the level after the fixed ones: the whole route
constexpr uint8_t  kLevels   = kFixed + 1;
constexpr uint8_t  kDefault  = 2;            ///< 150 m

/// One step closer in (stops at the closest).
constexpr uint8_t in(uint8_t level) { return level > 0 ? static_cast<uint8_t>(level - 1) : level; }
/// One step further out, as far as the whole route.
constexpr uint8_t out(uint8_t level) { return level + 1 < kLevels ? static_cast<uint8_t>(level + 1) : level; }

} // namespace MapZoom

#endif // TRAIL_MAP_ZOOM_HPP
