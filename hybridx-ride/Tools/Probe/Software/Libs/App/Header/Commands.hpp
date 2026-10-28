/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the bike probe's service and its GUI.
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include <cstdint>

#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type PROBE_LINE  = 0x00000001;
constexpr SDK::MessageType::Type PROBE_STATE = 0x00000002;

// GUI --> Service
constexpr SDK::MessageType::Type NEXT_REQUEST = 0x00000010;

/// Service --> GUI: one more line for the screen's log.
struct ProbeLine : public SDK::MessageBase {
    char text[40] = {};
    ProbeLine() : SDK::MessageBase(PROBE_LINE) {}
};

/// Service --> GUI: the request now in force and the next one, and the live heart rate.
struct ProbeState : public SDK::MessageBase {
    uint8_t  step       = 0xFF;   ///< RideProbe::kSteps index in force; 0xFF = none yet
    uint8_t  next       = 0;      ///< the one R1 sends next
    uint8_t  hrSource   = 0;      ///< HeartRateEx::Source: 0 unknown, 1 optical, 2 external
    uint8_t  extBpm     = 0;      ///< the external sensor's bpm; 0 = none
    bool     logging    = false;  ///< the CSV is being written
    ProbeState() : SDK::MessageBase(PROBE_STATE) {}
};

/// GUI --> Service: release everything, then request the next kinds.
struct NextRequest : public SDK::MessageBase {
    NextRequest() : SDK::MessageBase(NEXT_REQUEST) {}
};

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
