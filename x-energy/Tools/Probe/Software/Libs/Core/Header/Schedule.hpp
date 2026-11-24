/**
 ******************************************************************************
 * @file    Schedule.hpp
 * @brief   Which night or day a local time is in, and the mode it records in
 *          (x-energy/docs/PROBE.md, "The nights").
 *
 * Pure: the caller turns the clock into a LocalTime (localtime_r on the
 * watch), so tests inject any time they like.
 ******************************************************************************
 */

#ifndef ENERGY_PROBE_SCHEDULE_HPP
#define ENERGY_PROBE_SCHEDULE_HPP

#include <cstdint>

#include "ProbeMode.hpp"

namespace EnergyProbe
{

/// A local time: the day number of the local date, hour and minute.
struct LocalTime {
    int32_t day    = 0;
    uint8_t hour   = 0;
    uint8_t minute = 0;
};

/// A night (which belongs to the date of its evening) or the daytime of a date.
struct Period {
    bool    night = false;
    int32_t day   = 0;

    bool operator==(const Period& o) const { return night == o.night && day == o.day; }
    bool operator!=(const Period& o) const { return !(*this == o); }
};

/// What the wearer chose, and what the probe remembers.
struct Settings {
    uint8_t window       = kDefaultWindow;   ///< index into kWindows
    Mode    dayMode      = Mode::Full;       ///< daytime: Full or Off
    int32_t firstNight   = -1;               ///< the rotation's night 0; -1 until the first night
    int32_t overrideDay  = -1;               ///< the night the override is for; -1 none
    uint8_t overrideMode = kOverrideAuto;    ///< 0 auto, else 1 + Mode
};

/// The night or day `t` falls in, for night window `w`.
Period periodAt(const LocalTime& t, Window w);

/// The night that is on now, or else the one that starts next (the "Tonight" page).
int32_t tonight(const LocalTime& t, Window w);

/// The mode a night follows with no override: the rotation from firstNight (the first night
/// counts as 0 if firstNight is not yet set).
Mode rotationFor(int32_t nightDay, const Settings& s);

/// The mode the period records in, override included.
Mode modeFor(const Period& p, const Settings& s);

/// Minutes from `t` to the start of its period (0 at the first minute), up to a day.
uint32_t minutesIntoPeriod(const LocalTime& t, Window w);

} // namespace EnergyProbe

#endif // ENERGY_PROBE_SCHEDULE_HPP
