/**
 ******************************************************************************
 * @file    AppConfigFields.cpp
 * @brief   The AppConfig field table (see the header).
 ******************************************************************************
 */

#include "AppConfigFields.hpp"

using SDK::AppConfig;

namespace StreakConfig
{

// Every value here must match Resources/app-manifest.json exactly.
const AppConfig::Field kFields[] = {
    AppConfig::intField("weeklyTarget", 3, 1, 7),
    AppConfig::intField("weekStart", 1, 0, 6),
    AppConfig::intField("counts", 0, 0, 8),
    AppConfig::intField("minMinutes", 10, 0, 120),
    AppConfig::boolField("onePerDay", false),
};

const size_t kFieldCount = sizeof(kFields) / sizeof(kFields[0]);

} // namespace StreakConfig
