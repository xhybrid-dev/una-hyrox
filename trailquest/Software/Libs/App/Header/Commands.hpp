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
 * in the service); the GUI asks for a feedback pattern to be played. The
 * timestamps in a beat press are both the GUI's own clock, so the service
 * never has to share a clock with the GUI.
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
/// GUI's millisecond clock.
struct BeatPress : public SDK::MessageBase {
    uint32_t seq;        ///< Echoed in the result, to measure the round trip.
    uint32_t startMs;    ///< When this pass of the sweep began.
    uint32_t pressMs;    ///< When the button code reached the screen.

    BeatPress()
        : SDK::MessageBase(BEAT_PRESS)
        , seq(0)
        , startMs(0)
        , pressMs(0)
    {}

    BeatPress(uint32_t s, uint32_t start, uint32_t press)
        : BeatPress()
    {
        seq     = s;
        startMs = start;
        pressMs = press;
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
