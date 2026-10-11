/**
 ******************************************************************************
 * @file    AppConfigFields.cpp
 * @brief   The AppConfig field table.
 ******************************************************************************
 */

#include "AppConfigFields.hpp"

using SDK::AppConfig;

namespace RunConfig
{

// Every value here must match Resources/app-manifest.json exactly. CI checks it
// with validate_app_config.py --check <app-manifest.json> --check-bounds <this file>.
const AppConfig::Field kFields[] = {
    AppConfig::intField("birthYear", 0, 0, 2020),
    AppConfig::intField("birthMonth", 0, 0, 12),
    AppConfig::intField("maxHr", 0, 0, 230),
    AppConfig::intField("restingHr", 0, 0, 100),
};

const size_t kFieldCount = sizeof(kFields) / sizeof(kFields[0]);

}  // namespace RunConfig
