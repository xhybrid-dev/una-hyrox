/**
 ******************************************************************************
 * @file    WorkoutSettings.hpp
 * @brief   What the app remembers between workouts, as a small JSON file in
 *          its own folder. Service only (it uses the SDK's JSON reader and
 *          writer, which the GUI does not link).
 *
 * Version 2 (W1): {"v":2,"autolap":5,"type":1,"sets":3,"srest":60,"scap":300,
 * "emom":10,"amrap":12,"work":40,"rest":20,"rounds":6,"crounds":8,"run":1000}.
 * A version 1 file ({"v":1,"autolap":5}, W0) still reads: its auto lap is
 * kept and the plan takes the defaults. A missing key keeps its default, and
 * every value is pulled into its range (SessionPlan.hpp, clamp).
 ******************************************************************************
 */

#ifndef WORKOUT_WORKOUT_SETTINGS_HPP
#define WORKOUT_WORKOUT_SETTINGS_HPP

#include <cstddef>
#include <cstdint>

#include "SessionPlan.hpp"

namespace Workout {

inline constexpr const char* kSettingsPath = "settings.json";

struct WorkoutSettings {
    uint8_t    autoLapMin = 0;   ///< One of kAutoLapMinutes (0: off); a plain workout's
    PlanConfig plan;             ///< The kind of workout and its numbers

    /// A buffer big enough for any settings file.
    static constexpr size_t kMaxJsonBytes = 256;

    /// Write the file's text. Returns its length, or 0 (buf emptied) if it did not fit.
    size_t toJson(char* buf, size_t cap) const;
    /// Read it. False, and the defaults kept, if it is not version 1 or 2, or
    /// its auto lap is not one of the choices.
    bool fromJson(const char* text, size_t len);
};

} // namespace Workout

#endif // WORKOUT_WORKOUT_SETTINGS_HPP
