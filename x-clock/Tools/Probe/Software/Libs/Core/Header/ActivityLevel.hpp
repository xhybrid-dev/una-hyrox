/**
 ******************************************************************************
 * @file    ActivityLevel.hpp
 * @brief   A day's activity graded none, low or high, for the ring of seven.
 *
 * PLACEHOLDER thresholds. "Activity level" is not yet defined (x-clock NOTES,
 * "Jon's wish list"): the family's training load, the watch's all-day activity,
 * or a blend. This grades the family's load (Edwards' TRIMP, TrainingLoad.hpp)
 * so the ring can be drawn on a real week now; a session with no heart rate has
 * no load, so its minutes stand in at two points a minute. The real figures
 * that choose the threshold are in the probe's logs (days.csv, scan_*.txt).
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_ACTIVITY_LEVEL_HPP
#define CLOCK_PROBE_ACTIVITY_LEVEL_HPP

#include <cstdint>

#include "ProbeView.hpp"

namespace ClockProbe
{

/// Points at or above which a day is high. A guess: Jon's 16-minute ride scored 39; an hour in
/// zone 2 scores about 120.
inline constexpr uint32_t kHighScore = 100;

/// A day's score: its load, or twice its minutes when no session carried one.
constexpr uint32_t dayScore(uint16_t load, uint16_t minutes)
{
    return load > 0 ? load : static_cast<uint32_t>(minutes) * 2u;
}

constexpr Level levelFor(uint8_t sessions, uint16_t load, uint16_t minutes)
{
    if (sessions == 0) {
        return Level::None;
    }
    return dayScore(load, minutes) >= kHighScore ? Level::High : Level::Low;
}

} // namespace ClockProbe

#endif // CLOCK_PROBE_ACTIVITY_LEVEL_HPP
