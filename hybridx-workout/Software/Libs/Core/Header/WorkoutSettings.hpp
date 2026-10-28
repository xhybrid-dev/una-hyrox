/**
 ******************************************************************************
 * @file    WorkoutSettings.hpp
 * @brief   What the app remembers between workouts, as a small JSON file in
 *          its own folder: {"v":1,"autolap":5}. Service only (it uses the
 *          SDK's JSON reader and writer, which the GUI does not link).
 ******************************************************************************
 */

#ifndef WORKOUT_WORKOUT_SETTINGS_HPP
#define WORKOUT_WORKOUT_SETTINGS_HPP

#include <cstddef>
#include <cstdint>

namespace Workout {

inline constexpr const char* kSettingsPath = "settings.json";

struct WorkoutSettings {
    uint8_t autoLapMin = 0;   ///< One of kAutoLapMinutes (0: off)

    /// A buffer big enough for any settings file.
    static constexpr size_t kMaxJsonBytes = 64;

    /// Write the file's text. Returns its length, or 0 (buf emptied) if it did not fit.
    size_t toJson(char* buf, size_t cap) const;
    /// Read it. False, and the defaults kept, if it is not version 1 with a known auto lap.
    bool fromJson(const char* text, size_t len);
};

} // namespace Workout

#endif // WORKOUT_WORKOUT_SETTINGS_HPP
