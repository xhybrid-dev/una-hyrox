/**
 ******************************************************************************
 * @file    AppConfigFields.hpp
 * @brief   The goal settings the companion phone app can edit (PLAN 7).
 ******************************************************************************
 * Declared here in C++ and again in Resources/app-manifest.json. The two must
 * agree; CI checks that with
 *   validate_app_config.py --check Resources/app-manifest.json \
 *       --check-bounds Software/Libs/App/Sources/AppConfigFields.cpp
 * Keep the table one entry per line with plain literals: the form the checker
 * parses (una-sdk Docs/app-config-fields.md 5.1). Settings made on the watch
 * are written back here too, as HybridX Race does.
 ******************************************************************************
 */

#ifndef STREAK_APP_CONFIG_FIELDS_HPP
#define STREAK_APP_CONFIG_FIELDS_HPP

#include <cstddef>

#include "SDK/AppConfig/AppConfig.hpp"

namespace StreakConfig
{

/// The values file the companion app writes, as declared by "configFile".
constexpr const char* kFileName = "app_config.json";

constexpr const char* kWeeklyTarget = "weeklyTarget";
constexpr const char* kWeekStart    = "weekStart";
constexpr const char* kCounts       = "counts";
constexpr const char* kMinMinutes   = "minMinutes";
constexpr const char* kOnePerDay    = "onePerDay";

/// "counts": 0 = every kind, otherwise Streak::Kind + 1.
constexpr int32_t kCountsAny = 0;

extern const SDK::AppConfig::Field kFields[];
extern const size_t kFieldCount;

} // namespace StreakConfig

#endif // STREAK_APP_CONFIG_FIELDS_HPP
