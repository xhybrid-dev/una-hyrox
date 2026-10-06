/**
 ******************************************************************************
 * @file    WorkoutText.hpp
 * @brief   The words the watch shows for a workout: step lines, targets,
 *          totals, why a file can't be read (Phase P3c).
 *
 * Pure and host-tested, so the screens only place text. Integer formatting
 * only (no %f, per the embedded rules); ASCII only (the watch's fonts); every
 * function writes a NUL-terminated string cut to @p cap and returns its
 * length. British English.
 ******************************************************************************
 */

#ifndef INTERVALS_WORKOUT_TEXT_HPP
#define INTERVALS_WORKOUT_TEXT_HPP

#include <cstddef>
#include <cstdint>

#include "WorkoutParser.hpp"
#include "WorkoutSummary.hpp"
#include "WorkoutTypes.hpp"

namespace Intervals::Text
{

/// "0:45", "3:50", "10:00", "1:05:00".
size_t clock(char* out, size_t cap, uint32_t seconds);

/// Seconds per mile from seconds per km, rounded.
uint32_t secPerMile(uint32_t secPerKm);

/// "400 m", "1 km", "1.5 km", "21.1 km"; imperial: "0.25 mi", "1 mi", "13.1 mi".
size_t distance(char* out, size_t cap, uint32_t metres, bool imperial);

/// "Warm-up", "Run", "Rest", "Cool-down".
const char* intensityWord(StepIntensity intensity);

/// A step's duration: "10:00", "400 m", "open".
size_t duration(char* out, size_t cap, const Step& step, bool imperial);

/// "3:50-4:10 /km" (or /mi), "Zone 4", "Zones 2-3", "140-150 bpm"; "" for
/// no target. Pace @p imperial converts to minutes per mile.
size_t target(char* out, size_t cap, const Target& target, bool imperial);

/// "Run 400 m", "Warm-up 10:00", "Rest 1:30", "Cool-down, open".
size_t stepLine(char* out, size_t cap, const Step& step, bool imperial);

/// The list's hint: "2.4 km, 34 min", "34 min, 1 open", "open". Minutes
/// rounded up, so a workout never looks shorter than it is.
size_t summary(char* out, size_t cap, const WorkoutSummary& sum, bool imperial);

/// Why a file can't be used, in a few words ("newer format: update app").
const char* problem(ParseError error, ValidationError validation);

} // namespace Intervals::Text

#endif // INTERVALS_WORKOUT_TEXT_HPP
