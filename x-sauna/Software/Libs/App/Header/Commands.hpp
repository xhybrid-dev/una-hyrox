/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the X Sauna service and its GUI.
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SessionSnapshot.hpp"

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include <cstdint>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type SESSION_UPDATE = 0x00000001;
constexpr SDK::MessageType::Type SESSION_SAVED  = 0x00000002;
constexpr SDK::MessageType::Type SESSION_LAP_EVENT = 0x00000003;
constexpr SDK::MessageType::Type SESSION_LAPS_PAGE = 0x00000004;

// GUI --> Service
constexpr SDK::MessageType::Type SESSION_START  = 0x00000010;
constexpr SDK::MessageType::Type SESSION_LAP    = 0x00000011;
constexpr SDK::MessageType::Type SESSION_PAUSE  = 0x00000012;
constexpr SDK::MessageType::Type SESSION_RESUME = 0x00000013;
constexpr SDK::MessageType::Type SESSION_STOP   = 0x00000014;
constexpr SDK::MessageType::Type SESSION_LAPS_REQUEST = 0x00000015;

/// Service --> GUI: the session as it now stands. Sent when the GUI starts, on
/// every change, and once a second while a session runs.
struct SessionUpdate : public SDK::MessageBase {
    Sauna::SessionSnapshot snapshot {};

    SessionUpdate() : SDK::MessageBase(SESSION_UPDATE) {}
    explicit SessionUpdate(const Sauna::SessionSnapshot &s) : SessionUpdate() { snapshot = s; }
};

/// Service --> GUI: the session ended; what it came to, and whether the
/// activity file was written.
struct SessionSaved : public SDK::MessageBase {
    Sauna::SavedSession saved {};

    SessionSaved() : SDK::MessageBase(SESSION_SAVED) {}
    explicit SessionSaved(const Sauna::SavedSession &s) : SessionSaved() { saved = s; }
};

/// Service --> GUI: a lap just ended (the lap toast).
struct SessionLapEvent : public SDK::MessageBase {
    Sauna::LapEventData event {};

    SessionLapEvent() : SDK::MessageBase(SESSION_LAP_EVENT) {}
    explicit SessionLapEvent(const Sauna::LapEventData &e) : SessionLapEvent() { event = e; }
};

/// Service --> GUI: up to four laps of the session, answering a LapsRequest.
struct SessionLapsPage : public SDK::MessageBase {
    Sauna::LapsPageData page {};

    SessionLapsPage() : SDK::MessageBase(SESSION_LAPS_PAGE) {}
    explicit SessionLapsPage(const Sauna::LapsPageData &p) : SessionLapsPage() { page = p; }
};

/// GUI --> Service: send the laps from @p first on (the summary's pages).
struct SessionLapsRequest : public SDK::MessageBase {
    uint8_t first = 0;

    SessionLapsRequest() : SDK::MessageBase(SESSION_LAPS_REQUEST) {}
    explicit SessionLapsRequest(uint8_t f) : SessionLapsRequest() { first = f; }
};

/// GUI --> Service: begin a session.
struct SessionStart : public SDK::MessageBase {
    uint8_t mode = 0;     ///< Sauna::Mode: 0 sauna + plunge, 1 just sauna
    uint8_t rest = 0;     ///< Sauna + plunge only: include a Rest phase

    SessionStart() : SDK::MessageBase(SESSION_START) {}
    SessionStart(uint8_t m, uint8_t r) : SessionStart() { mode = m; rest = r; }
};

/// GUI --> Service: swap to the next phase.
struct SessionLap : public SDK::MessageBase {
    SessionLap() : SDK::MessageBase(SESSION_LAP) {}
};

struct SessionPause : public SDK::MessageBase {
    SessionPause() : SDK::MessageBase(SESSION_PAUSE) {}
};

struct SessionResume : public SDK::MessageBase {
    SessionResume() : SDK::MessageBase(SESSION_RESUME) {}
};

/// GUI --> Service: end the session, keeping it (save = 1) or throwing it away.
struct SessionStop : public SDK::MessageBase {
    uint8_t save = 1;

    SessionStop() : SDK::MessageBase(SESSION_STOP) {}
    explicit SessionStop(uint8_t s) : SessionStop() { save = s; }
};

// An oversized message fails silently on the watch (hybridx-race NOTES 0.9).
static_assert(sizeof(SessionUpdate) <= 256u, "SessionUpdate exceeds the 256-byte kernel pool block");
static_assert(sizeof(SessionSaved) <= 256u, "SessionSaved exceeds the 256-byte kernel pool block");
static_assert(sizeof(SessionLapEvent) <= 256u, "SessionLapEvent exceeds the 256-byte kernel pool block");
static_assert(sizeof(SessionLapsPage) <= 256u, "SessionLapsPage exceeds the 256-byte kernel pool block");

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
