/**
 ******************************************************************************
 * @file    WorkoutParser.hpp
 * @brief   Reads a workout file (JSON, schema in docs/WORKOUT_FILE.md) into a
 *          Workout (Phase P2).
 *
 * Fixed-size and SDK-free: no heap, no floating point, every read bounds-
 * checked against `len` (no MMU on the watch). The caller reads the whole file
 * into a buffer of its own (files are small: Workout::kMaxSteps is 20), so a
 * file bigger than the buffer is rejected by the caller, never truncated.
 ******************************************************************************
 */

#ifndef INTERVALS_WORKOUT_PARSER_HPP
#define INTERVALS_WORKOUT_PARSER_HPP

#include <cstddef>
#include <cstdint>

#include "HrZones.hpp"
#include "WorkoutTypes.hpp"
#include "WorkoutValidation.hpp"

namespace Intervals
{

/// File-format version this parser reads ("v" in the file).
constexpr uint8_t kWorkoutFileVersion = 1;

/// The largest file the app should read into its buffer. Twenty steps at
/// about 110 bytes each, plus the header, with headroom for whitespace.
constexpr size_t kMaxWorkoutFileBytes = 4096;

enum class ParseError : uint8_t {
    Ok,
    TooLarge,      ///< len > kMaxWorkoutFileBytes
    Syntax,        ///< not JSON of the expected shape, or cut short
    BadVersion,    ///< "v" is not kWorkoutFileVersion
    MissingField,  ///< a required key is absent
    BadValue,      ///< an unknown name, a number out of range, low > high
    TooManySteps,  ///< more than Workout::kMaxSteps
    NameTooLong,   ///< name does not fit Workout::kNameChars including its end
    Invalid,       ///< parsed, but validate() refused it; see `validation`
};

struct ParseResult {
    ParseError      error      = ParseError::Ok;
    ValidationError validation = ValidationError::Ok;  ///< set when error == Invalid
    /// Byte offset in the input where the parser stopped, for a log line
    /// (Syntax/BadValue/MissingField). 0 when Ok.
    size_t          offset     = 0;
};

/// Parses `buf[0..len)` into `out`. On any error `out` is left in an
/// unspecified state and must not be used. On Ok, validate() has passed, so
/// the workout meets WorkoutEngine::start()'s precondition.
///
/// Wire units are human ones and are converted here: time in seconds becomes
/// Step::durationValue in ms, distance in metres becomes cm. Pace is in
/// seconds per km with `low` the faster bound (the smaller number), matching
/// TargetEvaluator::classify. `low <= high` is required for every target kind.
ParseResult parseWorkout(const char* buf, size_t len, Workout& out);

} // namespace Intervals

#endif // INTERVALS_WORKOUT_PARSER_HPP
