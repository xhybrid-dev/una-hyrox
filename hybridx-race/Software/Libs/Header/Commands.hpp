/**
 ******************************************************************************
 * @file    Commands.hpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   Typed messages between the Service and the GUI process.
 ******************************************************************************
 *
 * Follows the pattern RunLVGL uses -- fixed-hex-ID structs deriving from
 * SDK::MessageBase -- with brief 9.3's message set.
 *
 * Two rules the SDK's own activity apps do not enforce, and we do:
 *
 * 1. **Every struct is static_assert'd against the 256-byte pool block.**
 *    Docs/writing-a-clockface.md:310: a larger allocation returns nullptr and
 *    the send is dropped *silently*. The simulator allocates with new[], so it
 *    will never show you the failure. This has to be a compile-time check.
 *
 * 2. **IDs are split into two ranges**, service->GUI below 0x80 and
 *    GUI->service at 0x80 and up. RunLVGL interleaves the two directions in
 *    one run of values, which makes an accidental collision easy when the set
 *    grows.
 *
 * The summary is the one thing that cannot fit a message: 31 segments is
 * 744 bytes. RunLVGL sends a raw pointer into service memory, which works only
 * because the two processes share an address space and leaves the GUI holding
 * a pointer whose lifetime it does not control. We page it instead --
 * SUMMARY_META once, then SUMMARY_PAGE per handful of segments.
 *
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include "RaceData.hpp"
#include "Settings.hpp"
#include "Track.hpp"

// Force 4-byte alignment for all message structures, as the SDK examples do.
#pragma pack(push, 4)

namespace CustomMessage
{

// -- Service -> GUI ------------------------------------------------------------
constexpr SDK::MessageType::Type SETTINGS_UPDATE   = 0x00000001;
constexpr SDK::MessageType::Type LOCAL_TIME        = 0x00000002;
constexpr SDK::MessageType::Type BATTERY           = 0x00000003;
constexpr SDK::MessageType::Type HR_UPDATE         = 0x00000004;
constexpr SDK::MessageType::Type RACE_STATE_UPDATE = 0x00000005;
constexpr SDK::MessageType::Type RACE_DATA_UPDATE  = 0x00000006;
constexpr SDK::MessageType::Type SPLIT_EVENT       = 0x00000007;
constexpr SDK::MessageType::Type RACE_FINISHED     = 0x00000008;
constexpr SDK::MessageType::Type SUMMARY_META      = 0x00000009;
constexpr SDK::MessageType::Type SUMMARY_PAGE      = 0x0000000A;
constexpr SDK::MessageType::Type ACCESSORY_STATUS  = 0x0000000B;

// -- GUI -> Service ------------------------------------------------------------
constexpr SDK::MessageType::Type SETTINGS_SAVE     = 0x00000080;
constexpr SDK::MessageType::Type RACE_START        = 0x00000081;
constexpr SDK::MessageType::Type RACE_SPLIT        = 0x00000082;
constexpr SDK::MessageType::Type RACE_UNDO_SPLIT   = 0x00000083;
constexpr SDK::MessageType::Type RACE_PAUSE        = 0x00000084;
constexpr SDK::MessageType::Type RACE_RESUME       = 0x00000085;
constexpr SDK::MessageType::Type RACE_FINISH_EARLY = 0x00000086;
constexpr SDK::MessageType::Type RACE_UNDO_FINISH  = 0x00000087;
constexpr SDK::MessageType::Type RACE_SAVE         = 0x00000088;
constexpr SDK::MessageType::Type RACE_DISCARD      = 0x00000089;
constexpr SDK::MessageType::Type SUMMARY_REQUEST   = 0x0000008A;

// =============================================================================
// Service -> GUI
// =============================================================================

/// Heart-rate zone boundaries, as the system profile reports them.
constexpr uint8_t kHrThresholdsCount = 6u;
constexpr uint8_t kHrThresholdsDefault[kHrThresholdsCount] = { 95, 114, 133, 152, 171, 190 };

/**
 * @brief The settings the service is working from, plus the system context the
 *        GUI needs to render.
 *
 * Units, clock format and heart-rate zones come from the system profile rather
 * than from us, but the GUI has no route to them, so the service forwards them
 * here -- the same shape the SDK's activity apps use.
 */
struct SettingsUpd : public SDK::MessageBase
{
    Settings settings;
    bool     isImperial = false;
    bool     is12HourFormat = false;
    uint8_t  hrThresholds[kHrThresholdsCount] {};
    uint8_t  hrThresholdsCount = 0u;

    SettingsUpd() : SDK::MessageBase(SETTINGS_UPDATE), settings {} {}
    explicit SettingsUpd(const Settings &s) : SettingsUpd() { settings = s; }
};

/// Wall-clock time of day, for the status face.
struct LocalTime : public SDK::MessageBase
{
    uint8_t hour = 0u;
    uint8_t minute = 0u;
    uint8_t second = 0u;
    uint8_t month = 0u;
    uint8_t day = 0u;
    uint8_t weekday = 0u;

    LocalTime() : SDK::MessageBase(LOCAL_TIME) {}
};

/// Battery percentage.
struct Battery : public SDK::MessageBase
{
    uint8_t level = 0u;

    Battery() : SDK::MessageBase(BATTERY) {}
    explicit Battery(uint8_t l) : Battery() { level = l; }
};

/// Heart rate outside a race, so the sensor status row can show a reading.
struct HrUpdate : public SDK::MessageBase
{
    uint8_t bpm = 0u;
    uint8_t source = 0u;  ///< 0 none, 1 optical, 2 external

    HrUpdate() : SDK::MessageBase(HR_UPDATE) {}
    HrUpdate(uint8_t b, uint8_t s) : HrUpdate()
    {
        bpm = b;
        source = s;
    }
};

/// The race has changed state.
struct RaceStateUpd : public SDK::MessageBase
{
    Track::State state = Track::State::INACTIVE;

    RaceStateUpd() : SDK::MessageBase(RACE_STATE_UPDATE) {}
    explicit RaceStateUpd(Track::State s) : RaceStateUpd() { state = s; }
};

/// The 1 Hz workhorse: where we are and how long it has taken.
struct RaceDataUpd : public SDK::MessageBase
{
    Track::Data data {};

    RaceDataUpd() : SDK::MessageBase(RACE_DATA_UPDATE) {}
    explicit RaceDataUpd(const Track::Data &d) : RaceDataUpd() { data = d; }
};

/// A segment just closed. Drives the toast and the haptics (brief 8.3).
struct SplitEvent : public SDK::MessageBase
{
    Track::SplitEvent split {};

    SplitEvent() : SDK::MessageBase(CustomMessage::SPLIT_EVENT) {}
    explicit SplitEvent(const Track::SplitEvent &s) : SplitEvent() { split = s; }
};

/// The race is over and the timer has stopped.
struct RaceFinished : public SDK::MessageBase
{
    bool completed = false;  ///< False when the race was ended early

    RaceFinished() : SDK::MessageBase(RACE_FINISHED) {}
    explicit RaceFinished(bool c) : RaceFinished() { completed = c; }
};

/// Overview of a finished race. Sent before any SUMMARY_PAGE.
struct SummaryMeta : public SDK::MessageBase
{
    Race::Format format = Race::Format::Full;
    bool     roxzone = false;
    bool     completed = false;
    uint8_t  segmentCount = 0u;   ///< How many SUMMARY_PAGE entries to expect
    uint32_t totalMs = 0u;
    uint32_t runsMs = 0u;
    uint32_t stationsMs = 0u;
    uint32_t roxzoneMs = 0u;
    uint8_t  hrAvg = 0u;
    uint8_t  hrMax = 0u;

    SummaryMeta() : SDK::MessageBase(SUMMARY_META) {}
};

/**
 * @brief A page of finished segments.
 *
 * Paged rather than sent whole because 31 segments do not fit a 256-byte pool
 * block. Eight entries a page keeps this message comfortably inside it and
 * means a full race is four pages.
 */
struct SummaryPage : public SDK::MessageBase
{
    static constexpr uint8_t kEntriesPerPage = 8u;

    struct Entry
    {
        Race::SegmentDesc desc {};
        uint32_t activeMs = 0u;
        uint8_t  hrAvg = 0u;
        uint8_t  hrMax = 0u;
    };

    uint8_t firstIndex = 0u;  ///< Index of entries[0] in the whole race
    uint8_t count = 0u;       ///< Valid entries, 1 to kEntriesPerPage
    Entry   entries[kEntriesPerPage] {};

    SummaryPage() : SDK::MessageBase(SUMMARY_PAGE) {}
};

/// External heart-rate strap link status.
struct AccessoryStatusUpd : public SDK::MessageBase
{
    uint8_t state = 0u;   ///< SDK::Accessory::State
    char    name[24] {};  ///< Device name, may be empty

    AccessoryStatusUpd() : SDK::MessageBase(ACCESSORY_STATUS) {}
};

// =============================================================================
// GUI -> Service
// =============================================================================

/// Persist edited settings.
struct SettingsSave : public SDK::MessageBase
{
    Settings settings;

    SettingsSave() : SDK::MessageBase(SETTINGS_SAVE), settings {} {}
    explicit SettingsSave(const Settings &s) : SettingsSave() { settings = s; }
};

/// Start a race in the given format.
struct RaceStart : public SDK::MessageBase
{
    Race::Format format = Race::Format::Full;

    RaceStart() : SDK::MessageBase(RACE_START) {}
    explicit RaceStart(Race::Format f) : RaceStart() { format = f; }
};

/**
 * @brief Split, carrying the instant the button went down.
 *
 * The GUI stamps the press rather than letting the service read its own clock,
 * so up to 100 ms of tick latency cannot bias the split (brief 7.4).
 */
struct RaceSplit : public SDK::MessageBase
{
    uint32_t pressMs = 0u;

    RaceSplit() : SDK::MessageBase(RACE_SPLIT) {}
    explicit RaceSplit(uint32_t ms) : RaceSplit() { pressMs = ms; }
};

/// Signals with no payload.
#define HYBRIDX_SIGNAL_MESSAGE(Name, Id)                     \
    struct Name : public SDK::MessageBase                    \
    {                                                        \
        Name() : SDK::MessageBase(Id) {}                     \
    }

HYBRIDX_SIGNAL_MESSAGE(RaceUndoSplit, RACE_UNDO_SPLIT);
HYBRIDX_SIGNAL_MESSAGE(RacePause, RACE_PAUSE);
HYBRIDX_SIGNAL_MESSAGE(RaceResume, RACE_RESUME);
HYBRIDX_SIGNAL_MESSAGE(RaceFinishEarly, RACE_FINISH_EARLY);
HYBRIDX_SIGNAL_MESSAGE(RaceUndoFinish, RACE_UNDO_FINISH);
HYBRIDX_SIGNAL_MESSAGE(RaceSave, RACE_SAVE);
HYBRIDX_SIGNAL_MESSAGE(RaceDiscard, RACE_DISCARD);
HYBRIDX_SIGNAL_MESSAGE(SummaryRequest, SUMMARY_REQUEST);

#undef HYBRIDX_SIGNAL_MESSAGE

// =============================================================================
// The guard the SDK's activity apps are missing
// =============================================================================

/// Largest block the kernel's message pools offer.
constexpr size_t kMaxMessageBytes = 256u;

#define HYBRIDX_ASSERT_FITS_POOL(T)                                        \
    static_assert(sizeof(T) <= kMaxMessageBytes,                           \
                  #T " exceeds the 256-byte kernel pool block: the send "  \
                     "would fail silently on the watch")

HYBRIDX_ASSERT_FITS_POOL(SettingsUpd);
HYBRIDX_ASSERT_FITS_POOL(LocalTime);
HYBRIDX_ASSERT_FITS_POOL(Battery);
HYBRIDX_ASSERT_FITS_POOL(HrUpdate);
HYBRIDX_ASSERT_FITS_POOL(RaceStateUpd);
HYBRIDX_ASSERT_FITS_POOL(RaceDataUpd);
HYBRIDX_ASSERT_FITS_POOL(SplitEvent);
HYBRIDX_ASSERT_FITS_POOL(RaceFinished);
HYBRIDX_ASSERT_FITS_POOL(SummaryMeta);
HYBRIDX_ASSERT_FITS_POOL(SummaryPage);
HYBRIDX_ASSERT_FITS_POOL(AccessoryStatusUpd);
HYBRIDX_ASSERT_FITS_POOL(SettingsSave);
HYBRIDX_ASSERT_FITS_POOL(RaceStart);
HYBRIDX_ASSERT_FITS_POOL(RaceSplit);
HYBRIDX_ASSERT_FITS_POOL(RaceUndoSplit);
HYBRIDX_ASSERT_FITS_POOL(SummaryRequest);

#undef HYBRIDX_ASSERT_FITS_POOL

}  // namespace CustomMessage

#pragma pack(pop)

#endif  // COMMANDS_HPP
