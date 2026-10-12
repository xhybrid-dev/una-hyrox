/**
 ******************************************************************************
 * @file    AppConfigFields.cpp
 * @brief   The app's copy of the configuration contract in its app-manifest.json.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "AppConfigFields.hpp"

namespace WeighInConfig {

using SDK::AppConfig;

// Every value here must match Resources/app-manifest.json exactly. CI checks it
// with validate_app_config.py --check <app-manifest.json> --check-bounds <this file>.
// birthYear and birthMonth are HybridX Run's, ids and bounds alike. sex: 0 not
// set, 1 female, 2 male (config fields have no enum type).
const AppConfig::Field kFields[] = {
    AppConfig::intField("birthYear", 0, 0, 2020),
    AppConfig::intField("birthMonth", 0, 0, 12),
    AppConfig::intField("sex", 0, 0, 2),
};

const size_t kFieldCount = sizeof(kFields) / sizeof(kFields[0]);

} // namespace WeighInConfig
