/**
 ******************************************************************************
 * @file    AppConfigFields.hpp
 * @brief   The app's copy of the configuration contract in its app-manifest.json.
 * @details app-manifest.json never reaches the watch, so the app repeats the id,
 *          type, default and bounds of every field here. CI compares the two
 *          (validate_app_config.py --check-bounds) and fails the build if they
 *          disagree, which is what makes it safe for SDK::AppConfig to clamp a
 *          value it should never have received.
 *
 *          Keep the table in one file, one entry per line, with plain literals:
 *          that is the form the checker parses.
 *
 *          Height and weight are not here: the UNA profile already holds them
 *          and the kernel hands them to every app (RequestSystemSettings), so
 *          a second copy would drift (una-sdk Docs/app-config-fields.md 1.1).
 ******************************************************************************
 */

#ifndef APP_CONFIG_FIELDS_HPP
#define APP_CONFIG_FIELDS_HPP

#include "SDK/AppConfig/AppConfig.hpp"

#include <cstddef>

namespace WeighInConfig {

/// The values file the companion app writes, as declared by "configFile".
constexpr const char *kFileName = "app_config.json";

/// Field ids. Never rename one: a new id discards the user's value.
constexpr const char *kBirthYear  = "birthYear";
constexpr const char *kBirthMonth = "birthMonth";
constexpr const char *kSex        = "sex";

/// The declaration, mirroring app-manifest.json's "configFields".
extern const SDK::AppConfig::Field kFields[];
extern const size_t kFieldCount;

} // namespace WeighInConfig

#endif // APP_CONFIG_FIELDS_HPP
