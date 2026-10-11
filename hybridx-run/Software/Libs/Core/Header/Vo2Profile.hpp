/**
 ******************************************************************************
 * @file    Vo2Profile.hpp
 * @brief   The athlete's numbers the estimate needs: age, max HR, resting HR.
 *
 * Max HR: the athlete's own value if entered; otherwise Tanaka's
 * 208 - 0.7 x age (Tanaka, Monahan and Seals 2001), raised to the highest
 * sustained reading seen on a run if that is higher (auto max, used only
 * when no value was entered). Resting HR: the athlete's own, else the
 * watch's daily figure.
 ******************************************************************************
 */

#ifndef RUN_VO2_PROFILE_HPP
#define RUN_VO2_PROFILE_HPP

#include <cstdint>
#include <ctime>

namespace RunVo2
{

/// What the athlete entered (0 = not set) and what the watch knows.
struct ProfileInput {
    uint16_t birthYear     = 0;   ///< e.g. 1985; 0 = not set
    uint8_t  birthMonth    = 0;   ///< 1-12; 0 = not set (taken as January)
    uint8_t  enteredMaxHr  = 0;   ///< bpm; 0 = calculate
    uint8_t  enteredRestHr = 0;   ///< bpm; 0 = use the watch's
    uint8_t  watchRestHr   = 0;   ///< bpm from the daily metrics; 0 = none
    uint8_t  autoMaxHr     = 0;   ///< highest sustained HR seen; 0 = none
};

enum class MaxHrSource : uint8_t { None, Entered, Formula, Observed };

enum class ProfileStatus : uint8_t {
    Ok,
    NeedsAge,          ///< no birth year and no max HR entered
    NeedsRestingHr,    ///< none entered and none from the watch
    ReserveTooSmall,   ///< max HR not clearly above resting HR
};

struct Profile {
    ProfileStatus status    = ProfileStatus::NeedsAge;
    uint8_t       ageYears  = 0;   ///< 0 when unknown
    uint8_t       maxHr     = 0;
    uint8_t       restHr    = 0;
    MaxHrSource   maxSource = MaxHrSource::None;
};

/// Whole years of age at nowUtc. 0 if the birth year is unset or in the future.
uint8_t ageAt(uint16_t birthYear, uint8_t birthMonth, std::time_t nowUtc);

/// Tanaka's age-predicted max HR, rounded to the nearest bpm.
uint8_t tanakaMaxHr(uint8_t ageYears);

/// Resolve the profile the estimate uses.
Profile resolveProfile(const ProfileInput& in, std::time_t nowUtc);

}  // namespace RunVo2

#endif  // RUN_VO2_PROFILE_HPP
