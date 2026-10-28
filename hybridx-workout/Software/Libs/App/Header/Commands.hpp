/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the HybridX Workout service and its GUI (from
 *          X Sauna's, with the strap, the settings and the Streak line).
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "WorkoutSnapshot.hpp"

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
constexpr SDK::MessageType::Type SETTINGS_UPDATE   = 0x00000005;
constexpr SDK::MessageType::Type ACCESSORY_STATUS  = 0x00000006;
constexpr SDK::MessageType::Type STREAK_LINE       = 0x00000007;

// GUI --> Service
constexpr SDK::MessageType::Type SESSION_START  = 0x00000010;
constexpr SDK::MessageType::Type SESSION_LAP    = 0x00000011;
constexpr SDK::MessageType::Type SESSION_PAUSE  = 0x00000012;
constexpr SDK::MessageType::Type SESSION_RESUME = 0x00000013;
constexpr SDK::MessageType::Type SESSION_STOP   = 0x00000014;
constexpr SDK::MessageType::Type SESSION_LAPS_REQUEST = 0x00000015;
constexpr SDK::MessageType::Type SET_AUTO_LAP   = 0x00000016;

/// Service --> GUI: the session as it now stands. Sent when the GUI starts, on
/// every change, and once a second while a session runs.
struct SessionUpdate : public SDK::MessageBase {
    Workout::SessionSnapshot snapshot {};

    SessionUpdate() : SDK::MessageBase(SESSION_UPDATE) {}
    explicit SessionUpdate(const Workout::SessionSnapshot &s) : SessionUpdate() { snapshot = s; }
};

/// Service --> GUI: the session ended; what it came to, and whether the
/// activity file was written.
struct SessionSaved : public SDK::MessageBase {
    Workout::SavedSession saved {};

    SessionSaved() : SDK::MessageBase(SESSION_SAVED) {}
    explicit SessionSaved(const Workout::SavedSession &s) : SessionSaved() { saved = s; }
};

/// Service --> GUI: a lap just ended (the lap toast).
struct SessionLapEvent : public SDK::MessageBase {
    Workout::LapEventData event {};

    SessionLapEvent() : SDK::MessageBase(SESSION_LAP_EVENT) {}
    explicit SessionLapEvent(const Workout::LapEventData &e) : SessionLapEvent() { event = e; }
};

/// Service --> GUI: up to kLapsPerPage laps of the workout, answering a LapsRequest.
struct SessionLapsPage : public SDK::MessageBase {
    Workout::LapsPageData page {};

    SessionLapsPage() : SDK::MessageBase(SESSION_LAPS_PAGE) {}
    explicit SessionLapsPage(const Workout::LapsPageData &p) : SessionLapsPage() { page = p; }
};

/// GUI --> Service: send the laps from @p first on (the summary's pages).
struct SessionLapsRequest : public SDK::MessageBase {
    uint8_t first = 0;

    SessionLapsRequest() : SDK::MessageBase(SESSION_LAPS_REQUEST) {}
    explicit SessionLapsRequest(uint8_t f) : SessionLapsRequest() { first = f; }
};

/// Service --> GUI: the remembered settings. Sent when the GUI starts and after a change.
struct SettingsUpdate : public SDK::MessageBase {
    uint8_t autoLapMin = 0;   ///< One of Workout::kAutoLapMinutes (0: off)

    SettingsUpdate() : SDK::MessageBase(SETTINGS_UPDATE) {}
    explicit SettingsUpdate(uint8_t m) : SettingsUpdate() { autoLapMin = m; }
};

/// Service --> GUI: the heart-rate strap's state, as the kernel reports it
/// (SDK::Accessory::State), with the strap's name when it has one.
struct AccessoryStatusUpd : public SDK::MessageBase {
    uint8_t state = 0;
    char    name[24] {};

    AccessoryStatusUpd() : SDK::MessageBase(ACCESSORY_STATUS) {}
};

/// Service --> GUI: the Streak line after a save (hybridx-link StreakNow): two lines of text;
/// show is false when there is none (HybridX Streak not installed, or never opened).
struct StreakLineUpd : public SDK::MessageBase {
    bool show      = false;
    char line1[24] = {};   ///< "3 of 4 this week"
    char line2[20] = {};   ///< "8-week streak", or empty

    StreakLineUpd() : SDK::MessageBase(STREAK_LINE) {}
};

/// GUI --> Service: begin a workout, with the remembered settings.
struct SessionStart : public SDK::MessageBase {
    SessionStart() : SDK::MessageBase(SESSION_START) {}
};

/// GUI --> Service: change the auto lap (minutes, one of Workout::kAutoLapMinutes) and remember it.
struct SetAutoLap : public SDK::MessageBase {
    uint8_t minutes = 0;

    SetAutoLap() : SDK::MessageBase(SET_AUTO_LAP) {}
    explicit SetAutoLap(uint8_t m) : SetAutoLap() { minutes = m; }
};

/// GUI --> Service: end this lap and begin the next.
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
static_assert(sizeof(AccessoryStatusUpd) <= 256u, "AccessoryStatusUpd exceeds the 256-byte kernel pool block");
static_assert(sizeof(StreakLineUpd) <= 256u, "StreakLineUpd exceeds the 256-byte kernel pool block");

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
