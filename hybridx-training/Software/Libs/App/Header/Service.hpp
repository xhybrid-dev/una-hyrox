#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <ctime>   // std::time_t (mLastCalibUtc, startTrack, ...)

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/TrackMap/TrackMapBuilder.hpp"
#include "SDK/Metrics/MonotonicTime.hpp"
#include "SDK/Metrics/MonotonicCounter.hpp"
#include "SDK/Metrics/VariableCounter.hpp"
#include "SDK/Metrics/GpsSpeedFilter.hpp"
#include "SDK/Metrics/DeltaCounter.hpp"
#include "SDK/Metrics/ThrottledSample.hpp"
#include "SDK/Filters/SimpleLPF.hpp"

#include "SDK/Calibration/OutdoorStrideCalibrator.hpp"
#include "SDK/Calibration/CadenceStrideModel.hpp"      // HybridX Run: treadmill mode
#include "SDK/Calibration/TreadmillSpeedEstimator.hpp"

#include "SDK/Fit/FitWorkoutReader.hpp"
#include "SDK/Workout/Intervals.hpp"
#include "SDK/Workout/ScheduleState.hpp"
#include "SDK/Workout/TargetGauge.hpp"
#include "SDK/Workout/WorkoutAlerts.hpp"
#include "SDK/Workout/WorkoutEngine.hpp"
#include "SDK/Workout/WorkoutLibrary.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

#include "SettingsSerializer.hpp"
#include "ActivitySummarySerializer.hpp"
#include "ActivityWriter.hpp"
#include "Commands.hpp"
#include "WristTiltDetector.hpp"

#include <memory>

#include "SDK/AppConfig/AppConfig.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"

#include "FormStats.hpp"
#include "RunStats.hpp"
#include "Vo2Window.hpp"
#include "Vo2WindowFile.hpp"
#include "RunVo2Max.hpp"

// X Training: the plan core (Software/Libs/Core).
#include "PaceModel.hpp"
#include "PlanBuilder.hpp"
#include "ZoneDebounce.hpp"
#include "PlanSchedule.hpp"
#include "PlanProgress.hpp"
#include "PlanRules.hpp"
#include "PlanState.hpp"
#include "ReviewInsight.hpp"
#include "ReviewLog.hpp"
#include "SessionReview.hpp"

class Service : public WristTiltDetector::IListener
{
public:
    Service(SDK::Kernel &kernel);

    virtual ~Service();

    void run();

private:
    // -- Constants ------------------------------------------------------------

    static constexpr uint32_t skBacklightTimeout     = 5000;
    static constexpr uint32_t skSamplePeriod         = 1000;
    static constexpr uint32_t skSampleLatency        = 1000;

    static constexpr float    skMapDistanceThreshold = 10.0f; // meters
    static constexpr uint32_t skMapMaxPoints         = 70;

    static constexpr uint32_t skBatteryLogPeriodMs   = 5 * 60 * 1000;
    static constexpr float    skFusionSampleRateHz   = 100.0f;

    /// Geometry of the live-speed filter, in 1 Hz track ticks. The receiver
    /// already averages its speed over 10 s, so the window only bridges a tick
    /// that brings no sample; a longer one would average twice and add lag.
    /// The position chord is 2 s, 5-8 m at running pace: a longer one cuts the
    /// corners of the path and reads the scale low.
    static constexpr std::size_t skSpeedSmoothTicks = 2;
    static constexpr std::size_t skSpeedScaleTau    = 180;
    static constexpr std::size_t skSpeedChordTicks  = 2;

    /// Laps reserved in the summary for a run, plus one per workout step up
    /// to skMaxWorkoutLapReserve, so that a workout's laps do not reallocate
    /// the list mid-run.
    static constexpr std::size_t skLapReserve           = 10;
    static constexpr std::size_t skMaxWorkoutLapReserve = 128;

    /// The lead-in's beep each second near the end of a step: shorter than
    /// the step-change alert's, so the countdown and the change sound apart.
    static constexpr uint16_t skLeadInBeepMs = 80;

    /// The gauge arc's outer thirds: 30 s/km beyond a pace band, 15 bpm
    /// beyond a heart-rate band.
    static constexpr float skArcMarginSecPerKm = 30.0f;
    static constexpr float skArcMarginBpm      = 15.0f;

    // -- Infrastructure -------------------------------------------------------

    SDK::Kernel&          mKernel;
    bool                  mGuiStarted;

    // -- Settings & persistence -----------------------------------------------

    Settings                  mSettings;
    bool                      mIsImperial = false;
    bool                      mTimeFormat12h = false;
    SettingsSerializer        mSettingsSerializer;
    ActivitySummary           mSummary;
    ActivitySummarySerializer mActivitySummarySerializer;
    ActivityWriter            mActivityWriter;
    SDK::TrackMapBuilder      mTrackMapBuilder;

    // -- Sensors --------------------------------------------------------------

    SDK::Sensor::Connection mSensorGpsLocation;
    SDK::Sensor::Connection mSensorGpsSpeed;
    SDK::Sensor::Connection mSensorGpsDistance;
    SDK::Sensor::Connection mSensorPressure;
    SDK::Sensor::Connection mSensorHr;
    SDK::Sensor::Connection mSensorBatteryLevel;
    SDK::Sensor::Connection mSensorBatteryMetrics;
    SDK::Sensor::Connection mSensorWristMotion;
    SDK::Sensor::Connection mSensorFusion;
    SDK::Sensor::Connection mSensorRunningCadence;
    SDK::Sensor::Connection mSensorGrade;
    bool                    mIsSensorsConnected = false;

    struct {
        float cadenceSpm      = 0.0f;
        bool  cadenceValid    = false;
    } mRunningCadence{};
    // HybridX Run: running form. The cadence sensor has no timestamp of its own, so the
    // arrival of the last sample is stamped here and a stalled sensor stops counting.
    bool                    mCadenceSeen   = false;
    uint32_t                mCadenceLastMs = 0;
    bool                    mFormCadenceOk = false;   ///< this tick: a valid, fresh cadence
    bool                    mFormStepOk    = false;   ///< this tick: and a step length in the SDK's gate
    float                   mFormStepM     = 0.0f;
    RunVo2::FormStats       mForm;                    ///< the run's and the lap's averages
    /// The run detail page's numbers (about 4 KB, so on the heap, made when a track starts and
    /// freed when it ends). Null if there was no memory: the page then shows nothing.
    std::unique_ptr<RunVo2::RunStats> mRunStats;
    uint32_t mHrLastMs    = 0;   ///< getTimeMs() of the latest HR sample (X Run's, for feedSecond)
    uint32_t mSpeedLastMs = 0;   ///< getTimeMs() of the latest GPS speed sample
    uint32_t mGradeLastMs = 0;   ///< getTimeMs() of the latest grade sample
    bool     mHrSeen      = false;
    bool     mSpeedSeen   = false;
    bool     mGradeSeen   = false;
    // TP4: this run's steady minutes for HybridX Run's VO2max (Vo2Window.hpp, copied from Run's).
    RunVo2::WindowBuilder                   mVo2Builder;
    std::unique_ptr<RunVo2::WindowRecord[]> mVo2Windows;   ///< kWindowFileMaxWindows, from the start to the save
    uint16_t                                mVo2WindowCount = 0;
    XTraining::RunVo2Max::Figure            mRunVo2;   ///< X Run's VO2max; x10 0 = none (no X Run)

    // -- Outdoor stride calibration inputs (latched per stream) ---------
    struct {
        float gradePct        = 0.0f;
        bool  gradeValid      = false;
    } mGradeData{};
    float       mGpsSpeedMs       = 0.0f; ///< Latest raw GPS speed (instantaneous source).
    bool        mGpsSpeedValid    = false;
    bool        mGpsSpeedFresh    = false; ///< A speed sample arrived since the last track tick.
    float       mGpsSpeedPeakMs   = 0.0f;  ///< Highest valid speed sample since the last track tick.
    bool        mGpsPosFresh      = false; ///< A position sample arrived since the last track tick.
    bool        mGpsDeadReckoning = false;
    std::time_t mLastCalibUtc     = 0;   ///< For per-tick delta_t.

    // -- Metrics --------------------------------------------------------------

    SDK::Metric::MonotonicTime<SDK::Interface::ISystem> mTimeTracker;
    SDK::Metric::MonotonicCounter<std::time_t>          mTimeCounter;
    SDK::Metric::MonotonicCounter<float>                mDistanceCounter;
    SDK::Metric::VariableCounter                        mSpeedCounter;
    /// Corrects the receiver's Doppler speed against the distance its own
    /// position track covers, and smooths it. This is the single source for
    /// everything derived from GPS speed -- the readout, the FIT record series,
    /// the maxima, the implied step length and the stride calibrator -- so that
    /// none of them inherit the receiver's environment-dependent under-read.
    /// The averages are not involved either way: they come from the distance and
    /// time totals, not from a mean of these samples.
    SDK::Metric::GpsSpeedFilter<skSpeedSmoothTicks,
                                skSpeedScaleTau,
                                skSpeedChordTicks>      mSpeedFilter;
    SDK::Metric::VariableCounter                        mHrCounter;
    uint8_t                                             mHrSource = 0;      ///< Latest HR source (HeartRateEx::Source) for the icon + FIT hr_source.
    uint8_t                                             mHrOpticalBpm = 0;  ///< Latest raw optical (PPG) bpm, for the FIT hr_optical series.
    uint8_t                                             mHrExternalBpm = 0; ///< Latest raw external (strap) bpm, for the FIT hr_external series.
    SDK::Filter::SimpleLPF                              mAltitudeFilter;
    SDK::Metric::DeltaCounter                           mAltitudeCounter;

    // Battery SoC and voltage are sampled independently;
    // a FIT record is written only when both are due.
    SDK::Metric::ThrottledSample<float, SDK::Interface::ISystem> mBatterySoc;     ///< State of charge, percent
    SDK::Metric::ThrottledSample<float, SDK::Interface::ISystem> mBatteryVoltage; ///< Voltage, volts

    // -- GPS state ------------------------------------------------------------

    struct {
        bool     fix;       // Actual GPS fix
        float    latitude;  // degrees
        float    longitude; // degrees
        float    altitude;  // meters
        uint32_t timestamp; // ms

        void reset()
        {
            fix       = false;
            latitude  = 0.0f;
            longitude = 0.0f;
            altitude  = 0.0f;
            timestamp = 0;
        }
    } mGps{};

    float mSeaLevelPressure = 0.0f; // Pa

    // -- Track state ----------------------------------------------------------

    enum class LapDivSource {
        OFF = 0,
        DISTANCE,
        TIME,
    };

    LapDivSource mLapDivSource        = LapDivSource::OFF;
    Track::State mTrackState          = Track::State::INACTIVE;
    bool         mPreviousGpsFixState = false;
    bool         mGpsInitialConnectFailed = false;  ///< GPS_LOCATION subscribe lost the startup ack race; the retry logs the recovery.
    bool         mGpsWanted = false;                 ///< GPS_LOCATION should stay connected (pre-activity + active track); cleared in disconnect() so the retry never re-wakes the GNSS post-activity.
    bool         mSessionNotEmpty     = false;
    uint16_t     mLastLoad            = 0;      ///< HybridX Run: the run's training load (0: none)
    std::time_t  mStartUtc            = 0;      ///< The start startTrack was given: the activity file's name is made from it.
    bool         mLapNotEmpty         = false;
    uint32_t     mLapCarryMs          = 0;      ///< Time the last lap handed to this one.
    Track::Data  mTrackData{};

    // -- Interval training state ----------------------------------------------

    bool mIntervalsMode      = false;
    bool mIntervalsCompleted = false; ///< Set after workout completed; blocks further phase processing

    /// The intervals settings as a workout program, which mEngine runs step
    /// by step. The program is about 15 KB: the Service is statically
    /// allocated, never on a stack.
    SDK::Workout::IntervalsSpec   mIntervalsSpec;
    SDK::Workout::IntervalsLayout mIntervalsLayout;
    SDK::Workout::Program         mProgram;
    SDK::Workout::Engine          mEngine;
    SDK::Workout::LeadInAlert     mLeadIn;

    // -- Wrist tilt -----------------------------------------------------------

    WristTiltDetector mWristTiltDetector;

    // -- Outdoor stride calibrator ---------------------------------------

    SDK::Calibration::OutdoorStrideCalibrator mCalibrator;

    // -- HybridX Run: treadmill mode (docs/NOTES.md "Treadmill") -------------
    // The SDK's method, as its Treadmill example: cadence -> stride (from the shared outdoor
    // stride table, plus a treadmill correction learned in this app's folder) -> speed and
    // distance; after Save the athlete may enter the console's distance, which corrects the
    // session and teaches the correction. No GPS, no VO2max.
    SDK::Calibration::CadenceStrideModel      mTreadModel;
    SDK::Calibration::TreadmillSpeedEstimator mTreadEstimator;
    float        mHeightM          = 1.70f;   ///< the UNA profile's height, for the stride before any calibration
    bool         mTreadmill        = false;   ///< this track is a treadmill run (latched at start)
    bool         mAwaitingDistance = false;   ///< saved up to its session; waiting for the console's distance
    bool         mAppStopping = false;        ///< COMMAND_APP_STOP is being handled: save what must be saved, nothing more
    std::time_t  mLastTreadUtc     = 0;
    float        mTreadEstimateM   = 0.0f;    ///< the run's total at the stop: what the counters hold (the belt
                                              ///< speed where one is assumed, the cadence estimate elsewhere)
    float        mTreadModelM      = 0.0f;    ///< what the stride model made of the steps alone, at the stop: what it learns against
    // Treadmill sessions (Jon, 16 November 2026): a step with a designated speed is run with the belt
    // assumed to be at it (SessionMeta::beltMmps); the speed and distance of the run are that, elsewhere
    // the cadence estimate. The estimator itself keeps running alone, for the stride model.
    float        mTreadSpeedMps    = 0.0f;    ///< the speed this second: assumed belt speed, or the estimate's
    bool         mTreadSpeedValid  = false;   ///< ... and whether the runner is running (the estimate's validity)
    float        mBeltDistanceM    = 0.0f;    ///< the run's distance so far on that basis
    float        mLastEstDistM     = 0.0f;    ///< the estimator's own distance at the last second
    bool         mBeltUsed         = false;   ///< a second of this run took an assumed belt speed
    float        mTreadSteps[SDK::Calibration::StrideLut::kBinCount] {};   ///< steps per cadence bin, for the learning
    ActivityWriter::TrackData mPendingTrack {};   ///< the session, held until the distance is known

    // X Training: the phone's settings (app_config.json), kept for write-back.
    std::unique_ptr<SDK::AppConfig> mConfig;
    bool mPhoneAsks    = false;   ///< X Training: the phone's settings ask for a new plan
    bool mForceNewPlan = false;   ///< X Training: the next loadPlan() starts the phone's plan afresh
    uint32_t mTrialSec = 0;       ///< X Training (TP5): a planned trial's or race's time, seconds; 0 = none
    bool mConfigDirty  = false;   ///< X Training (TP5): the plan's 5K changed; the phone's fields are written back
    uint8_t mNotice    = 0;       ///< X Training (TP5): the result of the last PlanAction, for the next PlanToday
    uint8_t mTrialOutcome = 0;    ///< X Training (TP5): PlanSavedInfo::Trial of the run just saved
    uint16_t mTrialFiveK = 0;     ///< ... and its 5K equivalent

    // -- X Training: the plan (docs/HYBRIDX_TRAINING_BRIEF.md sections 7 and 9) ----------
    XTraining::PlanSetup   mPhoneSetup;          ///< what app_config.json asks for
    const char*            mSetupProblem = nullptr;  ///< why it cannot make a plan, or null
    XTraining::PlanState   mPlan;                ///< plan.json (about 1 KB of log)
    XTraining::PaceResult  mPaces;               ///< from the plan's 5K (and half)
    XTraining::DaySession  mToday;               ///< today's session (Rest when none)
    uint32_t               mTodayFileHash = 0;   ///< of the training_today.json last written; 0 = none yet
    CustomMessage::PlanTodayInfo mTodayInfo;     ///< as last sent to the GUI
    /// The alert modes and estimates of the planned session being run.
    XTraining::SessionMeta mSessionMeta;
    bool                   mPlannedRun   = false;  ///< this track follows today's session
    XTraining::Ymd         mPlannedDate  = 0;      ///< the day it was planned for
    bool                   mOutAlerted   = false;  ///< an out-of-band alert played this step
    // X Training (TP3): the last short rep's result, for the next step's card.
    char                   mRepResult[24] = {};
    char                   mRepAim[24]    = {};
    uint8_t                mRepVerdict    = 0;
    // X Training (TP8): how the session kept to its paces, and the runner's feel (NOTES "TP8").
    XTraining::Review::Accumulator mReviewAcc;     ///< the session being run, fed once a second
    XTraining::ReviewLog           mReviews;       ///< reviews.json: the last 24 reviews and the feel
    XTraining::Ymd                 mReviewDate = 0; ///< the session the Feel screen is about (just saved); 0 = none
    /// Race alerts "First 2 km": how far into the race step "too fast" may sound.
    static constexpr uint32_t kRaceAlertCm = 200000;

    // -- Structured workouts --------------------------------------------------

    /// The workout files, listed when the GUI starts and no run is going
    /// (about 10 KB). A run reads its workout into mProgram, and so do the
    /// list and the details screen, which only happen before a run.
    SDK::Workout::Library        mLibrary;
    SDK::Fit::FitWorkoutReader   mReader;
    SDK::Workout::ScheduleState  mSchedule;

    bool              mWorkoutMode       = false; ///< A structured workout is running.
    int16_t           mWorkoutRequested  = CustomMessage::TrackStart::kNoWorkout;
    bool              mWorkoutReachedEnd = false; ///< Completion reached; ending early after it still counts.
    SDK::Workout::Ymd mWorkoutDate       = 0;     ///< The running workout's planned day, 0 if not scheduled.
    char              mWorkoutFile[SDK::Workout::Library::kFileBytes] = {};

    SDK::Workout::SpeedGauge     mSpeedGauge;
    SDK::Workout::GpsCatchUp     mGpsCatchUp;  ///< Holds the speed gauge off until the distance catches up.
    SDK::Workout::HeartRateGauge mHrGauge;
    SDK::Workout::ZoneAlerts     mZoneAlerts;
    XTraining::ZoneDebounce      mZoneDebounce;   ///< steadies the zone the alerts see
    /// A zone alert's second playing, until it is sent; see playAlertTwice().
    const SDK::Message::RequestBuzzerPlay::Note* mAlertRepeat        = nullptr;
    uint8_t                                      mAlertRepeatCount   = 0;
    uint32_t                                     mAlertRepeatFromMs  = 0;
    uint32_t                                     mAlertRepeatAfterMs = 0;
    float                        mBandLow  = 0.0f; ///< The step's target band, m/s or bpm.
    float                        mBandHigh = 0.0f;

    /// HR zone thresholds from the system settings; the last is maximum HR.
    uint8_t mHrThresholds[CustomMessage::kHrThresholdsCount] = {};

    // -- Lifecycle ------------------------------------------------------------

    // X Training
    void loadConfig();           ///< app_config.json, once at start; then the plan
    void loadPlan();             ///< plan.json, reconciled with the phone's settings
    void savePlan();
    void loadReviews();          ///< TP8: reviews.json
    void saveReviews();
    void reviewSecond(const SDK::Workout::Engine::Status& st);   ///< TP8: one judged second of a planned run
    void sendPlanForm();         ///< TP8: the Form page
    /// TP8: the offer to hold the plan, made of the reviews; false (and @p detail 0) when there is none.
    bool easeOffer(uint8_t& detail, uint8_t& line, uint8_t& n);
    /// Today's session from the plan and the watch's date. @p writeFile false leaves training_today.json alone
    /// (a session is starting: its writer needs about 15 KB the run's own buffers should have).
    void updateToday(bool writeFile = true);
    void writeTodayFile();       ///< TP6: training_today.json for the glance, when it has changed
    void computePaces();         ///< mPaces from the plan's 5K (and a recent half)
    void settleToday();          ///< TP5: log missed days, repeat a week, save the plan if it changed
    void writeFiveK();           ///< TP5: the plan's 5K back into the phone's fields, so they agree
    /// What the plan says of @p date (Today's page, a row of This week); @p day the session.
    void dayInfo(SDK::Workout::Ymd date, SDK::Workout::Ymd today, CustomMessage::PlanTodayInfo& info,
                 XTraining::DaySession& day);
    void sendPlanWeek(SDK::Workout::Ymd today);       ///< This week (TP3)
    void sendPlanOverview(SDK::Workout::Ymd today);   ///< Plan
    void sendPlanPaces();                             ///< Paces
    static CustomMessage::PlanSetupInfo toSetupInfo(const XTraining::PlanSetup& s);
    /// Why @p i cannot make a plan, in the watch's words; nullptr if it can.
    static const char* checkSetup(const CustomMessage::PlanSetupInfo& i);
    /// The plan's phone fields written to app_config.json (AppConfig::save), as Race does.
    void writeConfig(const CustomMessage::PlanSetupInfo& i);
    /// The plan's session on @p date (a rest day included); false outside the plan.
    bool planDay(SDK::Workout::Ymd date, XTraining::DaySession& out) const;
    /// The first day from @p from (within two weeks) with a run on it.
    bool nextSession(SDK::Workout::Ymd from, XTraining::DaySession& out) const;
    /// @p date's session built into mProgram for the step list; false on a rest day,
    /// outside the plan, or during a run.
    bool buildPlanDay(SDK::Workout::Ymd date);
    void sendPlanToday();
    bool startPlannedSession();  ///< build today's program into mProgram and start it
    void recordPlannedRun(const ActivityWriter::TrackData& track);
    /// Whether a zone alert may play on this step of a planned session (PLANS.md 2.5).
    bool zoneAlertAllowed(SDK::Workout::ZoneAlerts::Alert alert);
    uint32_t beltMmpsNow() const;   ///< the assumed belt speed of the step being run, mm/s; 0 = use the estimate
    void sendRunStats();         ///< the run detail page's numbers to the GUI (start, pause, save)
    void updateForm();           ///< cadence and step length for this tick: screen, record and averages
    void feedSecond();           ///< X Run's per-second feed: the run statistics and the VO2 windows
    void saveVo2Windows();       ///< the run's steady minutes, for HybridX Run (training_vo2_*.json)
    void readRunVo2Max();        ///< X Run's VO2max (vo2max.json), for the Plan screen
    void writeActivityLink(const ActivityWriter::TrackData& track);   ///< the saved run, for Streak (hybridx-link)
    bool closeActivity(const ActivityWriter::TrackData& track);       ///< finish the .fit, tell the watch and Streak
    void sendStreakLine();                                            ///< the Streak line, after a save
    void handleEvent(const CustomMessage::TreadmillDistance& event);  ///< HybridX Run: the console's distance
    void finishTreadmill(float metres);                               ///< 0 keeps the estimate

    void connectGps();
    void connectSensors(); // All except GPS
    void disconnect();
    void onStartGUI();
    void onStopGUI();

    // -- Sensor data dispatch -------------------------------------------------

    void handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data);

    // -- Event handlers -------------------------------------------------------

    void handleEvent(const CustomMessage::TrackStart& event);
    void handleEvent(const CustomMessage::TrackStop& event);
    void handleEvent(const CustomMessage::SettingsSave& event);
    void handleEvent(const CustomMessage::TrackPause& event);
    void handleEvent(const CustomMessage::TrackResume& event);
    void handleEvent(const CustomMessage::ManualLap& event);
    void handleEvent(const CustomMessage::IntervalsNextPhase& event);
    void handleEvent(const CustomMessage::WorkoutDetailsRequest& event);
    void handleEvent(const CustomMessage::PlanRequest& event);   ///< X Training
    void handleEvent(const CustomMessage::NewPlan& event);       ///< X Training
    void handleEvent(const CustomMessage::PlanAction& event);    ///< X Training (TP5)
    /// Judge the short rep that just ended (step @p endedStep, its lap saved if @p lapSaved).
    void judgeRep(uint16_t endedStep, bool lapSaved, bool ranOut);
    /// The open warm-up of a time trial or race, which R2 ends only when held.
    bool holdToStart(const SDK::Workout::Step& s) const;
    void handleEvent(const CustomMessage::WorkoutEnd& event);

    // -- Track control --------------------------------------------------------

    void sendInitialInfoToGui();
    void startTrack(std::time_t utc);
    void processTrack();

    /// What ended a lap, and the workout step it was, for the FIT lap.
    struct LapEnd {
        SDK::Fit::LapTrigger trigger      = SDK::Fit::LapTrigger::Manual;
        uint16_t             wktStepIndex = SDK::Fit::kMessageIndexInvalid;
        SDK::Fit::Intensity  intensity    = SDK::Fit::Intensity::Invalid;
        /// A workout step that ran out (ranOut) ended between two ticks.
        /// carryMs is the time counted beyond its end, for the next lap; stepM
        /// is the distance it covered, the rest of the lap's counted distance
        /// going to the next lap.
        bool                 ranOut       = false;
        uint32_t             carryMs      = 0;
        float                stepM        = 0.0f;
    };
    void saveLap(const LapEnd& end, float autoLapDistanceM = 0.0f);
    void stopTrack(bool discard);
    void pauseTrack(bool pause);
    void buildPartialSummary();
    ActivityWriter::RecordData prepareRecordData();
    LapDivSource getLapDivSource();

    // -- Interval training ----------------------------------------------------

    /// Build the program from the intervals settings and start it.
    void startIntervals();
    /// R2: end the current interval phase or workout step and start the next.
    void nextStep();
    void processIntervals();
    /// A step ended (@p how says why): close its lap, and alert.
    void onIntervalsStepEnded(SDK::Workout::Engine::Change how);
    /// Refresh mTrackData.intervals from the engine's current step.
    void updateIntervalsData();
    void notifyStepChange();
    Track::IntervalsPhase intervalsPhase(uint16_t step) const;

    /// The lap for workout step @p step, ended by @p trigger.
    LapEnd workoutLap(uint16_t step, SDK::Fit::LapTrigger trigger) const;
    /// The lap for the step the engine has just ended, from where it ended.
    LapEnd endedStepLap(SDK::Workout::Engine::Change how) const;
    /// What ended step @p step, as the engine reported it.
    SDK::Fit::LapTrigger lapTrigger(SDK::Workout::Engine::Change how, uint16_t step) const;

    // -- Structured workouts --------------------------------------------------

    /// List the workouts, unless a run is going.
    void scanWorkouts();
    /// Send the list, with today's workout unless it was completed.
    void sendWorkoutList();
    /// Read entry @p index of the list into mProgram.
    bool readWorkout(uint16_t index);
    bool startWorkout(uint16_t index);
    void processWorkout();
    void onWorkoutStepEnded(SDK::Workout::Engine::Change how);
    /// Set the gauges and alerts up for the engine's current step.
    void startWorkoutStep();
    void endWorkout();
    void sendWorkoutData();
    void sendWorkoutSteps();
    void sendWorkoutStep(const SDK::Workout::Step* step, bool next);
    void playZoneAlert(SDK::Workout::ZoneAlerts::Alert alert);
    void playAlertTwice(const SDK::Message::RequestBuzzerPlay::Note notes[], uint8_t count);
    void sendAlertRepeat();
    /// The zone of the current step's target, Unknown with none.
    SDK::Workout::Zone targetZone() const;
    /// A heart-rate target as a bpm band, from the watch's HR thresholds.
    bool hrBand(const SDK::Workout::Step& step, float& low, float& high) const;
    bool hasTrustedHr() const;
    /// The watch's local date, or 0 if its clock is not set.
    SDK::Workout::Ymd localDate();

    /// The run's active time and distance, as the workout engine counts them.
    uint32_t activeTimeMs() const;
    uint32_t activeDistanceCm() const;

    // -- Notifications --------------------------------------------------------

    void setCapabilities();
    void requestAccessoryPrepare();   // opt in to external HR (pre-warm at GUI start)
    void requestAccessoryRelease();
    void notifyFirstFix();
    void notifyLapEnd();
    void notifyNewActivity();
    void backlightOn(uint32_t timeoutMs = skBacklightTimeout);
    void playBuzzerPattern(uint16_t beepMs, uint8_t count = 1, uint16_t silenceMs = 100, uint8_t volume = 100);
    void playBuzzerNotes(const SDK::Message::RequestBuzzerPlay::Note notes[], uint8_t count);
    void playVibroPattern(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count = 1, uint16_t silenceMs = 100);

    // -- WristTilt callback ---------------------------------------------------

    virtual void onWristTilt(uint32_t timestampMs) override;
};

#endif // SERVICE_HPP
