/**
 ******************************************************************************
 * @file    ProbeSnapshot.hpp
 * @brief   The probe's readings as the screen shows them: one plain struct the
 *          service fills once a second and sends. Pure C++, header-only.
 ******************************************************************************
 */

#ifndef SAUNA_PROBE_SNAPSHOT_HPP
#define SAUNA_PROBE_SNAPSHOT_HPP

#include <cstdint>

#include "FixedPoint.hpp"
#include "ProbeStats.hpp"

namespace Sauna {

/// Where the CSV stands, so a failed write is seen on the wrist and not found
/// out when the file turns up empty.
enum class LogState : uint8_t {
    Off = 0,    ///< Not logging (sensors not started, or finished)
    Ok,         ///< The last write succeeded
    Failed,     ///< A write failed
};

struct ProbeSnapshot {
    // Time
    uint32_t elapsedS = 0;          ///< Seconds since the sensors started

    // Temperature: the sensor's value as it arrives, in hundredths of
    // whatever unit the driver uses (the probe does not assume degrees).
    int32_t  tempX100 = kX100Invalid;
    int32_t  tempMinX100 = kX100Invalid;
    int32_t  tempMaxX100 = kX100Invalid;
    uint32_t tempSamples = 0;
    uint32_t tempBad = 0;           ///< Samples the parser rejected
    uint32_t tempGapMs = 0;         ///< Gap between the last two samples
    uint32_t tempMinGapMs = 0;
    uint32_t tempMaxGapMs = 0;

    // Heart rate
    int32_t  trustX100 = kX100Invalid;
    uint32_t hrSamples = 0;
    uint16_t hr = 0;                ///< bpm; 0 until a sample arrives

    // Housekeeping
    uint16_t marks = 0;             ///< Presses of R1 so far
    uint16_t run = 0;               ///< Which run of the probe this is in the CSV
    uint16_t tempFields = 0;        ///< Fields in the temperature samples
    uint8_t  tempSense = 0;         ///< Sense
    uint8_t  hrSense = 0;           ///< Sense
    uint8_t  log = 0;               ///< LogState
    uint8_t  flags = 0;             ///< kFlag* below
};

constexpr uint8_t kFlagSimulated = 1u << 0;   ///< The temperature is made up (PC simulator only)
constexpr uint8_t kFlagFinished  = 1u << 1;   ///< The sensors have been stopped

} // namespace Sauna

#endif // SAUNA_PROBE_SNAPSHOT_HPP
