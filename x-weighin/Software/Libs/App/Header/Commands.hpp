/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the X Weigh-in service and its GUI.
 *
 * Fixed-size structs of plain integers, so the GUI formats without floating
 * point. Every message fits the kernel's 256-byte pool block
 * (una-sdk Docs/writing-a-clockface.md), checked at compile time below.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

#include "Window.hpp"

#include <cstddef>
#include <cstdint>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type HOME_STATE  = 0x00000001;
constexpr SDK::MessageType::Type GRAPH_DATA  = 0x00000002;
constexpr SDK::MessageType::Type SAVE_RESULT = 0x00000003;

// GUI --> Service
constexpr SDK::MessageType::Type SAVE_WEIGHIN = 0x00000010;

/**
 * @brief   Everything the Home, Body and weigh-in screens show, as plain data
 *          (a MessageBase is pool-allocated and non-copyable, so the GUI keeps
 *          a copy of this, not of the message).
 */
struct HomeData {
    int32_t  latest;         ///< The last day's weigh-in, 0.01 kg; 0 = no log yet.
    int32_t  trend;          ///< The trend at the last day, 0.01 kg.
    int32_t  weekChange;     ///< Trend change over 7 days, 0.01 kg (if hasWeekChange).
    int32_t  lastDay;        ///< Local day of the last weigh-in.
    int32_t  today;          ///< Local day now; 0 if the clock is not set.
    int32_t  entryStart;     ///< Where the weigh-in screen opens, 0.01 kg.
    int32_t  bmiTenths;      ///< From the trend and the profile height; 0 = unknown.
    int32_t  bmrKcal;        ///< Mifflin-St Jeor from the trend; 0 = unknown.
    uint32_t heightCm;       ///< The UNA profile's height; 0 = not set.
    uint16_t days;           ///< Days with a weigh-in.
    uint16_t rejected;       ///< Log lines that did not parse (shown nowhere yet; logged).
    uint8_t  ageYears;       ///< From the phone settings; 0 = unknown.
    uint8_t  sex;            ///< Config field: 0 not set, 1 female, 2 male.
    uint8_t  entryFrom;      ///< EntryFrom: what entryStart came from.
    bool     hasWeekChange;
    bool     clockSet;
};

/// Where the weigh-in screen's starting value came from.
enum class EntryFrom : uint8_t {
    Default = 0,   ///< No log and no profile weight: XWeighIn::kEntryFallback.
    Log,           ///< The last logged weigh-in.
    Profile,       ///< The UNA profile's weight (before the first weigh-in).
};

/// Service --> GUI: the home state. Sent when the GUI starts and after a save.
struct HomeState : public SDK::MessageBase {
    HomeData home;

    HomeState()
        : SDK::MessageBase(HOME_STATE)
        , home {}
    {}
};

/// Service --> GUI: the last 30 days, for the graph and the Recent page.
struct GraphData : public SDK::MessageBase {
    XWeighIn::Window window;

    GraphData()
        : SDK::MessageBase(GRAPH_DATA)
        , window {}
    {}
};

/// How a SAVE_WEIGHIN ended.
enum class SaveOutcome : uint8_t {
    Saved = 0,      ///< In the log (and, normally, its SharedData copy).
    ClockNotSet,    ///< No date to file it under: sync the watch with the phone.
    WriteFailed,    ///< The log could not be written.
    Invalid,        ///< The weight was out of range.
};

/// Service --> GUI: the outcome of a SAVE_WEIGHIN.
struct SaveResult : public SDK::MessageBase {
    SaveOutcome outcome;
    int32_t     hundredths;

    SaveResult()
        : SDK::MessageBase(SAVE_RESULT)
        , outcome(SaveOutcome::WriteFailed)
        , hundredths(0)
    {}

    SaveResult(SaveOutcome o, int32_t h)
        : SaveResult()
    {
        outcome    = o;
        hundredths = h;
    }
};

/// GUI --> Service: log this weigh-in now.
struct SaveWeighIn : public SDK::MessageBase {
    int32_t hundredths;

    SaveWeighIn()
        : SDK::MessageBase(SAVE_WEIGHIN)
        , hundredths(0)
    {}

    explicit SaveWeighIn(int32_t h)
        : SaveWeighIn()
    {
        hundredths = h;
    }
};

constexpr size_t kMaxMessageBytes = 256u;
#define WEIGHIN_ASSERT_FITS_POOL(T) \
    static_assert(sizeof(T) <= kMaxMessageBytes, #T " exceeds the 256-byte kernel pool block")
WEIGHIN_ASSERT_FITS_POOL(HomeState);
WEIGHIN_ASSERT_FITS_POOL(GraphData);
WEIGHIN_ASSERT_FITS_POOL(SaveResult);
WEIGHIN_ASSERT_FITS_POOL(SaveWeighIn);

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
