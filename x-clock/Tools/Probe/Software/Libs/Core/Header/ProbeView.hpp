/**
 ******************************************************************************
 * @file    ProbeView.hpp
 * @brief   What the Clock Probe's screen shows, as plain data the service
 *          fills and the GUI draws. Header-only and free of the SDK: the GUI
 *          process links no library source (Trail Quest NOTES, "Toolchain").
 *
 * The screen is one face: a ring of seven day segments on the rim (the week,
 * Monday at twelve o'clock, graded none / low / high), the time, then seven
 * short rows, each with a light. Every text is made by ProbeText.cpp and
 * measured in the host tests to fit the round face.
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_PROBE_VIEW_HPP
#define CLOCK_PROBE_PROBE_VIEW_HPP

#include <cstdint>

namespace ClockProbe
{

/// Rows under the time, and the longest text in one (the NUL included). The bottom row's chord on
/// the 240 px disc is about 150 px, and Poppins 14 is about 7.5 px a character.
inline constexpr uint8_t kRows     = 7;
inline constexpr uint8_t kRowChars = 24;

/// The ring's days, and the points of the VO2max trend (X Run keeps ten runs: Vo2Config::kHistoryRuns).
inline constexpr uint8_t kRingDays  = 7;
inline constexpr uint8_t kTrendMax  = 10;

/// A row's light: grey is "not known yet", green is a good answer, amber an answer with a catch,
/// red a failure.
enum class Light : uint8_t { Grey = 0, Green = 1, Amber = 2, Red = 3 };

/// A day's activity, graded none, low or high (ActivityLevel.hpp).
enum class Level : uint8_t { None = 0, Low = 1, High = 2 };

struct ViewRow {
    uint8_t light = 0;               ///< Light
    char    text[kRowChars] = {};
};

struct ViewData {
    uint8_t level[kRingDays] = {};   ///< Level, Monday first
    int8_t  today            = -1;   ///< 0 (Monday) to 6, or -1 when the clock is unset
    uint8_t trendCount       = 0;    ///< points in trend
    uint8_t trend[kTrendMax] = {};   ///< heights 2..20, oldest first
    ViewRow rows[kRows];
};

/// The time the face shows, pushed by the service once a minute.
struct ClockData {
    uint8_t hour   = 0;
    uint8_t minute = 0;
    uint8_t wday   = 0;     ///< Monday 0
    uint8_t mday   = 1;
    uint8_t month  = 1;     ///< 1 to 12
    uint8_t is12h  = 0;
    uint8_t valid  = 0;     ///< 0 while the watch has no date
};

} // namespace ClockProbe

#endif // CLOCK_PROBE_PROBE_VIEW_HPP
