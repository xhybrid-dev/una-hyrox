
#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <cstring>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

// Application types
#include "Settings.hpp"
#include "Track.hpp"
#include "ActivitySummary.hpp"

// HybridX Trail: the route core's types
#include "Navigator.hpp"

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

    // Kernel HR configuration — shared defaults used before first kernel update
    static constexpr uint8_t kHrThresholdsCount                       = 6;
    static constexpr uint8_t kHrThresholdsDefault[kHrThresholdsCount] = { 95, 114, 133, 152, 171, 190 };

    // Application custom commands
    // Service --> GUI
    constexpr SDK::MessageType::Type SETTINGS_UPDATE    = 0x00000001;
    constexpr SDK::MessageType::Type LOCAL_TIME         = 0x00000002;
    constexpr SDK::MessageType::Type BATTERY            = 0x00000003;
    constexpr SDK::MessageType::Type GPS_FIX            = 0x00000004;
    constexpr SDK::MessageType::Type TRACK_STATE_UPDATE = 0x00000005;
    constexpr SDK::MessageType::Type TRACK_DATA_UPDATE  = 0x00000006;
    constexpr SDK::MessageType::Type LAP_END                  = 0x00000007;
    constexpr SDK::MessageType::Type SUMMARY                  = 0x00000008;
    constexpr SDK::MessageType::Type INTERVALS_PHASE_ALERT      = 0x00000009;
    constexpr SDK::MessageType::Type INTERVALS_WORKOUT_COMPLETED = 0x00000010;
    constexpr SDK::MessageType::Type ACCESSORY_STATUS          = 0x00000012;

    // GUI --> Service
    constexpr SDK::MessageType::Type SETTINGS_SAVE         = 0x0000000A;
    constexpr SDK::MessageType::Type TRACK_START           = 0x0000000B;
    constexpr SDK::MessageType::Type TRACK_STOP            = 0x0000000C;
    constexpr SDK::MessageType::Type TRACK_PAUSE           = 0x0000000D;
    constexpr SDK::MessageType::Type TRACK_RESUME          = 0x0000000E;
    constexpr SDK::MessageType::Type MANUAL_LAP            = 0x0000000F;
    constexpr SDK::MessageType::Type INTERVALS_NEXT_PHASE  = 0x00000011;

    // HybridX Trail: routes and navigation.
    // Service --> GUI
    constexpr SDK::MessageType::Type ROUTE_LIST            = 0x00000020;
    constexpr SDK::MessageType::Type ROUTE_LOADED          = 0x00000021;
    constexpr SDK::MessageType::Type NAV_UPDATE            = 0x00000022;
    constexpr SDK::MessageType::Type NAV_ALERT             = 0x00000023;
    // GUI --> Service
    constexpr SDK::MessageType::Type ROUTE_SELECT          = 0x00000024;
    constexpr SDK::MessageType::Type ROUTE_RESCAN          = 0x00000025;

    // Service <-> GUI
    struct SettingsUpd : public SDK::MessageBase {
        // Application settings
        Settings settings;

        // Kernel settings
        bool    unitsImperial;
        bool    timeFormat12h;   // true = 12-hour clock, false = 24-hour
        uint8_t hrThresholds[kHrThresholdsCount];
        uint8_t hrThresholdsCount;

        SettingsUpd()
            : SDK::MessageBase(SETTINGS_UPDATE)
            , unitsImperial(false)
            , timeFormat12h(false)
            , hrThresholds {}
            , hrThresholdsCount(0)
        {}

        explicit SettingsUpd(Settings settings, bool units, bool timeFormat12h,
                         const uint8_t (&thresholds)[kHrThresholdsCount], uint8_t thresholdCount)
            : SettingsUpd()
        {
            this->settings          = settings;
            this->unitsImperial     = units;
            this->timeFormat12h     = timeFormat12h;
            memcpy(this->hrThresholds, thresholds, sizeof(this->hrThresholds));
            this->hrThresholdsCount = thresholdCount;
        }
    };

    // Service --> GUI
    struct Time : public SDK::MessageBase {
        std::tm localTime;
        Time()
            : SDK::MessageBase(LOCAL_TIME)
            , localTime {}
        {}

        explicit Time(std::tm localTime)
            : Time()
        {
            this->localTime = localTime;
        }
    };

    struct Battery : public SDK::MessageBase {
        uint8_t level;
        Battery()
            : SDK::MessageBase(BATTERY)
            , level(0)
        {}

        explicit Battery(uint8_t level)
            : Battery()
        {
            this->level = level;
        }
    };

    struct GpsFix : public SDK::MessageBase {
        bool state;
        GpsFix()
            : SDK::MessageBase(GPS_FIX)
            , state(false)
        {}

        explicit GpsFix(bool state)
            : GpsFix()
        {
            this->state = state;
        }
    };

    struct TrackStateUpd : public SDK::MessageBase {
        Track::State state;
        TrackStateUpd()
            : SDK::MessageBase(TRACK_STATE_UPDATE)
            , state{}
        {}

        explicit TrackStateUpd(Track::State state)
            : TrackStateUpd()
        {
            this->state = state;
        }
    };

    struct TrackDataUpd : public SDK::MessageBase {
        Track::Data data;
        TrackDataUpd()
            : SDK::MessageBase(TRACK_DATA_UPDATE)
            , data{}
        {}

        explicit TrackDataUpd(const Track::Data &data)
            : TrackDataUpd()
        {
            this->data = data;
        }
    };

    struct LapEnded : public SDK::MessageBase {
        uint32_t lapNum;
        LapEnded()
            : SDK::MessageBase(LAP_END)
            , lapNum(0)
        {}

        explicit LapEnded(uint32_t lapNum)
            : LapEnded()
        {
            this->lapNum = lapNum;
        }
    };

    struct Summary : public SDK::MessageBase {
        const ActivitySummary* summary; ///< Non-owning pointer; receiver must copy before releaseMessage
        Summary()
            : SDK::MessageBase(SUMMARY)
            , summary(nullptr)
        {}

        explicit Summary(const ActivitySummary* summaryPtr)
            : Summary()
        {
            this->summary = summaryPtr;
        }
    };

    struct IntervalsPhaseAlert : public SDK::MessageBase {
        Track::IntervalsData intervals; ///< Snapshot of the NEW phase — already set before this message is sent
        IntervalsPhaseAlert() : SDK::MessageBase(INTERVALS_PHASE_ALERT) {}

        explicit IntervalsPhaseAlert(const Track::IntervalsData& intervals)
            : IntervalsPhaseAlert()
        {
            this->intervals = intervals;
        }
    };

    struct IntervalsWorkoutCompleted : public SDK::MessageBase {
        IntervalsWorkoutCompleted() : SDK::MessageBase(INTERVALS_WORKOUT_COMPLETED) {}
    };

    // External-accessory link status forwarded from the kernel's
    // EVENT_ACCESSORY_STATUS, for the pre-activity HR indicator (WP-S4).
    struct AccessoryStatusUpd : public SDK::MessageBase {
        uint8_t state;     ///< SDK::Accessory::State
        char    name[24];  ///< device name (may be empty)
        AccessoryStatusUpd()
            : SDK::MessageBase(ACCESSORY_STATUS)
            , state(0)
            , name{}
        {}

        explicit AccessoryStatusUpd(uint8_t state, const char* name)
            : AccessoryStatusUpd()
        {
            this->state = state;
            if (name) {
                strncpy(this->name, name, sizeof(this->name) - 1);
            }
        }
    };

    // GUI --> Service
    struct SettingsSave : public SDK::MessageBase {
        // Application settings
        Settings settings;

        SettingsSave()
            : SDK::MessageBase(SETTINGS_SAVE)
        {}

        explicit SettingsSave(Settings settings)
            : SettingsSave()
        {
            this->settings = settings;
        }
    };

    struct TrackStart : public SDK::MessageBase {
        bool intervalsMode = false;
        TrackStart() : SDK::MessageBase(TRACK_START) {}

        explicit TrackStart(bool intervalsMode)
            : TrackStart()
        {
            this->intervalsMode = intervalsMode;
        }
    };

    struct IntervalsNextPhase : public SDK::MessageBase {
        IntervalsNextPhase() : SDK::MessageBase(INTERVALS_NEXT_PHASE) {}
    };

    struct TrackStop : public SDK::MessageBase {
        bool discard;   // If true, tarck will be discarded, otherwise saved
        TrackStop()
            : SDK::MessageBase(TRACK_STOP)
            , discard(false)
        {}

        explicit TrackStop(bool discard)
            : TrackStop()
        {
            this->discard = discard;
        }
    };

    struct TrackPause : public SDK::MessageBase {
        TrackPause() : SDK::MessageBase(TRACK_PAUSE) {}
    };

    struct TrackResume : public SDK::MessageBase {
        TrackResume() : SDK::MessageBase(TRACK_RESUME) {}
    };

    struct ManualLap : public SDK::MessageBase {
        ManualLap() : SDK::MessageBase(MANUAL_LAP) {}
    };


    // -- HybridX Trail ---------------------------------------------------------
    //
    // The route list and the route itself are too big for a message, so they
    // travel as pointers into the service's static storage, as RunLVGL's own
    // Summary does: the watch has no MMU, and the service and GUI of one app
    // share its memory. The GUI copies them before releasing the message, and
    // the service only rewrites them on the GUI's own request (ROUTE_SELECT,
    // ROUTE_RESCAN), never during an activity.

    struct RouteList : public SDK::MessageBase {
        const Trail::RouteInfo* routes   = nullptr;   ///< non-owning; copy before releaseMessage
        uint8_t                 count    = 0;
        int8_t                  selected = -1;        ///< -1: no route (a plain run)
        RouteList() : SDK::MessageBase(ROUTE_LIST) {}
        RouteList(const Trail::RouteInfo* r, uint8_t n, int8_t sel) : RouteList()
        {
            routes   = r;
            count    = n;
            selected = sel;
        }
    };

    struct RouteLoaded : public SDK::MessageBase {
        const Trail::GeoPoint* points = nullptr;      ///< non-owning; copy before releaseMessage
        uint16_t               count  = 0;            ///< 0: no route
        Trail::RouteInfo       info {};
        RouteLoaded() : SDK::MessageBase(ROUTE_LOADED) {}
        RouteLoaded(const Trail::GeoPoint* p, uint16_t n, const Trail::RouteInfo& i) : RouteLoaded()
        {
            points = p;
            count  = n;
            info   = i;
        }
    };

    struct NavUpdate : public SDK::MessageBase {
        Trail::Navigator::Status status {};
        NavUpdate() : SDK::MessageBase(NAV_UPDATE) {}
        explicit NavUpdate(const Trail::Navigator::Status& s) : NavUpdate() { status = s; }
    };

    struct NavAlert : public SDK::MessageBase {
        Trail::OffCourse::Event event = Trail::OffCourse::Event::None;
        NavAlert() : SDK::MessageBase(NAV_ALERT) {}
        explicit NavAlert(Trail::OffCourse::Event e) : NavAlert() { event = e; }
    };

    struct RouteSelect : public SDK::MessageBase {
        int8_t index = -1;                            ///< into the last RouteList; -1 for none
        RouteSelect() : SDK::MessageBase(ROUTE_SELECT) {}
        explicit RouteSelect(int8_t i) : RouteSelect() { index = i; }
    };

    struct RouteRescan : public SDK::MessageBase {
        RouteRescan() : SDK::MessageBase(ROUTE_RESCAN) {}
    };

    // An oversized message fails silently on the watch (hybridx-race NOTES 0.9).
    static_assert(sizeof(RouteLoaded) <= 256u, "RouteLoaded exceeds the 256-byte kernel pool block");
    static_assert(sizeof(NavUpdate) <= 256u, "NavUpdate exceeds the 256-byte kernel pool block");

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
