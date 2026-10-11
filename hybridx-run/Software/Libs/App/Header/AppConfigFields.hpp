/**
 ******************************************************************************
 * @file    AppConfigFields.hpp
 * @brief   The athlete's numbers the VO2max estimate needs, editable on the
 *          phone (or by hand over USB: Apps/HybridXRun/app_config.json).
 ******************************************************************************
 * Declared here in C++ and again in Resources/app-manifest.json. The two must
 * agree; CI checks that with
 *   validate_app_config.py --check Resources/app-manifest.json \
 *       --check-bounds Software/Libs/App/Sources/AppConfigFields.cpp
 * Keep the table one entry per line with plain literals: the form the checker
 * parses (una-sdk Docs/app-config-fields.md 5.1). 0 means "not set" in each.
 ******************************************************************************
 */

#ifndef RUN_APP_CONFIG_FIELDS_HPP
#define RUN_APP_CONFIG_FIELDS_HPP

#include <cstddef>

#include "SDK/AppConfig/AppConfig.hpp"

namespace RunConfig
{

/// The values file the companion app writes, as declared by "configFile".
constexpr const char* kFileName = "app_config.json";

constexpr const char* kBirthYear  = "birthYear";
constexpr const char* kBirthMonth = "birthMonth";
constexpr const char* kMaxHr      = "maxHr";
constexpr const char* kRestingHr  = "restingHr";

extern const SDK::AppConfig::Field kFields[];
extern const size_t kFieldCount;

} // namespace RunConfig

#endif // RUN_APP_CONFIG_FIELDS_HPP
