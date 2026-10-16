/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the Knitting service and its GUI.
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "RowCounter.hpp"

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include <cstdint>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type COUNTER_UPDATE = 0x00000001;

// GUI --> Service
constexpr SDK::MessageType::Type COUNT_ROW = 0x00000010;
constexpr SDK::MessageType::Type NEXT_SET  = 0x00000011;
constexpr SDK::MessageType::Type RESET_ALL = 0x00000012;
constexpr SDK::MessageType::Type UNDO_ROW  = 0x00000013;

/**
 * @brief   Service --> GUI: the counter as it now stands.
 *
 * Sent once when the GUI starts, then after every change. The counter lives
 * in the service, so it outlasts a screen that is closed and reopened.
 */
struct CounterUpdate : public SDK::MessageBase {
    uint32_t rows;      ///< Rows in the current set.
    uint32_t set;       ///< The current set, from 1.
    uint32_t total;     ///< Rows in all sets.

    CounterUpdate()
        : SDK::MessageBase(COUNTER_UPDATE)
        , rows(0)
        , set(1)
        , total(0)
    {}
};

/// GUI --> Service: one more row.
struct CountRow : public SDK::MessageBase {
    CountRow() : SDK::MessageBase(COUNT_ROW) {}
};

/// GUI --> Service: start the next set.
struct NextSet : public SDK::MessageBase {
    NextSet() : SDK::MessageBase(NEXT_SET) {}
};

/// GUI --> Service: take back the last row (a hold on R1).
struct UndoRow : public SDK::MessageBase {
    UndoRow() : SDK::MessageBase(UNDO_ROW) {}
};

/// GUI --> Service: back to set 1 with no rows (a hold on L2).
struct ResetAll : public SDK::MessageBase {
    ResetAll() : SDK::MessageBase(RESET_ALL) {}
};

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
