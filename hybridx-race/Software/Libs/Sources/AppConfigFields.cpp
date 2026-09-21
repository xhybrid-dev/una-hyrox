/**
 ******************************************************************************
 * @file    AppConfigFields.cpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   The AppConfig field table.
 ******************************************************************************
 */

#include "AppConfigFields.hpp"

using SDK::AppConfig;

namespace RaceConfig
{

// Every value here must match Output/app-manifest.json exactly. CI checks it
// with validate_app_config.py --check <app-manifest.json> --check-bounds <this file>.
const AppConfig::Field kFields[] = {
    AppConfig::boolField("roxzoneSplits", false),
    AppConfig::intField("splitLockoutSec", 3, 1, 10),
    AppConfig::boolField("vibrateOnSplit", true),
    AppConfig::intField("targetFinishMin", 0, 0, 240),
};

const size_t kFieldCount = sizeof(kFields) / sizeof(kFields[0]);

}  // namespace RaceConfig
