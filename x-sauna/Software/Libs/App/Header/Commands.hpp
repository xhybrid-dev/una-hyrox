/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the X Sauna service and its GUI.
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "ProbeSnapshot.hpp"

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include <cstdint>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type PROBE_UPDATE = 0x00000001;

// GUI --> Service
constexpr SDK::MessageType::Type PROBE_MARK        = 0x00000010;
constexpr SDK::MessageType::Type PROBE_CLEAR_RANGE = 0x00000011;

/**
 * @brief   Service --> GUI: the probe's readings as they now stand.
 *
 * Sent once when the GUI starts, then once a second while the sensors run.
 */
struct ProbeUpdate : public SDK::MessageBase {
    Sauna::ProbeSnapshot snapshot {};

    ProbeUpdate() : SDK::MessageBase(PROBE_UPDATE) {}
    explicit ProbeUpdate(const Sauna::ProbeSnapshot &s) : ProbeUpdate() { snapshot = s; }
};

// An oversized message fails silently on the watch (hybridx-race NOTES 0.9).
static_assert(sizeof(ProbeUpdate) <= 256u, "ProbeUpdate exceeds the 256-byte kernel pool block");

/// GUI --> Service: R1, a marker in the log ("I have just gone in").
struct ProbeMark : public SDK::MessageBase {
    ProbeMark() : SDK::MessageBase(PROBE_MARK) {}
};

/// GUI --> Service: L1, start the lowest and highest readings again.
struct ProbeClearRange : public SDK::MessageBase {
    ProbeClearRange() : SDK::MessageBase(PROBE_CLEAR_RANGE) {}
};

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
