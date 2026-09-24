/**
 ******************************************************************************
 * @file    ProbeMessages.hpp
 * @brief   The Streak Probe's one Service -> GUI message.
 ******************************************************************************
 */

#ifndef STREAK_PROBE_MESSAGES_HPP
#define STREAK_PROBE_MESSAGES_HPP

#include <cstddef>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include "ProbeResult.hpp"

#pragma pack(push, 4)

namespace CustomMessage
{

/// App-private ID (MessageTypes.hpp): the kernel never interprets it.
constexpr SDK::MessageType::Type PROBE_RESULT = 0x00000001;

struct ProbeResult : public SDK::MessageBase {
    Probe::Result result {};
    ProbeResult() : SDK::MessageBase(PROBE_RESULT) {}
    explicit ProbeResult(const Probe::Result& r) : ProbeResult() { result = r; }
};

// An oversized message fails silently on the watch (hybridx-race NOTES 0.9).
static_assert(sizeof(ProbeResult) <= 256u, "ProbeResult exceeds the 256-byte kernel pool block");

} // namespace CustomMessage

#pragma pack(pop)

#endif // STREAK_PROBE_MESSAGES_HPP
