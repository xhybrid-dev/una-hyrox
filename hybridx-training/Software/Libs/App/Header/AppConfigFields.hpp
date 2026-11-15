/**
 ******************************************************************************
 * @file    AppConfigFields.hpp
 * @brief   X Training's settings from the phone (brief section 5.1), editable
 *          in the UNA app or by hand over USB: Apps/HybridXTraining/app_config.json.
 ******************************************************************************
 * Declared here in C++ and again in Resources/app-manifest.json. The two must
 * agree; CI checks that with
 *   validate_app_config.py --check Resources/app-manifest.json \
 *       --check-bounds Software/Libs/App/Sources/AppConfigFields.cpp
 * Keep the table one entry per line with plain literals: the form the checker
 * parses (una-sdk Docs/app-config-fields.md 5.1).
 *
 * The watch reads every string again strictly (Core's TimeText.hpp): the
 * phone's patterns are not enforced on the watch.
 ******************************************************************************
 */

#ifndef TRAINING_APP_CONFIG_FIELDS_HPP
#define TRAINING_APP_CONFIG_FIELDS_HPP

#include <cstddef>

#include "SDK/AppConfig/AppConfig.hpp"

namespace TrainingConfig
{

/// The values file the companion app writes, as declared by "configFile".
constexpr const char* kFileName = "app_config.json";

constexpr const char* kPlan        = "plan";         ///< 0 none, 1 10K, 2 half, 3 VO2max
constexpr const char* kFiveK       = "fiveK";        ///< "mm:ss", "" = not set
constexpr const char* kFiveKRecent = "fiveKRecent";
constexpr const char* kHalfTime    = "halfTime";     ///< "h:mm:ss", "" = none
constexpr const char* kRaceDate    = "raceDate";     ///< "YYYY-MM-DD", "" = none
constexpr const char* kLevel       = "level";        ///< 1-3
/// The seven run days, Monday first (index = weekday 0..6).
constexpr const char* kRunDay[7] = {"runMon", "runTue", "runWed", "runThu", "runFri", "runSat", "runSun"};
constexpr const char* kLongRunDay  = "longRunDay";   ///< 1 Monday .. 7 Sunday
constexpr const char* kEasyByHr    = "easyByHr";

/// Buffer sizes for the strings, terminator included.
constexpr size_t kFiveKBytes = 9;
constexpr size_t kHalfBytes  = 9;
constexpr size_t kDateBytes  = 11;

extern const SDK::AppConfig::Field kFields[];
extern const size_t kFieldCount;

} // namespace TrainingConfig

#endif // TRAINING_APP_CONFIG_FIELDS_HPP
