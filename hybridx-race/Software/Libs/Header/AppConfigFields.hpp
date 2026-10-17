/**
 ******************************************************************************
 * @file    AppConfigFields.hpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   The settings the companion phone app can edit (brief 10.3).
 ******************************************************************************
 *
 * Declared here in C++ and again in Output/app-manifest.json. The two must
 * agree; CI checks that with
 *
 *   validate_app_config.py --check <app-manifest.json> \
 *       --check-bounds Software/Libs/Sources/AppConfigFields.cpp \
 *       --check-bounds Software/Libs/Header/AppConfigFields.hpp
 *
 * Keep the table in one file, one entry per line, with plain literals: that is
 * the form the checker parses (Docs/Tutorials/Waypoint).
 *
 ******************************************************************************
 */

#ifndef APP_CONFIG_FIELDS_HPP
#define APP_CONFIG_FIELDS_HPP

#include <cstddef>

#include "SDK/AppConfig/AppConfig.hpp"

namespace RaceConfig
{

/// The values file the companion app writes, as declared by "configFile".
constexpr const char *kFileName = "app_config.json";

/// Field ids, so the service never spells one out twice.
constexpr const char *kRoxzoneSplits = "roxzoneSplits";
constexpr const char *kRunDistanceM = "runDistanceM";
constexpr const char *kSplitLockoutSec = "splitLockoutSec";
constexpr const char *kVibrateOnSplit = "vibrateOnSplit";
constexpr const char *kTargetFinishMin = "targetFinishMin";

/// The declaration, mirroring app-manifest.json's "configFields".
extern const SDK::AppConfig::Field kFields[];
extern const size_t kFieldCount;

}  // namespace RaceConfig

#endif  // APP_CONFIG_FIELDS_HPP
