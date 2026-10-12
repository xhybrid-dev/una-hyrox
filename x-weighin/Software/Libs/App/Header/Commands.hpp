/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the X Weigh-in service and its GUI.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

#include <cstdint>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type FIRST_LOOK = 0x00000001;

/**
 * @brief   W0's first look: what the watch knows about the athlete.
 *
 * Plain integers only, so the GUI formats without floating point. Kept as
 * plain data rather than as message fields: a MessageBase is pool-allocated
 * and non-copyable, so the GUI could not keep a copy to redraw from.
 */
struct FirstLookState {
    uint32_t heightCm;          ///< The UNA profile's height; 0 = not set.
    int32_t  weightHundredths;  ///< The profile's weight in 0.01 kg; 0 = not set.
    uint8_t  restHr;            ///< The watch's daily resting HR, bpm; 0 = none yet.
    uint8_t  ageYears;          ///< From the birth year and month; 0 = unknown.
    uint8_t  sex;               ///< Config field: 0 not set, 1 female, 2 male.
    bool     settingsRead;      ///< The kernel answered RequestSystemSettings.
    bool     configLoaded;      ///< app_config.json was found and parsed.
    bool     clockSet;          ///< The clock reads 2026 or later.
};

/// Service --> GUI: a new first look. Sent when the GUI starts, and again
/// whenever the resting HR changes.
struct FirstLook : public SDK::MessageBase {
    FirstLookState state;

    FirstLook()
        : SDK::MessageBase(FIRST_LOOK)
        , state {}
    {}
};

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
