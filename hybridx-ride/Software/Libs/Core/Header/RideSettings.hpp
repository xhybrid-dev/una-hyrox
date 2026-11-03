/**
 ******************************************************************************
 * @file    RideSettings.hpp
 * @brief   What the app remembers between rides, as a small JSON file in
 *          its own folder. Service only (it uses the SDK's JSON reader and
 *          writer, which the GUI does not link).
 *
 * Version 2 (2 November 2026): {"v":2,"autolap":5,"outdoor":1,"autopause":1,
 * "lapkm":0,"type":0,"work":40,"rest":20,"rounds":6}. A version 1 file
 * ({"v":1,"autolap":5,"outdoor":1}) still reads: auto-pause on, no distance
 * lap, a free ride. The time and distance auto laps are one choice: at most
 * one of autolap and lapkm is set.
 ******************************************************************************
 */

#ifndef RIDE_RIDE_SETTINGS_HPP
#define RIDE_RIDE_SETTINGS_HPP

#include <cstddef>
#include <cstdint>

#include "RidePlan.hpp"

namespace Ride {

inline constexpr const char* kSettingsPath = "settings.json";

struct RideSettings {
    uint8_t    autoLapMin = 0;   ///< One of kAutoLapMinutes (0: off)
    uint8_t    autoLapKm = 0;    ///< One of kAutoLapKm (0: off); outdoors only
    bool       outdoor = true;   ///< The main menu's Outdoor / Indoor choice
    bool       autoPause = true; ///< Outdoors: pause when the bike stops
    PlanConfig plan;             ///< Indoors: Free, Intervals or Tabata

    /// A buffer big enough for any settings file.
    static constexpr size_t kMaxJsonBytes = 160;

    /// Write the file's text. Returns its length, or 0 (buf emptied) if it did not fit.
    size_t toJson(char* buf, size_t cap) const;
    /// Read it. False, and the defaults kept, if it is not version 1 or 2 with a known
    /// auto lap. Optional keys keep their defaults; the plan's numbers are clamped.
    bool fromJson(const char* text, size_t len);
};

} // namespace Ride

#endif // RIDE_RIDE_SETTINGS_HPP
