/**
 ******************************************************************************
 * @file    Schedule.cpp
 * @brief   Which night or day a local time is in (see the header).
 ******************************************************************************
 */

#include "Schedule.hpp"

namespace EnergyProbe
{

Period periodAt(const LocalTime& t, Window w)
{
    const uint8_t h = t.hour;
    if (w.startHour > w.endHour) {   // crosses midnight, e.g. 22:00-08:00
        if (h >= w.startHour) {
            return Period{true, t.day};
        }
        if (h < w.endHour) {
            return Period{true, t.day - 1};
        }
        return Period{false, t.day};
    }
    // Starts at or after midnight, e.g. 00:00-10:00: the night of the evening before.
    if (h >= w.startHour && h < w.endHour) {
        return Period{true, t.day - 1};
    }
    return Period{false, t.day};
}

int32_t tonight(const LocalTime& t, Window w)
{
    const Period p = periodAt(t, w);
    if (p.night) {
        return p.day;
    }
    // Daytime: the night that starts this evening (or, for a window from midnight, tonight
    // at midnight, which still belongs to this evening's date).
    return t.day;
}

Mode rotationFor(int32_t nightDay, const Settings& s)
{
    const int32_t first = s.firstNight < 0 ? nightDay : s.firstNight;
    return rotationMode(nightDay - first);
}

Mode modeFor(const Period& p, const Settings& s)
{
    if (!p.night) {
        return s.dayMode == Mode::Off ? Mode::Off : Mode::Full;
    }
    if (s.overrideDay == p.day && s.overrideMode != kOverrideAuto && s.overrideMode < kOverrideCount) {
        return modeFrom(s.overrideMode - 1u);
    }
    return rotationFor(p.day, s);
}

uint32_t minutesIntoPeriod(const LocalTime& t, Window w)
{
    const Period   p      = periodAt(t, w);
    const uint32_t nowMin = t.hour * 60u + t.minute;
    const uint32_t start  = (p.night ? w.startHour : w.endHour) * 60u;
    return (nowMin + 24u * 60u - start) % (24u * 60u);
}

} // namespace EnergyProbe
