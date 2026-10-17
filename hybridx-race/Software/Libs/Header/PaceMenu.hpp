/**
 ******************************************************************************
 * @file    PaceMenu.hpp
 * @date    15-10-2026
 * @author  HybridX
 * @brief   The wording and stepping of the pace plan's menu rows. Pure C++.
 ******************************************************************************
 *
 * Kept out of the screens so the rules can be host-tested: which mode a press
 * moves to, what the hint says (and why a mode cannot be used), and how the
 * target time steps on the watch.
 *
 * "Status" below is PacePlan::Status as a byte, as the service sends it in the
 * settings message. This file does not include PacePlan.hpp: the GUI process
 * does not link it, so the two values it needs are named here and checked
 * against the enum in the host tests.
 *
 ******************************************************************************
 */
#ifndef RACE_PACE_MENU_HPP
#define RACE_PACE_MENU_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "RaceData.hpp"

namespace Race
{
namespace PaceMenu
{

/// PacePlan::Status values this file reads. Checked against the enum in PaceMenuTest.
constexpr uint8_t kStatusReady = 1u;
constexpr uint8_t kStatusNoLastRace = 2u;
constexpr uint8_t kStatusNoMatch = 3u;
constexpr uint8_t kStatusNoTarget = 4u;

/// The on-watch target time: steps of five minutes, from here to the field's maximum. The phone
/// takes any minute.
constexpr uint16_t kTargetFirstMin = 60u;
constexpr uint16_t kTargetStepMin = 5u;
constexpr uint16_t kTargetMaxMin = 240u;

/**
 * @brief Whether a mode can be used.
 *
 * Off always can. The others need Ready: an unknown or zero status is not usable, so a settings
 * message the GUI has not received yet leaves them unselectable rather than offering a plan that
 * would not exist.
 */
constexpr bool usable(PaceMode mode, uint8_t status)
{
    return mode == PaceMode::Off || status == kStatusReady;
}

/**
 * @brief The mode a press of the row moves to: Off, Last race, Target, then Off again, skipping
 *        any that cannot be used.
 *
 * @param current The mode now.
 * @param status  Status for each mode, indexed by PaceMode.
 * @retval The next usable mode. Always usable, since Off always is.
 */
inline PaceMode nextMode(PaceMode current, const uint8_t (&status)[kPaceModeCount])
{
    for (uint8_t step = 1u; step <= kPaceModeCount; ++step) {
        const uint8_t i = static_cast<uint8_t>((static_cast<uint8_t>(current) + step) % kPaceModeCount);
        const PaceMode candidate = static_cast<PaceMode>(i);
        if (usable(candidate, status[i])) {
            return candidate;
        }
    }
    return PaceMode::Off;
}

/// A reason a mode cannot be used, short enough for the menu's hint line (13 characters).
constexpr const char *reason(uint8_t status)
{
    switch (status) {
    case kStatusNoLastRace: return "No saved race";
    case kStatusNoMatch:    return "Doesn't match";
    case kStatusNoTarget:   return "No target set";
    default:                return "Unavailable";
    }
}

/**
 * @brief The hint under "Pace plan": the mode, or why it cannot be used.
 *
 * @param buf       Destination; 16 bytes always hold the result.
 * @param size      Size of @p buf.
 * @param mode      The mode selected.
 * @param status    Its status.
 * @param targetMin The target finish in minutes, shown for Target.
 */
inline void tip(char *buf, size_t size, PaceMode mode, uint8_t status, uint16_t targetMin)
{
    if (buf == nullptr || size == 0u) {
        return;
    }
    if (mode == PaceMode::Off) {
        snprintf(buf, size, "Off");
    } else if (!usable(mode, status)) {
        snprintf(buf, size, "%s", reason(status));
    } else if (mode == PaceMode::LastRace) {
        snprintf(buf, size, "Last race");
    } else {
        snprintf(buf, size, "Target %u:%02u", static_cast<unsigned>(targetMin / 60u),
                 static_cast<unsigned>(targetMin % 60u));
    }
}

/**
 * @brief The next target time when the row is pressed: Off, then 60, 65 ... 240, then Off.
 *
 * A value set on the phone that is not a multiple of five moves up to the next multiple, so
 * a press always lands on a step.
 */
constexpr uint16_t nextTargetMin(uint16_t current)
{
    if (current == 0u) {
        return kTargetFirstMin;
    }
    if (current >= kTargetMaxMin) {
        return 0u;
    }
    const uint16_t next = static_cast<uint16_t>((current / kTargetStepMin + 1u) * kTargetStepMin);
    if (next < kTargetFirstMin) {
        return kTargetFirstMin;
    }
    return next > kTargetMaxMin ? kTargetMaxMin : next;
}

/// The target time as "1:45", or "Off".
inline void targetText(char *buf, size_t size, uint16_t targetMin)
{
    if (buf == nullptr || size == 0u) {
        return;
    }
    if (targetMin == 0u) {
        snprintf(buf, size, "Off");
    } else {
        snprintf(buf, size, "%u:%02u", static_cast<unsigned>(targetMin / 60u),
                 static_cast<unsigned>(targetMin % 60u));
    }
}

}  // namespace PaceMenu
}  // namespace Race

#endif  // RACE_PACE_MENU_HPP
