/**
 ******************************************************************************
 * @file    LapTiming.hpp
 * @brief   Whole-second lap times that add up to the session.
 ******************************************************************************
 * FIT stores times in whole seconds here. Rounding each lap's duration down on
 * its own loses up to a second per lap (16 s over the 31 laps of a Roxzone
 * race, 22 September test), so the laps summed to 48:52 against a session of
 * 49:08. Rounding the lap's two ends instead, and taking the difference, makes
 * every lap start exactly where the last one ended and the laps sum to the
 * session's elapsed time exactly.
 *
 * Pure C++: no SDK, no clock. Host-tested.
 ******************************************************************************
 */

#ifndef LAP_TIMING_HPP
#define LAP_TIMING_HPP

#include <cstdint>

namespace Race {

struct LapSeconds {
    uint32_t startSec;    ///< Offset of the lap's start from the race start
    uint32_t elapsedSec;  ///< Wall time of the lap, pauses included
    uint32_t activeSec;   ///< Elapsed minus paused: the lap's timer time
};

/**
 * @param cursorMs  Wall time of every earlier lap, summed, in ms
 * @param activeMs  Active time in the lap, in ms
 * @param pausedMs  Paused time in the lap, in ms
 */
constexpr LapSeconds lapSeconds(uint32_t cursorMs, uint32_t activeMs, uint32_t pausedMs)
{
    const uint32_t startSec = cursorMs / 1000u;
    const uint32_t endSec = (cursorMs + activeMs + pausedMs) / 1000u;
    const uint32_t elapsedSec = endSec - startSec;
    const uint32_t pausedSec = (pausedMs + 500u) / 1000u;
    return {startSec, elapsedSec, (pausedSec >= elapsedSec) ? 0u : elapsedSec - pausedSec};
}

}  // namespace Race

#endif  // LAP_TIMING_HPP
