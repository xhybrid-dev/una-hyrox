/**
 ******************************************************************************
 * @file    SimTemp.hpp
 * @brief   A made-up temperature for the PC simulator, which has no
 *          temperature sensor. Pure C++, header-only.
 *
 * Only the simulator build uses it (the service marks the reading as
 * simulated), so it never reaches a watch's log. The curve is a rough session
 * in ten minutes, repeating: a room, a climb into a hot room, a stay there, a
 * drop into something cold, a stay there. The numbers are invented to give
 * the screen something to show; they say nothing about the real sensor.
 ******************************************************************************
 */

#ifndef SAUNA_SIM_TEMP_HPP
#define SAUNA_SIM_TEMP_HPP

#include <cstdint>

namespace Sauna {

constexpr uint32_t kSimTempPeriodS = 600;

/// The made-up reading, in hundredths, @p elapsedS seconds in.
inline int32_t syntheticTempX100(uint32_t elapsedS)
{
    const uint32_t t = elapsedS % kSimTempPeriodS;
    if (t < 30) {
        return 2200;                                            // a room
    }
    if (t < 330) {
        return 2200 + static_cast<int32_t>(t - 30) * 21;        // 22 up to 85
    }
    if (t < 390) {
        return 8500;                                            // holding there
    }
    if (t < 450) {
        return 8500 - static_cast<int32_t>((t - 389) * 7300u / 60u);   // 85 down to 12
    }
    return 1200;                                                // cold
}

} // namespace Sauna

#endif // SAUNA_SIM_TEMP_HPP
