/**
 ******************************************************************************
 * @file    AppConfigFields.cpp
 * @brief   The AppConfig field table.
 ******************************************************************************
 */

#include "AppConfigFields.hpp"

using SDK::AppConfig;

namespace TrainingConfig
{

// Every value here must match Resources/app-manifest.json exactly. CI checks it
// with validate_app_config.py --check <app-manifest.json> --check-bounds <this file>.
const AppConfig::Field kFields[] = {
    AppConfig::intField("plan", 0, 0, 3),
    AppConfig::stringField("fiveK", "", 0, 8),
    AppConfig::boolField("fiveKRecent", true),
    AppConfig::stringField("halfTime", "", 0, 8),
    AppConfig::stringField("raceDate", "", 0, 10),
    AppConfig::intField("level", 2, 1, 3),
    AppConfig::boolField("runMon", false),
    AppConfig::boolField("runTue", true),
    AppConfig::boolField("runWed", true),
    AppConfig::boolField("runThu", false),
    AppConfig::boolField("runFri", true),
    AppConfig::boolField("runSat", false),
    AppConfig::boolField("runSun", true),
    AppConfig::intField("longRunDay", 7, 1, 7),
    AppConfig::boolField("easyByHr", false),
};

const size_t kFieldCount = sizeof(kFields) / sizeof(kFields[0]);

}  // namespace TrainingConfig
