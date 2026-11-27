/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the Clock Probe's service and its GUI. Fixed size,
 *          each under the kernel's 256-byte message (asserted in Service.cpp).
 *
 * The service sends snapshots, never deltas: the GUI's queue holds ten messages
 * and is drained only while the face is on screen, so after a spell off screen
 * it sees the last ten sent (Docs/writing-a-clockface.md). The GUI asks for
 * everything again when it resumes.
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <cstdint>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include "ProbeView.hpp"

#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type PROBE_VIEW  = 0x00000001;
constexpr SDK::MessageType::Type PROBE_CLOCK = 0x00000002;

// GUI --> Service
constexpr SDK::MessageType::Type GUI_REFRESH = 0x00000010;
constexpr SDK::MessageType::Type GUI_REPORT  = 0x00000011;
constexpr SDK::MessageType::Type GUI_LIFE    = 0x00000012;

/// Service --> GUI: the ring, the trend and the seven rows, whole.
struct ProbeView : public SDK::MessageBase {
    ClockProbe::ViewData d;
    ProbeView() : SDK::MessageBase(PROBE_VIEW) {}
};

/// Service --> GUI: the time, once a minute and on request.
struct ProbeClock : public SDK::MessageBase {
    ClockProbe::ClockData d;
    ProbeClock() : SDK::MessageBase(PROBE_CLOCK) {}
};

/// GUI --> Service: "I am back on screen, send everything again."
struct GuiRefresh : public SDK::MessageBase {
    GuiRefresh() : SDK::MessageBase(GUI_REFRESH) {}
};

/// GUI --> Service: every 50 frames while the GUI is being ticked. Running totals since the GUI started.
struct GuiReport : public SDK::MessageBase {
    uint32_t frames   = 0;
    uint16_t resumes  = 0;
    uint16_t suspends = 0;
    GuiReport() : SDK::MessageBase(GUI_REPORT) {}
};

/// GUI --> Service: the kernel gave the display to this face (1) or took it away (2).
struct GuiLife : public SDK::MessageBase {
    uint8_t  kind   = 0;
    uint8_t  pad[3] = {};
    uint32_t frames = 0;
    GuiLife() : SDK::MessageBase(GUI_LIFE) {}
};

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
