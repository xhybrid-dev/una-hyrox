/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the Energy Probe's service and its GUI. Fixed
 *          size, each well under the kernel's 256-byte message.
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include <cstdint>

#include "NightSummary.hpp"
#include "StateFile.hpp"

#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type PROBE_STATUS = 0x00000001;
constexpr SDK::MessageType::Type PROBE_NIGHTS = 0x00000002;
constexpr SDK::MessageType::Type PROBE_CHECK  = 0x00000003;

// GUI --> Service
constexpr SDK::MessageType::Type SET_OVERRIDE = 0x00000010;
constexpr SDK::MessageType::Type SET_WINDOW   = 0x00000011;
constexpr SDK::MessageType::Type SET_DAY_MODE = 0x00000012;
constexpr SDK::MessageType::Type START_CHECK  = 0x00000013;

/// What the probe is doing now: plain data, so the GUI keeps a copy.
struct StatusData {
    int32_t  tonightDay     = 0;       ///< the night on now, or the next one
    int32_t  battPctX100    = -1;      ///< -1 unknown
    uint32_t rows           = 0;       ///< rows written since the service started
    uint32_t kbWritten      = 0;       ///< KB written since the service started
    uint32_t writeErrors    = 0;
    uint32_t uptimeS        = 0;
    uint32_t starts         = 0;       ///< service starts since install
    uint32_t groupsOn       = 0;       ///< EnergyProbe::Group bits connected now
    uint32_t groupsFailed   = 0;       ///< bits whose sensor would not connect
    uint8_t  modeNow        = 0;       ///< EnergyProbe::Mode
    uint8_t  night          = 0;       ///< 1 in a night window
    uint8_t  tonightMode    = 0;       ///< the mode tonight records in
    uint8_t  overrideMode   = 0;       ///< 0 auto, else 1 + Mode (for tonightDay)
    uint8_t  window         = 0;       ///< index into EnergyProbe::kWindows
    uint8_t  dayMode        = 1;       ///< Mode: Full or Off
    uint8_t  lastBpm        = 0;       ///< 0 none this row
    int8_t   charging       = -1;
    int8_t   touch          = -1;      ///< -1 unknown, 0 off the wrist, 1 on it
    uint8_t  checkRunning   = 0;
    uint8_t  timeValid      = 0;       ///< the watch's clock has a date
    uint8_t  valid          = 0;       ///< 1 once the service has sent one
};

/// Service --> GUI: the status. Sent each row, on a change, and when the GUI starts.
struct ProbeStatus : public SDK::MessageBase {
    StatusData d;
    ProbeStatus() : SDK::MessageBase(PROBE_STATUS) {}
};

/// The last nights, newest first: plain data.
struct NightsData {
    uint8_t                 count  = 0;
    uint8_t                 pad[3] = {};
    EnergyProbe::NightBrief nights[EnergyProbe::kHistory] {};
};

/// Service --> GUI: the last nights.
struct ProbeNights : public SDK::MessageBase {
    NightsData d;
    ProbeNights() : SDK::MessageBase(PROBE_NIGHTS) {}
};

/// Service --> GUI: up to five lines of the sensor check, or its progress. Five to a message,
/// because the GUI's queue holds only ten (Docs/writing-a-clockface.md, "The GUI stops draining").
struct ProbeCheck : public SDK::MessageBase {
    static constexpr uint8_t kPerMessage = 5;
    uint8_t first    = 0;      ///< the first line's number, 0 first
    uint8_t count    = 0;      ///< lines in this message
    uint8_t total    = 0;      ///< lines in all
    uint8_t secsLeft = 0;      ///< while it listens (count 0); 0 once the results are in
    char    text[kPerMessage][40] = {};
    ProbeCheck() : SDK::MessageBase(PROBE_CHECK) {}
};

/// GUI --> Service: tonight's override (0 auto, else 1 + Mode).
struct SetOverride : public SDK::MessageBase {
    uint8_t value = 0;
    SetOverride() : SDK::MessageBase(SET_OVERRIDE) {}
};

/// GUI --> Service: the night window (an index into EnergyProbe::kWindows).
struct SetWindow : public SDK::MessageBase {
    uint8_t value = 0;
    SetWindow() : SDK::MessageBase(SET_WINDOW) {}
};

/// GUI --> Service: the daytime mode (Mode::Full or Mode::Off).
struct SetDayMode : public SDK::MessageBase {
    uint8_t value = 1;
    SetDayMode() : SDK::MessageBase(SET_DAY_MODE) {}
};

/// GUI --> Service: run the sensor check.
struct StartCheck : public SDK::MessageBase {
    StartCheck() : SDK::MessageBase(START_CHECK) {}
};

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
