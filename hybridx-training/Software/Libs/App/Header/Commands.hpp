
#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <cstring>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Workout/WorkoutLibrary.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

// Application types
#include "Settings.hpp"
#include "Track.hpp"
#include "ActivitySummary.hpp"
#include "RunStats.hpp"   // HybridX Run: RunVo2::RunStatsResult (plain data)

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
    constexpr SDK::MessageType::Type WORKOUT_LIST              = 0x00000013;
    constexpr SDK::MessageType::Type WORKOUT_DETAILS           = 0x00000014;
    constexpr SDK::MessageType::Type WORKOUT_DATA              = 0x00000015;
    constexpr SDK::MessageType::Type WORKOUT_STEP              = 0x00000016;
    // HybridX Run: our own IDs, after UNA's workout block (0x13-0x18).
    constexpr SDK::MessageType::Type VO2_UPDATE                = 0x00000019;   // HybridX Run
    constexpr SDK::MessageType::Type VO2_PROFILE_SAVE          = 0x0000001A;   // HybridX Run, GUI -> Service
    constexpr SDK::MessageType::Type RUN_STATS_UPDATE          = 0x0000001B;   // HybridX Run: the run detail page
    constexpr SDK::MessageType::Type TREADMILL_ASK             = 0x0000001C;   // HybridX Run: the treadmill's distance, please
    constexpr SDK::MessageType::Type BESTS_UPDATE              = 0x0000001E;   // HybridX Run: personal bests
    constexpr SDK::MessageType::Type STREAK_LINE               = 0x0000001F;   // the Streak line after a save
    // X Training: our own, after X Run's (0x19-0x1F; VO2_UPDATE, VO2_PROFILE_SAVE and
    // BESTS_UPDATE are X Run's and never sent here).
    constexpr SDK::MessageType::Type PLAN_TODAY                = 0x00000020;   // X Training: today's session
    constexpr SDK::MessageType::Type PLAN_DAY                  = 0x00000021;   // X Training: a day of This week
    constexpr SDK::MessageType::Type PLAN_WEEK                 = 0x00000022;   // X Training: This week
    constexpr SDK::MessageType::Type PLAN_OVERVIEW             = 0x00000023;   // X Training: Plan
    constexpr SDK::MessageType::Type PLAN_PACES                = 0x00000024;   // X Training: Paces
    constexpr SDK::MessageType::Type PLAN_SETUP                = 0x00000026;   // X Training: the phone's set-up
    constexpr SDK::MessageType::Type PLAN_PREVIEW              = 0x00000027;   // X Training: a new plan's calendar
    constexpr SDK::MessageType::Type PLAN_SAVED                = 0x00000029;   // X Training: the Saved screen's lines

    // GUI --> Service
    constexpr SDK::MessageType::Type SETTINGS_SAVE         = 0x0000000A;
    constexpr SDK::MessageType::Type TRACK_START           = 0x0000000B;
    constexpr SDK::MessageType::Type TRACK_STOP            = 0x0000000C;
    constexpr SDK::MessageType::Type TRACK_PAUSE           = 0x0000000D;
    constexpr SDK::MessageType::Type TRACK_RESUME          = 0x0000000E;
    constexpr SDK::MessageType::Type MANUAL_LAP            = 0x0000000F;
    constexpr SDK::MessageType::Type INTERVALS_NEXT_PHASE  = 0x00000011;  ///< R2: next interval phase or workout step
    constexpr SDK::MessageType::Type WORKOUT_DETAILS_REQUEST = 0x00000017;
    constexpr SDK::MessageType::Type WORKOUT_END             = 0x00000018;
    constexpr SDK::MessageType::Type TREADMILL_DISTANCE      = 0x0000001D;  ///< HybridX Run: the answer to TREADMILL_ASK
    constexpr SDK::MessageType::Type PLAN_REQUEST            = 0x00000025;  ///< X Training: a plan screen's data, please
    constexpr SDK::MessageType::Type NEW_PLAN                = 0x00000028;  ///< X Training: preview, start or keep a plan

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

    // HybridX Run: the VO2max figures the screens show. Sent with the initial
    // info, when a track starts and is saved, and after a profile change.
    struct Vo2Info {
        static constexpr uint8_t kMaxRows = 10;   ///< RunVo2::Config::kHistoryRuns

        uint16_t rollingX10    = 0;   ///< shown value, ml/kg/min x 10; 0 = none yet
        uint16_t beforeX10     = 0;   ///< the shown value before the latest run (trend)
        uint8_t  runs          = 0;   ///< runs behind it

        // The run just saved (the summary's VO2MAX page).
        bool     hasRun        = false; ///< the run fields below describe a just-saved run
        uint8_t  runStatus     = 0;   ///< RunVo2::RunStatus
        uint8_t  profileStatus = 0;   ///< RunVo2::ProfileStatus
        uint16_t runX10        = 0;   ///< this run's estimate x 10; 0 = none
        uint16_t windowsUsed   = 0;   ///< steady minutes behind it
        uint16_t spreadX10     = 0;   ///< how far a typical minute's estimate sat from it, x 10 (P3)
        uint8_t  maxHr         = 0;
        uint8_t  maxSource     = 0;   ///< RunVo2::MaxHrSource
        uint8_t  restHr        = 0;
        bool     runTreadmill  = false; ///< the run was on a treadmill: no estimate (NOTES, "Treadmill")

        // The VO2max page: the last runs, newest first, as local dates.
        struct Row {
            uint8_t  day    = 0;
            uint8_t  month  = 0;
            uint16_t vo2x10 = 0;
        };
        Row      rows[kMaxRows] {};
        uint8_t  rowCount      = 0;

        // The profile as it stands now (RunVo2::resolveProfile at send time).
        uint8_t  nowStatus     = 0;   ///< RunVo2::ProfileStatus
        uint8_t  nowAge        = 0;
        uint8_t  nowMaxHr      = 0;
        uint8_t  nowMaxSource  = 0;   ///< RunVo2::MaxHrSource
        uint8_t  nowRestHr     = 0;
        uint8_t  nowRestSource = 0;   ///< RunVo2::RestHrSource

        // What the athlete set (0 = not set / automatic), for the pickers.
        uint16_t birthYear     = 0;
        uint8_t  birthMonth    = 0;
        uint8_t  setMaxHr      = 0;
        uint8_t  setRestHr     = 0;
        bool     tuningLog     = true;
    };

    struct Vo2Upd : public SDK::MessageBase {
        Vo2Info info;
        Vo2Upd() : SDK::MessageBase(VO2_UPDATE), info{} {}

        explicit Vo2Upd(const Vo2Info& info)
            : Vo2Upd()
        {
            this->info = info;
        }
    };

    // HybridX Run: the run detail page's numbers (zones, best splits, halves, drift). Sent when a
    // track starts (all zero), when it is paused and when it is saved.
    struct RunStatsUpd : public SDK::MessageBase {
        RunVo2::RunStatsResult stats;
        RunStatsUpd() : SDK::MessageBase(RUN_STATS_UPDATE), stats{} {}

        explicit RunStatsUpd(const RunVo2::RunStatsResult& stats)
            : RunStatsUpd()
        {
            this->stats = stats;
        }
    };

    // The Streak line after a save (hybridx-link StreakNow): two lines of text; show is false when
    // there is none (HybridX Streak not installed, or never opened).
    struct StreakLineUpd : public SDK::MessageBase {
        bool show      = false;
        char line1[24] = {};   ///< "3 of 4 this week"
        char line2[20] = {};   ///< "8-week streak", or empty
        uint16_t load  = 0;    ///< The run's training load, sent whether or not there is a line (0: none)
        StreakLineUpd() : SDK::MessageBase(STREAK_LINE) {}
    };

    // HybridX Run: the personal bests over 1 km, 1 mile and 5 km (RunVo2::Bests), sent when the
    // app opens and after each saved run. newMask bit i: the run just saved set best i.
    struct BestsUpd : public SDK::MessageBase {
        uint32_t ms[3]   = {};   ///< 0 = none yet
        uint32_t utc[3]  = {};   ///< the run each came from
        uint8_t  day[3]  = {};   ///< its local date, for the screen (0 = none)
        uint8_t  month[3] = {};
        uint8_t  newMask = 0;
        BestsUpd() : SDK::MessageBase(BESTS_UPDATE) {}
    };

    // HybridX Run: a treadmill run has been stopped and saved up to its session; the service
    // waits for the distance the treadmill's console shows (NOTES, "Treadmill").
    struct TreadmillAsk : public SDK::MessageBase {
        float estimateM = 0.0f;   ///< the distance worked out from cadence
        TreadmillAsk() : SDK::MessageBase(TREADMILL_ASK) {}
        explicit TreadmillAsk(float estimateM) : TreadmillAsk() { this->estimateM = estimateM; }
    };

    // HybridX Run: the answer. 0 keeps the estimate (Skip, or the app closing).
    struct TreadmillDistance : public SDK::MessageBase {
        float metres = 0.0f;
        TreadmillDistance() : SDK::MessageBase(TREADMILL_DISTANCE) {}
        explicit TreadmillDistance(float metres) : TreadmillDistance() { this->metres = metres; }
    };

    // HybridX Run: the athlete's numbers set on the watch (0 = not set /
    // automatic), written back to app_config.json by the service.
    struct Vo2ProfileSave : public SDK::MessageBase {
        uint16_t birthYear  = 0;
        uint8_t  birthMonth = 0;
        uint8_t  maxHr      = 0;
        uint8_t  restHr     = 0;
        bool     tuningLog  = true;
        Vo2ProfileSave() : SDK::MessageBase(VO2_PROFILE_SAVE) {}
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

    /**
     * The structured workouts on the watch, sent when the GUI starts and
     * again after a run. The list is the service's and stays valid while the
     * GUI runs: the service builds it only when the GUI starts, and sends
     * the same list after a run. @p today is the workout planned for today
     * to offer at launch, or kNoToday if there is none or it was completed.
     */
    struct WorkoutList : public SDK::MessageBase {
        static constexpr uint16_t kNoToday = 0xFFFF;

        const SDK::Workout::Library* library;  ///< Non-owning
        uint16_t                     today;

        WorkoutList()
            : SDK::MessageBase(WORKOUT_LIST)
            , library(nullptr)
            , today(kNoToday)
        {}

        WorkoutList(const SDK::Workout::Library* library, uint16_t today)
            : WorkoutList()
        {
            this->library = library;
            this->today   = today;
        }
    };

    /**
     * A workout read in full, in answer to WORKOUT_DETAILS_REQUEST. The
     * program is the service's: it stays valid until the next request or
     * the start of a run. nullptr if the file could not be read.
     */
    struct WorkoutDetails : public SDK::MessageBase {
        uint16_t                     index;    ///< In the list
        const SDK::Workout::Program* program;  ///< Non-owning

        WorkoutDetails()
            : SDK::MessageBase(WORKOUT_DETAILS)
            , index(0)
            , program(nullptr)
        {}

        WorkoutDetails(uint16_t index, const SDK::Workout::Program* program)
            : WorkoutDetails()
        {
            this->index   = index;
            this->program = program;
        }
    };

    /**
     * The workout face, every second while a structured workout runs. The
     * gauge is in m/s for a speed target and bpm for a heart-rate one; arc
     * is where the value sits on the gauge (0-1, with the band in the middle
     * third, faster or higher to the right).
     *
     * Each step is a lap, so the step's average pace and heart rate are the
     * lap's, in Track::Data (avgLapSpeed, avgLapHR).
     */
    struct WorkoutData : public SDK::MessageBase {
        uint32_t remainingMs;  ///< Time step: time left
        uint32_t remainingCm;  ///< Distance step: distance left
        uint32_t stepTimeMs;   ///< Time in the step so far
        uint32_t rep;          ///< Pass of the innermost repeat; 0 outside one
        uint32_t reps;         ///< Its passes in all; 0 if unlimited
        float    value;        ///< Gauge value; 0 with no target, or a speed too low for a pace
        float    low;          ///< Target band
        float    high;
        float    arc;
        uint8_t  stepEnd;      ///< SDK::Workout::StepEnd
        uint8_t  target;       ///< SDK::Workout::Target
        uint8_t  zone;         ///< SDK::Workout::Zone
        uint8_t  intensity;    ///< SDK::Workout::Intensity
        bool     leadIn;       ///< The step ends within about 5 s

        WorkoutData()
            : SDK::MessageBase(WORKOUT_DATA)
            , remainingMs(0)
            , remainingCm(0)
            , stepTimeMs(0)
            , rep(0)
            , reps(0)
            , value(0.0f)
            , low(0.0f)
            , high(0.0f)
            , arc(0.0f)
            , stepEnd(0)
            , target(0)
            , zone(0)
            , intensity(0)
            , leadIn(false)
        {}
    };

    /**
     * A workout step, sent in pairs when a workout starts and at each step
     * change: first the step that has just started (for the next-step card
     * and the step face), then the one after it (next set, for the
     * next-step face; none set if nothing follows).
     */
    struct WorkoutStep : public SDK::MessageBase {
        char     notes[SDK::Workout::kStepNotesBytes];  ///< The step's notes, or else its name; UTF-8, may be empty
        char     duration[24];  ///< e.g. "400 m"; empty for an open step
        char     target[32];    ///< e.g. "3:45-4:05 /km"; empty for no target
        uint32_t rep;           ///< The step that started: pass of the innermost repeat; 0 outside one
        uint32_t reps;          ///< Its passes in all; 0 if unlimited
        uint8_t  intensity;     ///< SDK::Workout::Intensity
        bool     next;          ///< The step after the current one
        bool     none;          ///< next only: nothing follows
        // X Training (TP3), the current step only:
        char     result[24];    ///< how the short rep just ended went: "Last rep 1:36"; empty for none
        char     aim[24];       ///< its target: "aim 1:34-1:40"; empty for none
        uint8_t  verdict;       ///< XTraining::RepJudge::Verdict
        bool     holdToStart;   ///< the warm-up before a time trial or race: R2 must be held

        WorkoutStep()
            : SDK::MessageBase(WORKOUT_STEP)
            , notes{}
            , duration{}
            , target{}
            , rep(0)
            , reps(0)
            , intensity(0)
            , next(false)
            , none(false)
            , result{}
            , aim{}
            , verdict(0)
            , holdToStart(false)
        {}
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
        static constexpr int16_t kNoWorkout = -1;
        /// X Training: follow today's planned session (built by the service from the plan).
        static constexpr int16_t kPlanSession = -2;

        bool    intervalsMode = false;
        int16_t workout       = kNoWorkout;  ///< Index in the workout list to follow; wins over intervalsMode
        TrackStart() : SDK::MessageBase(TRACK_START) {}

        explicit TrackStart(bool intervalsMode, int16_t workout = kNoWorkout)
            : TrackStart()
        {
            this->intervalsMode = intervalsMode;
            this->workout       = workout;
        }
    };

    /// Read a workout from the list in full, for its details screen. X Training: index
    /// kPlanDay asks for the plan's session on @p date instead (0: today), built by the
    /// service; the answer carries the same index.
    struct WorkoutDetailsRequest : public SDK::MessageBase {
        static constexpr uint16_t kPlanDay = 0xFFFE;
        uint16_t index;
        uint32_t date;
        WorkoutDetailsRequest() : SDK::MessageBase(WORKOUT_DETAILS_REQUEST), index(0), date(0) {}

        explicit WorkoutDetailsRequest(uint16_t index, uint32_t date = 0)
            : WorkoutDetailsRequest()
        {
            this->index = index;
            this->date  = date;
        }
    };

    /// End workout, from the action menu: the run carries on as a free run.
    struct WorkoutEnd : public SDK::MessageBase {
        WorkoutEnd() : SDK::MessageBase(WORKOUT_END) {}
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

    // HybridX Run: a message is allocated from the kernel's 256-byte pool
    // blocks (HybridX Streak's Commands.hpp, kMaxMessageBytes); the VO2
    // messages, the largest this app adds, must fit one.
    constexpr size_t kMaxMessageBytes = 256u;
    static_assert(sizeof(Vo2Upd) <= kMaxMessageBytes, "Vo2Upd exceeds the 256-byte kernel pool block");
    static_assert(sizeof(Vo2ProfileSave) <= kMaxMessageBytes, "Vo2ProfileSave exceeds the 256-byte kernel pool block");
    static_assert(sizeof(RunStatsUpd) <= kMaxMessageBytes, "RunStatsUpd exceeds the 256-byte kernel pool block");
    // The live data goes to the GUI as a whole struct every second; the running form fields
    // (cadence, step length) were added to it (136 bytes of data, 176 with the header on the host).
    static_assert(sizeof(TrackDataUpd) <= kMaxMessageBytes, "TrackDataUpd exceeds the 256-byte kernel pool block");


    // The kernel's largest message block is 256 bytes.
    static_assert(sizeof(WorkoutData) <= 256, "WorkoutData must fit a message block");
    static_assert(sizeof(WorkoutStep) <= 256, "WorkoutStep must fit a message block");

    // -- X Training: today's planned session (docs/HYBRIDX_TRAINING_BRIEF.md 8.1) -----------

    /// What the GUI shows of today: fixed-size text, worked out by the service from the
    /// plan, so the GUI needs no plan code beyond the header-only words.
    struct PlanTodayInfo {
        enum Status : uint8_t {
            NoPlan = 0,      ///< No plan chosen (the phone's "plan" is 0).
            Session,         ///< A session today, not yet done.
            RestDay,
            Done,            ///< Today's session is done (or part done).
            BeforeStart,     ///< The plan starts on a later day.
            Finished,        ///< The plan's last day has passed.
            Problem,         ///< The set-up cannot make a plan; see text.
            PhoneAsks,       ///< The phone asks for a new plan in place of the one under way.
        };
        static constexpr size_t kText = 36;
        uint8_t  status     = NoPlan;
        uint8_t  kind       = 0;      ///< XTraining::Kind of today's session
        uint8_t  runWeek    = 0;      ///< 0-based; for BeforeStart, 0
        uint8_t  weeks      = 0;
        bool     key        = false;
        uint32_t date       = 0;      ///< today, YYYYMMDD; 0 if the watch's date is not set
        uint32_t plannedSec = 0;
        uint32_t plannedM   = 0;
        char     name[kText]   = {};  ///< "Intervals"; for Problem, what is wrong
        char     what[kText]   = {};  ///< "6 x 3 min"; for BeforeStart, the start date
        char     target[kText] = {};  ///< "4:12-4:24 /km"
        // The next session after today (or the plan's first, before it starts); nextDate 0
        // for none (no plan, a problem, or the plan's last session is today or past).
        uint32_t nextDate   = 0;
        uint8_t  nextKind   = 0;
        char     nextWhat[kText] = {};   ///< "40 min"
    };

    struct PlanToday : public SDK::MessageBase {
        PlanTodayInfo info;
        PlanToday() : SDK::MessageBase(PLAN_TODAY), info{} {}
        explicit PlanToday(const PlanTodayInfo& i) : PlanToday() { info = i; }
    };
    static_assert(sizeof(PlanToday) <= kMaxMessageBytes, "PlanToday exceeds the 256-byte kernel pool block");

    /// A day of the plan, for This week's day page: the same words as Today's (no next session).
    struct PlanDay : public SDK::MessageBase {
        PlanTodayInfo info;
        PlanDay() : SDK::MessageBase(PLAN_DAY), info{} {}
    };
    static_assert(sizeof(PlanDay) <= kMaxMessageBytes, "PlanDay exceeds the 256-byte kernel pool block");

    /// This week: the run week shown (today's; the first before the plan, the last after it).
    struct PlanWeekInfo {
        struct Day {
            uint32_t date = 0;
            uint8_t  kind = 0;      ///< XTraining::Kind
            uint8_t  mark = 0;      ///< XTraining::DayMark
            bool     key  = false;  ///< the week's key session
        };
        bool    ok      = false;    ///< false: no usable plan
        uint8_t runWeek = 0;        ///< 0-based
        uint8_t weeks   = 0;
        uint8_t phase   = 0;        ///< XTraining::Phase
        Day     day[7];             ///< Monday first
    };
    struct PlanWeek : public SDK::MessageBase {
        PlanWeekInfo info;
        PlanWeek() : SDK::MessageBase(PLAN_WEEK), info{} {}
    };
    static_assert(sizeof(PlanWeek) <= kMaxMessageBytes, "PlanWeek exceeds the 256-byte kernel pool block");

    /// The plan as a whole, for the Plan screen.
    struct PlanOverviewInfo {
        static constexpr uint8_t kMaxWeeks = 24;
        uint8_t  status   = 0;      ///< PlanTodayInfo::Status, as today's
        uint8_t  plan     = 0;      ///< XTraining::PlanId; 0 when there is no usable plan
        uint8_t  level    = 0;
        uint8_t  runDays  = 0;
        uint8_t  runWeek  = 0;      ///< the week shown, 0-based
        uint8_t  weeks    = 0;
        uint8_t  weekSessions = 0;
        uint8_t  weekDone     = 0;
        uint16_t total    = 0;
        uint16_t soFar    = 0;
        uint16_t done     = 0;
        uint32_t start    = 0;      ///< the first Monday
        uint32_t end      = 0;      ///< the last Sunday
        uint32_t raceDate = 0;      ///< 0 for none
        uint16_t vo2X10      = 0;   ///< X Run's VO2max x 10 (vo2max.json); 0 = none, no line (TP4)
        uint16_t vo2StartX10 = 0;   ///< the same as first seen in this plan; 0 = not yet
        uint8_t  phase[kMaxWeeks] = {};   ///< XTraining::Phase of each run week
        char     problem[PlanTodayInfo::kText] = {};
    };
    struct PlanOverview : public SDK::MessageBase {
        PlanOverviewInfo info;
        PlanOverview() : SDK::MessageBase(PLAN_OVERVIEW), info{} {}
    };
    static_assert(sizeof(PlanOverview) <= kMaxMessageBytes, "PlanOverview exceeds the 256-byte kernel pool block");

    /// The paces, s/km, and where they came from, for the Paces screen.
    struct PlanPacesInfo {
        bool     ok         = false;
        bool     halfUsed   = false;   ///< a half was entered and is recent enough to count
        bool     gap        = false;   ///< the half is well behind the 5K (PLANS.md 2.4)
        uint16_t easyFast   = 0;
        uint16_t easySlow   = 0;
        uint16_t marathon   = 0;
        uint16_t half       = 0;
        uint16_t threshold  = 0;
        uint16_t tenK       = 0;
        uint16_t interval   = 0;
        uint16_t repetition = 0;
        uint16_t fiveK      = 0;
        uint32_t fiveKSec   = 0;
        uint32_t fiveKDate  = 0;
        uint32_t halfSec    = 0;       ///< as entered, used or not
        uint32_t halfDate   = 0;
        uint32_t halfAs5kSec          = 0;
        uint32_t predicted10kSec      = 0;
        uint32_t predictedHalfSec     = 0;
        uint32_t predictedMarathonSec = 0;
    };
    struct PlanPaces : public SDK::MessageBase {
        PlanPacesInfo info;
        PlanPaces() : SDK::MessageBase(PLAN_PACES), info{} {}
    };
    static_assert(sizeof(PlanPaces) <= kMaxMessageBytes, "PlanPaces exceeds the 256-byte kernel pool block");

    /// A plan's set-up as the phone has it (and the watch's New plan makes it).
    struct PlanSetupInfo {
        uint8_t  plan        = 0;      ///< XTraining::PlanId
        uint8_t  level       = 2;
        uint8_t  days        = 0;      ///< bit 0 Monday .. bit 6 Sunday
        uint8_t  longDay     = 6;      ///< 0 Monday .. 6 Sunday
        bool     fiveKRecent = true;
        uint32_t fiveKSec    = 0;
        uint32_t raceDate    = 0;      ///< YYYYMMDD; 0 for none
    };

    /// The answer to PlanRequest::Setup: the phone's set-up and, when it asks for a new plan,
    /// the plan under way.
    struct PlanSetupUpd : public SDK::MessageBase {
        PlanSetupInfo phone;
        PlanSetupInfo current;
        bool          phoneAsks = false;
        PlanSetupUpd() : SDK::MessageBase(PLAN_SETUP) {}
    };
    static_assert(sizeof(PlanSetupUpd) <= kMaxMessageBytes, "PlanSetupUpd exceeds the 256-byte kernel pool block");

    /// GUI -> Service: a new plan. Preview answers PLAN_PREVIEW; Apply writes the set-up to the
    /// phone's values file and starts it from the next Monday (or this one, Monday to
    /// Wednesday); KeepCurrent answers the phone's ask with no, writing the plan under way back.
    struct NewPlan : public SDK::MessageBase {
        enum Action : uint8_t { Preview = 0, Apply, KeepCurrent };
        uint8_t       action = Preview;
        PlanSetupInfo info;
        NewPlan() : SDK::MessageBase(NEW_PLAN) {}
        NewPlan(uint8_t a, const PlanSetupInfo& i) : NewPlan() { action = a; info = i; }
    };

    /// What a new plan would be: its calendar, or why it cannot be made.
    struct PlanPreviewInfo {
        bool     ok        = false;
        uint8_t  status    = 0;        ///< XTraining::Calendar::Status
        uint8_t  weeks     = 0;        ///< weeks to run
        uint8_t  planWeeks = 0;        ///< the plan's full length
        uint32_t start     = 0;        ///< the first Monday
        char     problem[PlanTodayInfo::kText] = {};
    };
    struct PlanPreview : public SDK::MessageBase {
        PlanPreviewInfo info;
        PlanPreview() : SDK::MessageBase(PLAN_PREVIEW), info{} {}
    };
    static_assert(sizeof(PlanPreview) <= kMaxMessageBytes, "PlanPreview exceeds the 256-byte kernel pool block");

    /// After a planned session is saved: how it counts, the week, and what is next.
    struct PlanSavedInfo {
        enum Outcome : uint8_t { None = 0, Done, PartDone, TooShort };
        uint8_t  outcome      = None;
        uint8_t  runWeek      = 0;
        uint8_t  weekDone     = 0;
        uint8_t  weekSessions = 0;
        uint32_t nextDate     = 0;      ///< 0 for none
        uint8_t  nextKind     = 0;      ///< XTraining::Kind
    };
    struct PlanSaved : public SDK::MessageBase {
        PlanSavedInfo info;
        PlanSaved() : SDK::MessageBase(PLAN_SAVED), info{} {}
    };

    /// GUI -> Service: a plan screen's data.
    struct PlanRequest : public SDK::MessageBase {
        enum What : uint8_t { Day = 0, Week, Overview, Paces, Setup };
        uint8_t  what = Week;
        uint32_t date = 0;          ///< for Day; 0 is today
        PlanRequest() : SDK::MessageBase(PLAN_REQUEST) {}
        PlanRequest(uint8_t w, uint32_t d) : PlanRequest() { what = w; date = d; }
    };

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
