/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the Trail Quest service and its GUI.
 *
 * Fixed-size structs of plain integers. Every message fits the kernel's
 * 256-byte pool block (una-sdk Docs/writing-a-clockface.md), checked at
 * compile time below.
 *
 * Q0 (the feasibility probe) uses three: the GUI reports a button press on
 * the timing beat and the service judges it (the real design: the core lives
 * in the service); the GUI asks for a feedback pattern to be played.
 *
 * A beat press carries everything the judgement needs, as the player saw it:
 * the zones the GUI drew, the start of the pass, and the needle's time in the
 * frame on screen. Both times are the GUI's lv_tick_get(), so the service
 * never shares a clock with the GUI, and the zones on screen can never differ
 * from the zones judged. (From Q1 the core picks the zones and sends them in
 * the view; the press still echoes what was drawn.)
 ******************************************************************************
 */

#ifndef TRAILQUEST_COMMANDS_HPP
#define TRAILQUEST_COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

#include "Beat.hpp"
#include "Feedback.hpp"

#include <cstddef>
#include <cstdint>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type BEAT_RESULT = 0x00000001;

// GUI --> Service
constexpr SDK::MessageType::Type BEAT_PRESS = 0x00000010;
constexpr SDK::MessageType::Type FX_PLAY    = 0x00000011;

/// GUI --> Service: the button was pressed on the beat. Both times are the
/// GUI's lv_tick_get().
struct BeatPress : public SDK::MessageBase {
    uint32_t seq;        ///< Echoed in the result, to measure the round trip.
    uint32_t startMs;    ///< When the sweep began.
    uint32_t pressMs;    ///< The needle's time in the frame on screen at the press.
    TrailQuest::BeatConfig beat;   ///< The zones as drawn.

    BeatPress()
        : SDK::MessageBase(BEAT_PRESS)
        , seq(0)
        , startMs(0)
        , pressMs(0)
        , beat {}
    {}

    BeatPress(uint32_t s, uint32_t start, uint32_t press, const TrailQuest::BeatConfig& b)
        : BeatPress()
    {
        seq     = s;
        startMs = start;
        pressMs = press;
        beat    = b;
    }
};

/// Service --> GUI: how the press was judged.
struct BeatResult : public SDK::MessageBase {
    uint32_t seq;
    int32_t  offsetMs;   ///< Press minus target: negative early, positive late.
    uint8_t  result;     ///< TrailQuest::BeatResult.

    BeatResult()
        : SDK::MessageBase(BEAT_RESULT)
        , seq(0)
        , offsetMs(0)
        , result(0)
    {}

    BeatResult(uint32_t s, int32_t off, TrailQuest::BeatResult r)
        : BeatResult()
    {
        seq      = s;
        offsetMs = off;
        result   = static_cast<uint8_t>(r);
    }
};

/// GUI --> Service: play this feedback pattern now.
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
TRAILQUEST_ASSERT_FITS_POOL(BeatPress);
TRAILQUEST_ASSERT_FITS_POOL(BeatResult);
TRAILQUEST_ASSERT_FITS_POOL(FxPlay);

} // namespace CustomMessage

#pragma pack(pop)

#endif // TRAILQUEST_COMMANDS_HPP
