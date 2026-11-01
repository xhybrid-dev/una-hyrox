/**
 ******************************************************************************
 * @file    RideSnapshot.hpp
 * @brief   The running ride as the screens show it: one plain struct the
 *          service fills once a second and sends. Pure C++, header-only.
 ******************************************************************************
 */

#ifndef RIDE_RIDE_SNAPSHOT_HPP
#define RIDE_RIDE_SNAPSHOT_HPP

#include <cstdint>

#include "RideModel.hpp"

namespace Ride {

/// Battery level when the watch has not said.
constexpr uint8_t kBatteryUnknown = 255;

/// The GPS as the screens show it. Off indoors; outdoors Searching until a fix.
enum class GpsState : uint8_t { Off = 0, Searching = 1, Fix = 2 };

struct SessionSnapshot {
    uint8_t  state = 0;           ///< RunState
    uint8_t  laps = 0;            ///< The current lap's number (1 on)
    uint8_t  zone = 0;            ///< 0 below zone 1 or no HR, else 1 to 5
    uint8_t  autoLapMin = 0;      ///< 0: off
    uint16_t hr = 0;              ///< bpm, 0 none
    uint8_t  logOk = 1;           ///< 0 when a write to the activity file failed
    uint8_t  hour = 0;            ///< Local time of day
    uint8_t  minute = 0;
    uint8_t  battery = kBatteryUnknown;   ///< 0 to 100
    uint32_t lapMs = 0;           ///< Active time in this lap
    uint32_t activeMs = 0;
    uint32_t elapsedMs = 0;
    uint32_t kcalX10 = 0;         ///< Calories so far, tenths of a kcal
    uint32_t zoneMs[kZoneCount] = {};   ///< Active time so far in each zone
    uint8_t  outdoor = 0;         ///< 1 for a GPS ride
    uint8_t  gps = 0;             ///< GpsState
    uint32_t speedMmS = 0;        ///< Speed now (0 when still, or no GPS)
    uint32_t distCm = 0;          ///< Distance so far
    uint32_t ascentCm = 0;
    uint32_t lapDistCm = 0;
    uint32_t avgSpeedMmS = 0;     ///< Over the ride's active time
    uint32_t lapAvgSpeedMmS = 0;
};

/// What a finished ride came to, for the summary screen.
struct SavedSession {
    uint8_t        saved = 0;     ///< 1 when the activity file was written
    uint8_t        outdoor = 0;   ///< 1 for a GPS ride
    uint32_t       activeMs = 0;
    SessionSummary summary;
};

/// A lap just ended (R2, or the auto lap): what it was, for the lap toast.
struct LapEventData {
    uint8_t  finishedLap = 1;     ///< Its number, from 1
    uint8_t  autoEnded = 0;
    uint16_t hrAvg = 0;           ///< bpm, 0 none
    uint32_t durationMs = 0;
    uint32_t distCm = 0;          ///< 0 indoors
    uint32_t avgSpeedMmS = 0;
};

/// One lap of a finished ride, for the summary's lap pages.
struct LapRow {
    uint8_t  autoEnded = 0;
    uint16_t hrAvg = 0;
    uint16_t hrMax = 0;
    uint16_t kcal = 0;
    uint32_t durationMs = 0;
    uint32_t distCm = 0;          ///< 0 indoors
};

/// Laps on one summary screen: each takes two lines, and three fit the round face.
constexpr uint8_t kLapsPerPage = 3;

struct LapsPageData {
    uint8_t first = 0;            ///< Index of rows[0]
    uint8_t total = 0;            ///< Laps in the ride
    uint8_t count = 0;            ///< Rows filled, at most kLapsPerPage
    LapRow  rows[kLapsPerPage];
};

} // namespace Ride

#endif // RIDE_RIDE_SNAPSHOT_HPP
