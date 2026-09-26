#include "HrZones.hpp"

namespace Intervals
{

uint8_t zoneOf(float hr, const uint8_t* thresholds, uint8_t count)
{
    if (count == 0 || hr <= 0.0f) {
        return 0;
    }

    const uint8_t thresholdCount = (count > kMaxHrThresholds) ? kMaxHrThresholds : count;
    uint8_t       zone           = 0;
    for (uint8_t i = 0; i < thresholdCount; ++i) {
        if (hr > static_cast<float>(thresholds[i])) {
            zone = i + 1;
        }
    }
    return zone;
}

} // namespace Intervals
