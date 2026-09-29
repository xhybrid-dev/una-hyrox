/**
 ******************************************************************************
 * @file    FaceCycle.hpp
 * @brief   Paging the run screens, skipping the map.
 *
 * With a route the run has a map and several data screens (navigation,
 * elevation, run, lap, status). UP and DOWN page the data screens (zoom on the
 * map itself), and R2 flips between the map and the data screen you were on:
 * so the map is one press from anywhere. Paging therefore wraps round the data
 * screens only and never lands on the map.
 ******************************************************************************
 */

#ifndef TRAIL_FACE_CYCLE_HPP
#define TRAIL_FACE_CYCLE_HPP

#include <cstdint>

namespace FaceCycle
{

/// From face @p at of @p count, one step in @p direction (+1 down, -1 up),
/// wrapping, and passing over face @p skip (the map; pass count for none).
constexpr uint8_t step(uint8_t count, uint8_t skip, uint8_t at, int direction)
{
    if (count == 0) {
        return 0;
    }
    int i = at;
    for (uint8_t tries = 0; tries < count; ++tries) {
        i = (i + (direction < 0 ? -1 : 1) + count) % count;
        if (i != skip) {
            return static_cast<uint8_t>(i);
        }
    }
    return at;   // only the skipped face exists
}

} // namespace FaceCycle

#endif // TRAIL_FACE_CYCLE_HPP
