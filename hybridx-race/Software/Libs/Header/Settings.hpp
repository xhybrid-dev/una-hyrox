/**
 ******************************************************************************
 * @file    Settings.hpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   On-watch settings for HybridX Race.
 ******************************************************************************
 *
 * Adapted from the SDK's Running/RunLVGL example: the alert and interval
 * settings are gone, and what remains is the race configuration of brief 10.3.
 *
 * Three of these fields also live in AppConfig so the phone can edit them
 * (see AppConfigFields.hpp). AppConfig is the source of truth for those;
 * this struct is the in-memory working copy plus the last-used race format,
 * which brief 10.3 says to keep in the app's own settings file rather than
 * AppConfig.
 *
 ******************************************************************************
 */

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <cstdint>

#include "RaceData.hpp"

/**
 * @brief User settings.
 */
struct Settings
{
    /// Bumped when the on-disk shape changes; written and read, not yet branched on.
    static constexpr uint8_t kVersion = 1;

    /// Bounds for the split lockout, mirrored in the AppConfig field table.
    static constexpr uint8_t kLockoutMinSec = 1u;
    static constexpr uint8_t kLockoutMaxSec = 10u;
    static constexpr uint8_t kLockoutDefaultSec = 3u;  ///< Decision D8

    /// Bounds for the target finish time, in minutes. 0 turns targets off.
    static constexpr uint16_t kTargetFinishMaxMin = 240u;

    uint32_t version = kVersion;

    /// Last race format chosen on the watch. Not an AppConfig field (brief 10.3).
    Race::Format format = Race::Format::Full;

    /// Record Roxzone in and out as separate segments. Decision D3: off.
    bool roxzoneSplits = false;

    /// Ignore the split button for this many seconds after a split. Decision D8.
    uint8_t splitLockoutSec = kLockoutDefaultSec;

    /// Vibrate when a new segment starts (brief 8.3).
    bool vibrateOnSplit = true;

    /// Target race time in minutes; 0 = off. Hidden until F14 ships (D6).
    uint16_t targetFinishMin = 0u;

    /// @retval The split lockout in milliseconds, clamped to its declared range.
    uint32_t lockoutMs() const
    {
        uint8_t sec = splitLockoutSec;
        if (sec < kLockoutMinSec) {
            sec = kLockoutMinSec;
        }
        if (sec > kLockoutMaxSec) {
            sec = kLockoutMaxSec;
        }
        return static_cast<uint32_t>(sec) * 1000u;
    }
};

#endif  // SETTINGS_HPP
