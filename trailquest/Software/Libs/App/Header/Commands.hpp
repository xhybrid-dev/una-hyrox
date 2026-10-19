/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the Trail Quest service and its GUI.
 *
 * Fixed-size structs of plain integers. Every message fits the kernel's
 * 256-byte pool block (una-sdk Docs/writing-a-clockface.md), checked at
 * compile time below: the simulator does not model the pools, so an oversized
 * message would work there and be dropped silently on the watch.
 *
 * The game runs in the service (Session); the GUI sends what the player did
 * and draws what comes back. After every command, and when the GUI starts,
 * the service answers with a whole World snapshot (and a Duel snapshot while
 * there is a duel to show, sent first). Snapshots, not changes, so a message
 * the GUI's ten-deep queue drops costs nothing but a stale frame. Each command
 * carries a number the World echoes, so a screen knows its answer arrived. A
 * Hello (the GUI asking again after a silence) has its own echo, so an answer
 * to the Hello without one to the command says the command never ran.
 *
 * The timing beat is judged in the GUI (Beat.hpp is header-only), against the
 * needle on screen, and sent as its result: no round trip before the player
 * sees "PERFECT".
 ******************************************************************************
 */

#ifndef TRAILQUEST_COMMANDS_HPP
#define TRAILQUEST_COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

#include "Feedback.hpp"
#include "View.hpp"

#include <cstddef>
#include <cstdint>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type WORLD = 0x00000001;
constexpr SDK::MessageType::Type DUEL  = 0x00000002;

// GUI --> Service
constexpr SDK::MessageType::Type HELLO        = 0x00000010;
constexpr SDK::MessageType::Type NEW_GAME     = 0x00000011;
constexpr SDK::MessageType::Type TRAVEL       = 0x00000012;
constexpr SDK::MessageType::Type OBSTACLE_TRY = 0x00000013;
constexpr SDK::MessageType::Type DUEL_TURN    = 0x00000014;
constexpr SDK::MessageType::Type JOIN         = 0x00000015;
constexpr SDK::MessageType::Type ADVANCE      = 0x00000016;
constexpr SDK::MessageType::Type SETTINGS     = 0x00000017;
constexpr SDK::MessageType::Type FX_PLAY      = 0x00000018;

/// Service --> GUI: the world now, and what the last command did.
struct World : public SDK::MessageBase {
    TrailQuest::WorldView view;
    uint16_t              ack;     ///< The number of the last command handled (not a Hello).
    uint16_t              hello;   ///< The number of the last Hello handled.
    uint8_t               beat;    ///< 1: the timing beat is on (the player's setting).

    World()
        : SDK::MessageBase(WORLD)
        , view {}
        , ack(0)
        , hello(0)
        , beat(1)
    {}

    World(const TrailQuest::WorldView& v, uint16_t a, uint16_t h, bool b)
        : World()
    {
        view  = v;
        ack   = a;
        hello = h;
        beat  = b ? 1u : 0u;
    }
};

/// Service --> GUI: the duel now, with the last turn's events to play back.
struct Duel : public SDK::MessageBase {
    TrailQuest::DuelView view;

    Duel()
        : SDK::MessageBase(DUEL)
        , view {}
    {}

    explicit Duel(const TrailQuest::DuelView& v)
        : Duel()
    {
        view = v;
    }
};

/// The part every GUI --> Service command shares: its number, echoed in World.
struct Command : public SDK::MessageBase {
    uint16_t seq;

    explicit Command(SDK::MessageType::Type t, uint16_t s = 0)
        : SDK::MessageBase(t)
        , seq(s)
    {}
};

/// GUI --> Service: send the world (the GUI has just started, or lost track).
struct Hello : public Command {
    Hello() : Command(HELLO) {}
    explicit Hello(uint16_t s) : Command(HELLO, s) {}
};

/// GUI --> Service: start a new game (over any save).
struct NewGame : public Command {
    NewGame() : Command(NEW_GAME) {}
    explicit NewGame(uint16_t s) : Command(NEW_GAME, s) {}
};

/// GUI --> Service: take this exit from where the runner stands.
struct Travel : public Command {
    uint8_t exit;

    Travel() : Command(TRAVEL), exit(0) {}
    Travel(uint16_t s, uint8_t e) : Command(TRAVEL, s), exit(e) {}
};

/// GUI --> Service: attempt the obstacle, with the beat's result.
struct ObstacleTry : public Command {
    uint8_t beat;   ///< TrailQuest::BeatResult.

    ObstacleTry() : Command(OBSTACLE_TRY), beat(0) {}
    ObstacleTry(uint16_t s, TrailQuest::BeatResult b) : Command(OBSTACLE_TRY, s), beat(static_cast<uint8_t>(b)) {}
};

/// GUI --> Service: one turn of the duel.
struct DuelTurn : public Command {
    uint8_t action;   ///< TrailQuest::Action.
    uint8_t item;     ///< For Fuel: the item (TrailQuest::ItemId).
    uint8_t beat;     ///< TrailQuest::BeatResult.

    DuelTurn() : Command(DUEL_TURN), action(0), item(TrailQuest::kNone), beat(1) {}
    DuelTurn(uint16_t s, TrailQuest::Action a, TrailQuest::ItemId i, TrailQuest::BeatResult b)
        : Command(DUEL_TURN, s)
        , action(static_cast<uint8_t>(a))
        , item(i)
        , beat(static_cast<uint8_t>(b))
    {}
};

/// GUI --> Service: the answer to an offer to join.
struct Join : public Command {
    uint8_t accept;
    uint8_t slot;     ///< With a full pack: who leaves (else TrailQuest::kNone).

    Join() : Command(JOIN), accept(0), slot(TrailQuest::kNone) {}
    Join(uint16_t s, bool a, uint8_t sl) : Command(JOIN, s), accept(a ? 1u : 0u), slot(sl) {}
};

/// GUI --> Service: on from a stage clear (or the summit).
struct Advance : public Command {
    Advance() : Command(ADVANCE) {}
    explicit Advance(uint16_t s) : Command(ADVANCE, s) {}
};

/// GUI --> Service: the player's settings changed.
struct Settings : public Command {
    uint8_t beat;

    Settings() : Command(SETTINGS), beat(1) {}
    Settings(uint16_t s, bool b) : Command(SETTINGS, s), beat(b ? 1u : 0u) {}
};

/// GUI --> Service: play this feedback pattern now (timed by the GUI to what
/// is on screen).
struct FxPlay : public SDK::MessageBase {
    uint8_t fx;   ///< TrailQuest::FxId.

    FxPlay()
        : SDK::MessageBase(FX_PLAY)
        , fx(0)
    {}

    explicit FxPlay(TrailQuest::FxId id)
        : FxPlay()
    {
        fx = static_cast<uint8_t>(id);
    }
};

constexpr size_t kMaxMessageBytes = 256u;
#define TRAILQUEST_ASSERT_FITS_POOL(T) \
    static_assert(sizeof(T) <= kMaxMessageBytes, #T " exceeds the 256-byte kernel pool block")
TRAILQUEST_ASSERT_FITS_POOL(World);
TRAILQUEST_ASSERT_FITS_POOL(Duel);
TRAILQUEST_ASSERT_FITS_POOL(Hello);
TRAILQUEST_ASSERT_FITS_POOL(NewGame);
TRAILQUEST_ASSERT_FITS_POOL(Travel);
TRAILQUEST_ASSERT_FITS_POOL(ObstacleTry);
TRAILQUEST_ASSERT_FITS_POOL(DuelTurn);
TRAILQUEST_ASSERT_FITS_POOL(Join);
TRAILQUEST_ASSERT_FITS_POOL(Advance);
TRAILQUEST_ASSERT_FITS_POOL(Settings);
TRAILQUEST_ASSERT_FITS_POOL(FxPlay);

} // namespace CustomMessage

#pragma pack(pop)

#endif // TRAILQUEST_COMMANDS_HPP
