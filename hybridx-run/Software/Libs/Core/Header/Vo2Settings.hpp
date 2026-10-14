/**
 ******************************************************************************
 * @file    Vo2Settings.hpp
 * @brief   The athlete's numbers as the watch's pickers edit them: ranges and
 *          steps (R5).
 *
 * The same values the config fields hold (AppConfigFields.cpp: 0 = not set /
 * automatic). The pickers step through:
 *   birth month 1-12 and birth year kMinBirthYear-kMaxBirthYear (a picker
 *     cannot unset a birth date; an unset one starts at January 1985);
 *   max HR: 0 (auto) then kMinMaxHr-kMaxMaxHr; stepping up from auto starts
 *     at the max HR in use, so the athlete adjusts rather than scrolls;
 *   resting HR: 0 (from the watch) then 30-100, likewise.
 ******************************************************************************
 */

#ifndef RUN_VO2_SETTINGS_HPP
#define RUN_VO2_SETTINGS_HPP

#include <cstdint>

#include "Vo2Config.hpp"

namespace RunVo2::Settings
{

inline constexpr uint16_t kMinBirthYear     = 1930;
inline constexpr uint16_t kMaxBirthYear     = 2015;
inline constexpr uint16_t kDefaultBirthYear = 1985;
inline constexpr uint8_t  kMinMaxHr         = 120;
inline constexpr uint8_t  kMaxMaxHr         = Config::kMaxHrBpm;
inline constexpr uint8_t  kMinRestHr        = Config::kMinRestingHr;
inline constexpr uint8_t  kMaxRestHr        = Config::kMaxRestingHr;

struct Values {
    uint16_t birthYear  = 0;      ///< 0 = not set
    uint8_t  birthMonth = 0;      ///< 0 = not set
    uint8_t  maxHr      = 0;      ///< 0 = auto
    uint8_t  restHr     = 0;      ///< 0 = from the watch
    bool     tuningLog  = true;
};

/// A birth date the picker can show: an unset or out-of-range one becomes
/// January kDefaultBirthYear.
void seedBirth(uint16_t& year, uint8_t& month);

/// One step (dir +1 or -1), clamped at the ends.
uint8_t  stepMonth(uint8_t month, int dir);
uint16_t stepYear(uint16_t year, int dir);

/// One step of max HR: 0 (auto) sits below kMinMaxHr; up from 0 goes to
/// @p inUse clamped into range.
uint8_t stepMaxHr(uint8_t value, int dir, uint8_t inUse);

/// One step of resting HR, as stepMaxHr with 0 = from the watch.
uint8_t stepRestHr(uint8_t value, int dir, uint8_t inUse);

}  // namespace RunVo2::Settings

#endif  // RUN_VO2_SETTINGS_HPP
