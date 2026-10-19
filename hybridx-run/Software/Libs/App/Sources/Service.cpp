
#include "Service.hpp"

#include <algorithm>
#include <ctime>
#include <cmath>
#include <memory>
#include <new>
#include <cstring>
#include <cstdio>
#include <utility>

#include "Settings.hpp"
#include "ActivitySummary.hpp"
#include "Track.hpp"
#include "SDK/Tools/FirmwareVersion.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/Messages/AccessoryMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Utils/Utils.hpp"
#include "SDK/Timer/Timer.hpp"

#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsSpeed.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsDistance.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserPressure.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryLevel.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserBatteryMetrics.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserWristMotion.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserFusionRaw.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserRunningCadence.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGrade.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateMetrics.hpp"
#include "SDK/AppConfig/AppConfig.hpp"

// HybridX Run
#include "AppConfigFields.hpp"
#include "SafeFile.hpp"
#include "Vo2TrailIngest.hpp"

#include "SDK/Calibration/StrideMath.hpp"
#include "SDK/Workout/StepLabel.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace {

/** @brief Convert speed (m/s) to pace (s/m). Returns 0 if speed is below threshold. */
static float getPace(float speed, float threshold)
{
    return (speed > threshold) ? (1.0f / speed) : 0.0f;
}

// Average speed per the FIT definition: total distance / active timer time.
// This is NOT the mean of the instantaneous speed samples -- a sample mean
// drifts away from distance/time because sub-threshold samples are dropped
// from the average while their seconds still count toward the duration. Using
// the totals keeps distance, duration and the reported average pace mutually
// consistent on every lap and session.
static float speedFromTotals(float distanceM, float activeTimeS)
{
    return (activeTimeS > 0.0f) ? (distanceM / activeTimeS) : 0.0f;
}

// HybridX Run: where the VO2max history and its public copy live.
constexpr const char* kVo2File    = "vo2.json";
constexpr const char* kSharedDir  = "../SharedData/HybridX";
constexpr const char* kSharedFile = "../SharedData/HybridX/vo2max.json";

// A latched HR or speed reading counts for the VO2max windows only while it is
// this recent (both sensors report once a second).
constexpr uint32_t kVo2FreshMs = 2500;

} // namespace

Service::Service(SDK::Kernel &kernel)
        : mKernel(kernel)
        , mGuiStarted(false)
        , mSettings{}
        , mSettingsSerializer(mKernel, "settings.json")
        , mSummary{}
        , mActivitySummarySerializer(mKernel, "Activity/summary.json")
        , mActivityWriter(mKernel, "Activity")
        , mTrackMapBuilder{}
        , mSensorGpsLocation(SDK::Sensor::Type::GPS_LOCATION, skSamplePeriod, skSampleLatency)
        , mSensorGpsSpeed(SDK::Sensor::Type::GPS_SPEED, skSamplePeriod, skSampleLatency)
        , mSensorGpsDistance(SDK::Sensor::Type::GPS_DISTANCE, skSamplePeriod, skSampleLatency)
        , mSensorPressure(SDK::Sensor::Type::PRESSURE, skSamplePeriod, skSampleLatency)
        , mSensorHr(SDK::Sensor::Type::HEART_RATE_EX, skSamplePeriod, skSampleLatency)
        , mSensorBatteryLevel(SDK::Sensor::Type::BATTERY_LEVEL)
        , mSensorBatteryMetrics(SDK::Sensor::Type::BATTERY_METRICS, skSamplePeriod, skSampleLatency)
        , mSensorWristMotion(SDK::Sensor::Type::WRIST_MOTION)
        , mSensorFusion(SDK::Sensor::Type::FUSION_RAW, 1000.0f / skFusionSampleRateHz, 100)
        , mSensorRunningCadence(SDK::Sensor::Type::RUNNING_CADENCE, skSamplePeriod, skSampleLatency)
        , mSensorGrade(SDK::Sensor::Type::GRADE, skSamplePeriod, skSampleLatency)
        // A daily figure: once a minute is plenty. With the default period 0
        // the simulator's sensor never reported (its timer needs a period).
        , mSensorHrMetrics(SDK::Sensor::Type::HEART_RATE_METRICS_DAILY, 60000)
        , mTimeTracker(kernel.sys)
        , mAltitudeFilter(0.8f)
        , mAltitudeCounter()
        , mBatterySoc(kernel.sys)
        , mBatteryVoltage(kernel.sys)
        , mWristTiltDetector()
        , mCalibrator(mKernel.fs)
        , mSchedule(mKernel.fs)
{
    mTimeCounter.init();
    mDistanceCounter.init();
    mSpeedCounter.init(0.5f, 300.0f);
    mSpeedFilter.init(0.5f, 300.0f);    // same valid range as the raw counter
    mHrCounter.init(20.0f, 300.0f);
    mAltitudeCounter.init(2.0f);

    WristTiltDetector::Config config{};
    config.sampleRateHz = skFusionSampleRateHz;
    mWristTiltDetector.setConfig(config);

    mWristTiltDetector.setListener(this);

    std::memcpy(mHrThresholds, CustomMessage::kHrThresholdsDefault, sizeof(mHrThresholds));
}

Service::~Service()
{
    closeVo2Log(false);   // HybridX Run: keep what a stopped app had logged
    disconnect();   // Cleanup resources
}

void Service::run()
{
    LOG_INFO("Started\n");

    // Initialize time
    mTimeTracker.init();

    // Get settings
    if (!mSettingsSerializer.load(mSettings)) {
        LOG_WARNING("Failed to load settings\n");
    }

    // Get summary
    if (!mActivitySummarySerializer.load(mSummary)) {
        LOG_WARNING("Failed to load activity summary\n");
    }

    // HybridX Run: the athlete's numbers and the VO2max history.
    loadVo2();

    // Recover any activity a previous boot left unfinished (power loss /
    // crash mid-recording), before any new track can start.
    if (mActivityWriter.recoverInterrupted()) {
        LOG_INFO("Recovered an interrupted activity\n");
        notifyNewActivity();
    }

    SDK::Timer guiInitTimeout(TIMER_SECONDS(5));
    guiInitTimeout.start();

    bool firstFix = false;

    std::time_t processedUtc = 0;

    while (true) {
        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, 500)) {
            // Command handling
            switch (msg->getType()) {

                // Kernel messages
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    disconnect();   // Cleanup resources
                    if (mTrackState != Track::State::INACTIVE) {
                        stopTrack(false);
                    }
                    // We must release message because this is the last event.
                    mKernel.comm.releaseMessage(msg);
                    return;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                    LOG_INFO("GUI is now running\n");
                    onStartGUI();
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                    LOG_INFO("GUI has stopped\n");
                    onStopGUI();
                    break;

                // Custom messages
                case CustomMessage::SETTINGS_SAVE:  {
                    LOG_DEBUG("SETTINGS_SAVE\n");
                    handleEvent(*static_cast<CustomMessage::SettingsSave*>(msg));
                } break;

                case CustomMessage::VO2_PROFILE_SAVE:  {   // HybridX Run
                    handleEvent(*static_cast<CustomMessage::Vo2ProfileSave*>(msg));
                } break;

                case CustomMessage::TRACK_START:  {
                    LOG_DEBUG("TRACK_START\n");
                    handleEvent(*static_cast<CustomMessage::TrackStart*>(msg));
                } break;

                case CustomMessage::TRACK_STOP:  {
                    LOG_DEBUG("TRACK_STOP\n");
                    handleEvent(*static_cast<CustomMessage::TrackStop*>(msg));
                } break;

                case CustomMessage::TRACK_PAUSE:  {
                    LOG_DEBUG("TRACK_PAUSE\n");
                    handleEvent(*static_cast<CustomMessage::TrackPause*>(msg));
                } break;

                case CustomMessage::TRACK_RESUME:  {
                    LOG_DEBUG("TRACK_RESUME\n");
                    handleEvent(*static_cast<CustomMessage::TrackResume*>(msg));
                } break;

                case CustomMessage::MANUAL_LAP:  {
                    LOG_DEBUG("MANUAL_LAP\n");
                    handleEvent(*static_cast<CustomMessage::ManualLap*>(msg));
                } break;

                case CustomMessage::INTERVALS_NEXT_PHASE:  {
                    LOG_DEBUG("INTERVALS_NEXT_PHASE\n");
                    handleEvent(*static_cast<CustomMessage::IntervalsNextPhase*>(msg));
                } break;

                case CustomMessage::WORKOUT_DETAILS_REQUEST:  {
                    LOG_DEBUG("WORKOUT_DETAILS_REQUEST\n");
                    handleEvent(*static_cast<CustomMessage::WorkoutDetailsRequest*>(msg));
                } break;

                case CustomMessage::WORKOUT_END:  {
                    LOG_DEBUG("WORKOUT_END\n");
                    handleEvent(*static_cast<CustomMessage::WorkoutEnd*>(msg));
                } break;

                // Sensors messages
                case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                    auto event = static_cast<SDK::Message::Sensor::EventData*>(msg);
                    SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                    handleSensorsData(event->handle, batch);
                } break;

                // External accessory link status (WP-S4) -> forward to GUI for
                // the pre-activity HR indicator.
                case SDK::MessageType::EVENT_ACCESSORY_STATUS: {
                    auto* evt = static_cast<SDK::Message::Accessory::EventStatus*>(msg);
                    LOG_INFO("Accessory status: state %u\n", evt->state);
                    SDK::send_msg<CustomMessage::AccessoryStatusUpd>(mKernel, evt->state, evt->name);
                } break;

                default:
                    break;
            }
            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }


        // Periodic process
        if (mGuiStarted) {
            // Update time every second
            std::time_t utc = mTimeTracker.getExpectedUTC();

            if (processedUtc != utc) {
                processedUtc = utc;

                // GPS_LOCATION is subscribed once at GUI start so acquisition
                // begins on the pre-activity screen. That first attempt can lose
                // a ~100 ms connect race during app startup, which would strand
                // position logging for the entire session (distance/speed still
                // record via the kernel's own GPS_LOCATION listener, so the run
                // looks complete but has no map). Retry until it takes.
                if (mGpsWanted && !mSensorGpsLocation.isConnected()) {
                    connectGps();
                    if (mGpsInitialConnectFailed && mSensorGpsLocation.isConnected()) {
                        mGpsInitialConnectFailed = false;
                        LOG_INFO("GPS location subscription recovered after a lost startup connect\n");
                    }
                }

                // Send to GUI real "local time" to display
                std::tm tmNow = mTimeTracker.getLocalTime(std::time(nullptr));
                SDK::send_msg<CustomMessage::Time>(mKernel, tmNow);

                SDK::send_msg<CustomMessage::Battery>(mKernel, static_cast<uint8_t>(mBatterySoc.get()));

                // Update GPS fix
                if (mPreviousGpsFixState != mGps.fix) {
                    mPreviousGpsFixState = mGps.fix;

                    if (!firstFix) {
                        notifyFirstFix();
                        firstFix = true;
                    }
                    SDK::send_msg<CustomMessage::GpsFix>(mKernel, mGps.fix);
                }

                if (mTrackState != Track::State::INACTIVE) {
                    mTimeCounter.add(utc);
                    processTrack();
                }
            }
        } else {
            // Just wait some time to see if GUI starts
            if (guiInitTimeout.expired()) {
                LOG_INFO("No activities, exiting service\n");
                return; // Exit app
            }
        }

        // After the periodic update, so a step that changed on this pass has
        // already dropped the last step's repeat.
        sendAlertRepeat();
    }
}

void Service::connectGps()
{
    if (!mSensorGpsLocation.isConnected()) {
        LOG_DEBUG("Connect to GPS sensor...\n");
        mSensorGpsLocation.connect();
    }
}

void Service::connectSensors()
{
    // Idempotent + self-healing: connect only sensors not already connected,
    // so a subscribe that lost the ~100 ms ack race at track start is retried
    // (pumped from processTrack each tick) instead of dropped for the whole
    // session. Already-connected sensors are skipped, so there is no churn.
    if (!mSensorBatteryLevel.isConnected())   { mSensorBatteryLevel.connect(); }
    if (!mSensorBatteryMetrics.isConnected()) { mSensorBatteryMetrics.connect(); }
    if (!mSensorGpsSpeed.isConnected())       { mSensorGpsSpeed.connect(); }
    if (!mSensorGpsDistance.isConnected())    { mSensorGpsDistance.connect(); }
    if (!mSensorPressure.isConnected())       { mSensorPressure.connect(); }
    if (!mSensorHr.isConnected())             { mSensorHr.connect(); }
    if (!mSensorFusion.isConnected())         { mSensorFusion.connect(); }
    if (!mSensorRunningCadence.isConnected()) { mSensorRunningCadence.connect(); }
    if (!mSensorGrade.isConnected())          { mSensorGrade.connect(); }

    mIsSensorsConnected = true;
}

void Service::disconnect()
{
    if (mIsSensorsConnected) {
        LOG_DEBUG("Disconnect from sensors...\n");

        mSensorGrade.disconnect();
        mSensorFusion.disconnect();
        mSensorRunningCadence.disconnect();
        mSensorHr.disconnect();
        mSensorPressure.disconnect();
        mSensorGpsSpeed.disconnect();
        mSensorGpsDistance.disconnect();
        mSensorBatteryLevel.disconnect();
        mSensorBatteryMetrics.disconnect();

        mIsSensorsConnected = false;
    }

    // The activity is over (stopTrack) or the app is stopping: GPS is no longer
    // wanted, so the run() retry must not re-wake it. Release unconditionally --
    // disconnect() is a no-op if never subscribed, and firing it whenever a
    // handle is held is what releases a listener whose connect-ack timed out
    // (isConnected() would be false in exactly that case).
    mGpsWanted = false;
    LOG_DEBUG("Disconnect from GPS sensor...\n");
    mSensorGpsLocation.disconnect();
}


void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data)
{
    if (mSensorGpsLocation.matchesDriver(handle)) {
        SDK::SensorDataParser::GpsLocation parser(data[0]);
        if (parser.isDataValid()) {
            mGps.timestamp = parser.getTimestamp();
            mGps.fix = parser.isCoordinatesValid();

            if (mGps.fix) { // Do not change position if no fix
                parser.getCoordinates(mGps.latitude, mGps.longitude, mGps.altitude);
                mGpsPosFresh = true;    // consumed by the speed filter each tick
            }
            mGpsCatchUp.onLocation(mGps.fix, parser.getTimestamp());
            LOG_DEBUG("Location: fix %u, lat %f, lon %f\n", mGps.fix, mGps.latitude, mGps.longitude);
        }
    } else if (mSensorGpsSpeed.matchesDriver(handle)) {
        SDK::SensorDataParser::GpsSpeed parser(data[0]);
        if (parser.isDataValid()) {
            mGpsSpeedMs       = parser.getSpeed();  // raw instantaneous speed
            mGpsSpeedValid    = parser.isSpeedValid();
            mGpsDeadReckoning = parser.isDeadReckoning();
            mGpsSpeedFresh    = true;   // consumed by the speed filter each tick
            mSpeedLastMs      = mKernel.sys.getTimeMs();   // HybridX Run
            mSpeedSeen        = true;
            if (mGpsSpeedValid && mGpsSpeedMs > mGpsSpeedPeakMs &&
                mGpsSpeedMs <= mSpeedCounter.getMaxValid()) {
                mGpsSpeedPeakMs = mGpsSpeedMs;   // for the maxima; see processTrack()
            }
            LOG_DEBUG("Speed:    %.2f m/s (valid %u, dr %u)\n",
                      mGpsSpeedMs, mGpsSpeedValid, mGpsDeadReckoning);
        }
    } else if (mSensorGrade.matchesDriver(handle)) {
        SDK::SensorDataParser::Grade parser(data[0]);
        if (parser.isDataValid()) {
            mGradeData.gradePct   = parser.getGradePct();
            mGradeData.gradeValid = parser.isGradeValid();
            mGradeLastMs          = mKernel.sys.getTimeMs();   // HybridX Run
            mGradeSeen            = true;
            LOG_DEBUG("Grade:    %.2f %% (valid %u)\n",
                      mGradeData.gradePct, mGradeData.gradeValid);
        }
    } else if (mSensorGpsDistance.matchesDriver(handle)) {
        SDK::SensorDataParser::GpsDistance parser(data[0]);
        if (parser.isDataValid()) {
            mDistanceCounter.add(parser.getDistance());
            mGpsCatchUp.onDistance(parser.getTimestamp());
            LOG_DEBUG("Distance: %.2f m\n", parser.getDistance());
        }
    } else if (mSensorPressure.matchesDriver(handle)) {
        SDK::SensorDataParser::Pressure parser(data[0]);
        if (parser.isDataValid()) {
            if (!mAltitudeCounter.isValid()) {
                mSeaLevelPressure = parser.getP0();
            }
            float altitude = parser.getAltitude(parser.getPressure(), mSeaLevelPressure);
            float filtered = mAltitudeFilter.execute(altitude);
            mAltitudeCounter.add(filtered);

            LOG_DEBUG("Altitude %.2f (Filtered %.2f) (P0 %f, Pa %f)\n", altitude, filtered, mSeaLevelPressure, parser.getPressure());
        }
    } else if (mSensorHr.matchesDriver(handle)) {
        SDK::SensorDataParser::HeartRateEx parser(data[0]);
        if (parser.isDataValid()) {
            mHrCounter.add(parser.getBpm());           // arbitrated (kernel's choice)
            mTrackData.hrTrustLevel = parser.getTrustLevel();
            mHrSource     = static_cast<uint8_t>(parser.getSource());
            mHrOpticalBpm = static_cast<uint8_t>(parser.getOpticalBpm());
            mHrExternalBpm= static_cast<uint8_t>(parser.getExternalBpm());
            mHrLastMs     = mKernel.sys.getTimeMs();   // HybridX Run
            mHrSeen       = true;
            LOG_DEBUG("HR %.1f trust %.1f src %u (opt %u ext %u)\n",
                      parser.getBpm(), parser.getTrustLevel(), mHrSource,
                      mHrOpticalBpm, mHrExternalBpm);
        }
    } else if (mSensorHrMetrics.matchesDriver(handle)) {
        // HybridX Run: the watch's daily resting HR, for the VO2max estimate.
        SDK::SensorDataParser::HeartRateMetrics parser(data[0]);
        if (parser.isDataValid()) {
            // Only a plausible figure replaces the last one: the daily value
            // reads 0 after midnight until today's exists, and a run that
            // crosses midnight must not lose it. Kept in vo2.json too, for a
            // run that ends before the sensor first reports.
            const float   rhr = parser.getRhr();
            const uint8_t bpm = (rhr > 0.0f && rhr < 255.0f) ? static_cast<uint8_t>(rhr + 0.5f) : 0;
            if (RunVo2::plausibleRestHr(bpm)) {
                if (bpm != mVo2Input.watchRestHr) {
                    LOG_INFO("Resting HR from the watch: %u\n", bpm);
                }
                mVo2Input.watchRestHr = bpm;
                mVo2History.setWatchRestHr(bpm);
            } else {
                LOG_DEBUG("Resting HR from the watch ignored: %u\n", bpm);
            }
        }
    } else if (mSensorBatteryLevel.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryLevel parser(data[0]);
        if (parser.isDataValid()) {
            mBatterySoc.set(parser.getCharge());
            LOG_DEBUG("Battery %.1f %%\n", mBatterySoc.get());
        }
    } else if (mSensorBatteryMetrics.matchesDriver(handle)) {
        SDK::SensorDataParser::BatteryMetrics parser(data[0]);
        if (parser.isDataValid()) {
            mBatteryVoltage.set(parser.getVoltage());
            LOG_DEBUG("Battery voltage %.1f V\n", mBatteryVoltage.get());
        }
    } else if (mSensorWristMotion.matchesDriver(handle)) {
        SDK::SensorDataParser::WristMotion parser(data[0]);
        if (parser.isDataValid()) {
            LOG_DEBUG("Wrist Motion detected\n");
            backlightOn();
        }
    } else if (mSensorRunningCadence.matchesDriver(handle)) {
        SDK::SensorDataParser::RunningCadence parser(data[0]);
        if (parser.isDataValid()) {
            mRunningCadence.cadenceSpm   = parser.getCadenceSpm();
            mRunningCadence.cadenceValid = parser.isCadenceValid();
        }
    } else if (mSensorFusion.matchesDriver(handle)) {
        static constexpr uint16_t kBatchSize = 10u;
        TiltImuSample batch[kBatchSize];
        uint16_t batchLen = 0;

        for (uint16_t i = 0; i < data.size(); i++) {
            SDK::SensorDataParser::FusionRaw parser(data[i]);
            if (parser.isDataValid()) {
                SDK::SensorDataParser::FusionRaw::Data sample{};
                parser.getData(sample);
                batch[batchLen].ayLsb = sample.accel.y;
                batch[batchLen].azLsb = sample.accel.z;
                batch[batchLen].gxLsb = sample.gyro.x;
                batch[batchLen].timestampMs = parser.getTimestamp();
                //LOG_DEBUG("AY: %d, GX: %d\n", sample.accel.y, sample.gyro.x);
                ++batchLen;

                if (batchLen == kBatchSize) {
                    mWristTiltDetector.addBatch(batch, kBatchSize);
                    batchLen = 0;
                }
            }
        }

        if (batchLen > 0u) {
            mWristTiltDetector.addBatch(batch, batchLen);
        }
    }
}


void Service::onStartGUI()
{
    mGuiStarted = true;

    setCapabilities();
    requestAccessoryPrepare();   // pre-warm external HR while on the pre-activity screen

    // GPS stays wanted from the pre-activity screen until the activity ends
    // (cleared in disconnect()), so the run() loop keeps it connected during the
    // activity but never re-wakes the GNSS on the post-activity summary screen.
    mGpsWanted = true;

    // Subscribe to GPS to get fix. If this first attempt loses the ~100 ms
    // startup ack race, the run() loop retries; track the outcome so the retry
    // logs the recovery (and field logs reveal how often the race fires).
    connectGps();
    mGpsInitialConnectFailed = !mSensorGpsLocation.isConnected();
    if (mGpsInitialConnectFailed) {
        LOG_WARNING("GPS location subscribe lost the startup race; will retry\n");
    }

    mSensorWristMotion.connect();
    mSensorHrMetrics.connect();   // HybridX Run: resting HR

    sendInitialInfoToGui();
    scanWorkouts();
    sendWorkoutList();
}

void Service::onStopGUI()
{
    mGuiStarted = false;

    requestAccessoryRelease();
    mSensorWristMotion.disconnect();
    mSensorHrMetrics.disconnect();
}

void Service::handleEvent(const CustomMessage::TrackStart& event)
{
    // We can synchronize the time because we haven't started the track yet,
    // and the GPS could have already updated the current time.
    mTimeTracker.init();

    // A workout from the list replaces any armed intervals.
    mWorkoutRequested = event.workout;
    mIntervalsMode    = event.intervalsMode && event.workout == CustomMessage::TrackStart::kNoWorkout;

    startTrack(mTimeTracker.getExpectedUTC());
}

void Service::handleEvent(const CustomMessage::TrackStop& event)
{
    stopTrack(event.discard);
    // After a run, a completed workout is no longer today's to offer. The
    // list itself is left as it is: the GUI may be reading it.
    sendWorkoutList();
}

void Service::handleEvent(const CustomMessage::SettingsSave& event)
{
    bool updCaps = mSettings.phoneNotifEn != event.settings.phoneNotifEn;
    mSettings = event.settings;
    mSettingsSerializer.save(event.settings);

    if (updCaps) {
        setCapabilities();
    }
}

void Service::handleEvent(const CustomMessage::TrackPause& /*event*/)
{
    pauseTrack(true);
}

void Service::handleEvent(const CustomMessage::TrackResume& /*event*/)
{
    pauseTrack(false);
}

void Service::handleEvent(const CustomMessage::ManualLap& /*event*/)
{
    // In a workout each step is a lap: the lap button moves to the next.
    if (mWorkoutMode) {
        nextStep();
        return;
    }
    saveLap(LapEnd{});
    SDK::send_msg<CustomMessage::LapEnded>(mKernel, mTrackData.lapNum);
    notifyLapEnd();
}

void Service::setCapabilities()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::RequestSetCapabilities>();
    if (msg) {
        msg->enPhoneNotification = mSettings.phoneNotifEn;
        msg->enUsbChargingScreen = false;
        msg->enMusicControl = true;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::requestAccessoryPrepare()
{
    // Pre-acquire an external HR strap at the pre-activity screen (sent at
    // onStartGUI). No-op kernel-side unless external HR is enabled in Settings.
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::Accessory::RequestPrepare>();
    if (msg) {
        msg->kinds = SDK::Accessory::Kind::HRM;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::requestAccessoryRelease()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::Accessory::RequestRelease>();
    if (msg) {
        msg->kinds = 0;   // release everything we acquired
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}


void Service::notifyFirstFix()
{
    backlightOn();
    playBuzzerPattern(150, 3);
    playVibroPattern(SDK::Message::RequestVibroPlay::Effect::STRONG_CLICK_100);
}

void Service::notifyLapEnd()
{
    backlightOn();
    playBuzzerPattern(150, 3);
    playVibroPattern(SDK::Message::RequestVibroPlay::Effect::ALERT_750MS_100);
}

void Service::notifyNewActivity()
{
    auto *msg = mKernel.comm.allocateMessage<SDK::Message::CommandAppNewActivity>();
    if (msg) {
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::backlightOn(uint32_t timeoutMs)
{
    auto bl = SDK::make_msg<SDK::Message::RequestBacklightSet>(mKernel);
    if (bl) {
        bl->brightness       = 100;
        bl->autoOffTimeoutMs = timeoutMs;
        bl.send();
    }
}

void Service::playBuzzerPattern(uint16_t beepMs, uint8_t count, uint16_t silenceMs, uint8_t volume)
{
    if (count == 0) {
        return;
    }

    // A series of N beeps needs 2*N-1 notes (beeps + silences between them).
    // Cap to what fits in skMaxNotes: max count = (skMaxNotes + 1) / 2 = 5.
    const uint8_t maxCount = (SDK::Message::RequestBuzzerPlay::skMaxNotes + 1u) / 2u;
    if (count > maxCount) {
        count = maxCount;
    }

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestBuzzerPlay>();
    if (msg) {
        uint8_t n = 0;
        for (uint8_t i = 0; i < count; ++i) {
            msg->notes[n].volume = volume;
            msg->notes[n].time = beepMs;
            ++n;
            if (i < count - 1u) {
                msg->notes[n].volume = 0;
                msg->notes[n].time = silenceMs;
                ++n;
            }
        }
        msg->notesCount = n;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::playBuzzerNotes(const SDK::Message::RequestBuzzerPlay::Note notes[], uint8_t count)
{
    if (count > SDK::Message::RequestBuzzerPlay::skMaxNotes) {
        count = SDK::Message::RequestBuzzerPlay::skMaxNotes;
    }

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestBuzzerPlay>();
    if (msg) {
        for (uint8_t i = 0; i < count; ++i) {
            msg->notes[i] = notes[i];
        }
        msg->notesCount = count;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::playVibroPattern(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count, uint16_t silenceMs)
{
    if (count == 0) {
        return;
    }

    // A series of N effects needs 2*N-1 notes (effects + silences between them).
    // Cap to what fits in skMaxNotes: max count = (skMaxNotes + 1) / 2 = 4.
    const uint8_t maxCount = (SDK::Message::RequestVibroPlay::skMaxNotes + 1u) / 2u;
    if (count > maxCount) {
        count = maxCount;
    }

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestVibroPlay>();
    if (msg) {
        uint8_t n = 0;
        for (uint8_t i = 0; i < count; ++i) {
            msg->notes[n].effect = static_cast<uint8_t>(effect);
            msg->notes[n].pause = 0;
            ++n;
            if (i < count - 1u) {
                msg->notes[n].effect = static_cast<uint8_t>(SDK::Message::RequestVibroPlay::Effect::NO_EFFECT);
                msg->notes[n].pause = silenceMs;
                ++n;
            }
        }
        msg->notesCount = n;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}


ActivityWriter::RecordData Service::prepareRecordData()
{
    ActivityWriter::RecordData fitRecord{};

    fitRecord.timestamp    = mTimeCounter.getCurrent();

    fitRecord.set(ActivityWriter::RecordData::Field::COORDS, mGps.fix);
    fitRecord.latitude     = mGps.latitude;
    fitRecord.longitude    = mGps.longitude;

    // The corrected speed of this tick rather than the display window, so the
    // records agree with max_speed. It is still the receiver's 10 s average.
    fitRecord.set(ActivityWriter::RecordData::Field::SPEED, mSpeedFilter.hasCurrentSample());
    fitRecord.speed        = mSpeedFilter.getInstantSpeed();

    fitRecord.set(ActivityWriter::RecordData::Field::ALTITUDE, mAltitudeCounter.isValid());
    fitRecord.altitude     = mAltitudeCounter.getCurrent();

    const bool hasHeartRate = hasTrustedHr();
    fitRecord.set(ActivityWriter::RecordData::Field::HEART_RATE, hasHeartRate);
    fitRecord.heartRate    = mHrCounter.getCurrent();
    // Tag each record with where the HR came from (none when no valid HR).
    fitRecord.hrSource     = hasHeartRate ? mHrSource : 0;
    fitRecord.hrOpticalBpm = mHrOpticalBpm;
    fitRecord.hrExternalBpm= mHrExternalBpm;

    // Both samples must be checked every call; evaluate separately to avoid short-circuit.
    const bool socReady     = mBatterySoc.isDue();
    const bool voltReady    = mBatteryVoltage.isDue();
    const bool batteryReady = socReady && voltReady;
    if (batteryReady) {
        mBatterySoc.consume();
        mBatteryVoltage.consume();
    }
    fitRecord.set(ActivityWriter::RecordData::Field::BATTERY, batteryReady);
    fitRecord.batteryLevel   = static_cast<uint8_t>(mBatterySoc.get());
    fitRecord.batteryVoltage = static_cast<uint16_t>(mBatteryVoltage.get() * 1000);

    fitRecord.set(ActivityWriter::RecordData::Field::CADENCE, mRunningCadence.cadenceValid);
    fitRecord.cadenceSpm = mRunningCadence.cadenceSpm;

    // Implied step length is derived SDK-side from GPS speed + cadence (the
    // kernel no longer emits it). Gated to 0.15-2.50 m, preserving the prior
    // record.step_length values.
    const SDK::Calibration::StrideMath::StepLength stepLen =
        SDK::Calibration::StrideMath::impliedStepLengthM(
            mSpeedFilter.getInstantSpeed(),
            mSpeedFilter.hasCurrentSample() && !mGpsDeadReckoning,
            mRunningCadence.cadenceSpm, mRunningCadence.cadenceValid);
    fitRecord.set(ActivityWriter::RecordData::Field::STEP_LENGTH, stepLen.valid);
    fitRecord.stepLengthM = stepLen.meters;

    return fitRecord;
}

void Service::sendInitialInfoToGui()
{
    // Settings
    uint8_t hrThresholds[CustomMessage::kHrThresholdsCount];
    memcpy(hrThresholds, CustomMessage::kHrThresholdsDefault, sizeof(hrThresholds));

    if (auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel)) {
        if (msg.send(100) && msg.ok()) {
            mIsImperial = msg->imperialUnits;
            mTimeFormat12h = msg->timeFormat;

            if (msg->heartRateCount > CustomMessage::kHrThresholdsCount) {
                msg->heartRateCount = CustomMessage::kHrThresholdsCount;
            }

            if (msg->heartRateCount > 0) {
                // Copy received elements
                uint8_t i = 0;
                for (; i < msg->heartRateCount; ++i) {
                    hrThresholds[i] = msg->heartRateTh[i];
                }

                // Complete the array elements to the full number
                for (; i < CustomMessage::kHrThresholdsCount; ++i) {
                    if (i > 0) {
                        hrThresholds[i] = hrThresholds[i - 1] + 20;
                    } else {
                        hrThresholds[i] = CustomMessage::kHrThresholdsDefault[0];
                    }
                }
            }
        }
    }

    std::memcpy(mHrThresholds, hrThresholds, sizeof(mHrThresholds));
    SDK::send_msg<CustomMessage::SettingsUpd>(mKernel, mSettings, mIsImperial, mTimeFormat12h, hrThresholds, CustomMessage::kHrThresholdsCount);
    SDK::send_msg<CustomMessage::Summary>(mKernel, &mSummary);
    SDK::send_msg<CustomMessage::Battery>(mKernel, static_cast<uint8_t>(mBatterySoc.get()));
    sendVo2Info();
}

void Service::startTrack(std::time_t utc)
{
    // Reset data
    mTrackData = {};

    mTimeCounter.reset();
    mTimeCounter.add(utc);

    mDistanceCounter.reset();
    mSpeedCounter.reset();
    mSpeedFilter.reset();
    mHrCounter.reset();
    mHrSource = 0;  // don't carry a prior track's HR source/readings into the new session
    mHrOpticalBpm = 0;
    mHrExternalBpm = 0;
    mAltitudeFilter.reset();
    mAltitudeCounter.reset();
    mBatterySoc.reset(skBatteryLogPeriodMs);
    mBatteryVoltage.reset(skBatteryLogPeriodMs);
    mGps.reset();
    mRunningCadence = {};

    // Outdoor stride calibrator: reset latched inputs and load the stored LUT
    // for this session.
    mGradeData = {};
    mGpsSpeedValid    = false;
    mGpsSpeedFresh    = false;
    mGpsPosFresh      = false;
    mGpsSpeedPeakMs   = 0.0f;
    mGpsDeadReckoning = false;
    mGpsCatchUp.reset();
    mLastCalibUtc     = 0;
    mCalibrator.load();
    if (mSettings.calibTraceEn) {
        // Debug: per-tick CSV trace pulled over USB mass storage.
        mCalibrator.enableTrace("../SharedData/stride_trace.csv");
    }

    mSessionNotEmpty = false;
    mLapNotEmpty = false;
    mLapCarryMs = 0;

    // HybridX Run: a fresh VO2max run; the last run's figures leave the screens.
    mVo2Run.reset();
    mVo2Info.hasRun = false;
    sendVo2Info();
    openVo2Log(utc);

    // Intervals mode initialization
    mIntervalsCompleted = false;
    mTrackData.intervalsMode = mIntervalsMode;
    if (mIntervalsMode) {
        startIntervals();
    }

    // A structured workout from the list
    mWorkoutMode       = false;
    mWorkoutReachedEnd = false;
    mWorkoutDate       = 0;
    mWorkoutFile[0]    = '\0';
    if (!mIntervalsMode && mWorkoutRequested >= 0) {
        mWorkoutMode = startWorkout(static_cast<uint16_t>(mWorkoutRequested));
    }
    mTrackData.workoutMode = mWorkoutMode;
    if (mWorkoutMode) {
        sendWorkoutSteps();
    }

    // Each workout step is a lap: reserve them all up front.
    std::size_t lapReserve = skLapReserve;
    if (mIntervalsMode || mWorkoutMode) {
        const uint64_t steps = SDK::Workout::totals(mProgram).stepsRun;
        lapReserve += static_cast<std::size_t>(
            std::min<uint64_t>(steps, skMaxWorkoutLapReserve));
    }
    mSummary = ActivitySummary{};
    mSummary.laps.reserve(lapReserve);

    // Configure TrackMapBuilder
    mTrackMapBuilder.reset();
    SDK::TrackMapBuilder::GpsPoint startGpsPoint{ mGps.latitude, mGps.longitude };
    mTrackMapBuilder.setDistanceThreshold(startGpsPoint, skMapDistanceThreshold);

    // Determine lap split source. In intervals mode each phase (warm up / run /
    // rest / cool down), and in a workout each step, is recorded as its own lap,
    // so the alert-based auto-lap is disabled to avoid splitting one across laps.
    mLapDivSource = (mIntervalsMode || mWorkoutMode) ? LapDivSource::OFF : getLapDivSource();

    mWristTiltDetector.reset();

    connectSensors();

    ActivityWriter::AppInfo info{};
    info.timestamp = utc;
    info.appVersion = SDK::ParseVersion(BUILD_VERSION).u32;
    info.devID = DEV_ID;
    info.appID = APP_ID;
    mActivityWriter.start(info);

    if (mIntervalsMode || mWorkoutMode) {
        mActivityWriter.addWorkout(mProgram);
    }

    mTrackState = Track::State::ACTIVE;

    SDK::send_msg<CustomMessage::TrackStateUpd>(mKernel, mTrackState);
}

void Service::processTrack()
{
    LOG_DEBUG("Time: %u / %u\n", static_cast<uint32_t>(mTimeCounter.getValueActive()), static_cast<uint32_t>(mTimeCounter.getValueTotal()));

    // Retry any track-sensor subscription that lost the connect-ack race at
    // track start. connectSensors() is idempotent, so this is a cheap no-op
    // once everything is connected, and it runs only while the track is active
    // (processTrack) so it never re-powers sensors after the track ends.
    connectSensors();

    // Creating map
    SDK::TrackMapBuilder::GpsPoint newPoint{ mGps.latitude, mGps.longitude };
    // Add point to the track map
    if (mGps.fix && mTrackState == Track::State::ACTIVE) {
        mTrackMapBuilder.addPoint(newPoint);
    }

    // Time, s
    mTrackData.totalTime = mTimeCounter.getValueActive();
    mTrackData.lapTime = mTimeCounter.getLapValueActive();

    // Distance, m
    mTrackData.distance = mDistanceCounter.getValueActive();
    mTrackData.lapDistance = mDistanceCounter.getLapValueActive();

    // Speed, m/s
    //
    // The live speed and pace shown to the user are the rolling-window mean of
    // the GPS speed, not the latest sample: a single sample's noise moves the
    // pace readout by tens of seconds per kilometre, which is unusable for
    // holding a target pace. Fed here rather than from the GPS_SPEED callback so
    // the window advances once per track tick even when a sample is missing,
    // which ages a lost fix out of the window instead of holding it forward.
    // Advanced only while ACTIVE, so the readout freezes for the duration of a
    // pause. That is a deliberate change: VariableCounter::add() latches its
    // current value before its own pause check, so the old readout went on
    // tracking the raw speed of a standing runner while the activity was paused.
    // The counter is fed from here rather than from the sensor callback so that
    // every statistic derives from the corrected speed, at a uniform 1 Hz. The
    // INSTANT value goes in, not the smoothed one, so a maximum keeps its peak.
    if (mTrackState == Track::State::ACTIVE) {
        mSpeedFilter.tick(mGpsSpeedMs, mGpsSpeedValid && mGpsSpeedFresh,
                          mGps.latitude, mGps.longitude, mGps.fix && mGpsPosFresh);
        if (mSpeedFilter.hasCurrentSample()) {
            // A tick can see two samples and the filter keeps the newest, so the
            // maxima take the higher of it and the tick's peak, on the same scale.
            const float peak = mGpsSpeedPeakMs * mSpeedFilter.getScale();
            mSpeedCounter.add(std::max(mSpeedFilter.getInstantSpeed(), peak));
        }
    }
    // Consumed every tick, paused or not, so a sample counts as fresh only on the
    // tick that follows it -- including the first tick after a resume.
    mGpsSpeedFresh  = false;
    mGpsPosFresh    = false;
    mGpsSpeedPeakMs = 0.0f;
    mTrackData.speed = mSpeedFilter.getSpeed();

    mTrackData.avgSpeed    = speedFromTotals(mTrackData.distance, mTrackData.totalTime);
    mTrackData.maxSpeed    = mSpeedCounter.getMaximum();
    mTrackData.avgLapSpeed = speedFromTotals(mTrackData.lapDistance, mTrackData.lapTime);
    mTrackData.maxLapSpeed = mSpeedCounter.getLapMaximum();


    // Pace, s/m
    const float kMinSpeed = mSpeedCounter.getMinValid();
    mTrackData.pace = mSpeedFilter.getPace();
    mTrackData.avgPace = getPace(mTrackData.avgSpeed, kMinSpeed);
    mTrackData.lapPace = getPace(mTrackData.avgLapSpeed, kMinSpeed);


    // HR
    mTrackData.hr = mHrCounter.getCurrent();
    mTrackData.hrSource = mHrSource;  // for the in-activity source-driven HR icon
    mTrackData.avgHR = mHrCounter.getAverage();
    mTrackData.maxHR = mHrCounter.getMaximum();
    mTrackData.avgLapHR = mHrCounter.getLapAverage();
    mTrackData.maxLapHR = mHrCounter.getLapMaximum();


    // Altitude, m
    mTrackData.elevation = mAltitudeCounter.getCurrent();

    // HybridX Run: one second of the VO2max windows (a pause breaks a window).
    feedVo2();

    // Intervals state machine - only while actively running (not paused)
    if (mIntervalsMode && mTrackState == Track::State::ACTIVE) {
        processIntervals();
    }
    if (mWorkoutMode && mTrackState == Track::State::ACTIVE) {
        processWorkout();
    }

    // Update GUI
    SDK::send_msg<CustomMessage::TrackDataUpd>(mKernel, mTrackData);


    if (mTrackState == Track::State::ACTIVE) {
        // Feed the outdoor stride calibrator from the latest latched values,
        // inline at the 1 Hz record-write point and before the FIT write.
        {
            SDK::Calibration::CalibratorSample cs;
            cs.gps_speed_ms           = mSpeedFilter.getInstantSpeed();
            cs.gps_speed_valid        = mSpeedFilter.hasCurrentSample();
            cs.gps_fix_dead_reckoning = mGpsDeadReckoning;
            cs.cadence_spm            = mRunningCadence.cadenceSpm;
            cs.cadence_valid          = mRunningCadence.cadenceValid;
            cs.grade_pct              = mGradeData.gradePct;
            cs.grade_valid            = mGradeData.gradeValid;

            const std::time_t nowUtc = mTimeCounter.getCurrent();
            cs.delta_t_s = (mLastCalibUtc == 0)
                ? 1.0f
                : static_cast<float>(nowUtc - mLastCalibUtc);
            mLastCalibUtc = nowUtc;

            mCalibrator.ingestSample(cs);
        }

        // Save record to the FIT file
        ActivityWriter::RecordData fitRecord = prepareRecordData();
        mActivityWriter.addRecord(fitRecord);

        mSessionNotEmpty = true;    // Session has at least one record
        mLapNotEmpty = true;        // Lap has at least one record

        // Next lap
        bool  switchLap        = false;
        float autoLapDistanceM = 0.0f;  // >0 marks a grid-aligned distance auto-lap
        switch (mLapDivSource) {
        case LapDivSource::DISTANCE: {
            const float target = Settings::Alerts::Distance::toMeters(mSettings.alertDistanceId, mIsImperial);
            if (mDistanceCounter.getLapValueActive() >= target) {
                switchLap        = true;
                autoLapDistanceM = target;
            }
            break;
        }
        case LapDivSource::TIME:
            switchLap = static_cast<uint32_t>(mTimeCounter.getLapValueActive()) >= Settings::Alerts::Time::toSeconds(mSettings.alertTimeId);
            break;

        case LapDivSource::OFF:
        default:
            break;
        }

        if (switchLap) {
            // A distance auto-lap is recorded at exactly the target distance,
            // with the overshoot carried into the next lap. Before saveLap()
            // (which increments lapNum and resets the lap counters), refresh the
            // lap fields the lap-alert popup reads so its pace is computed over
            // that same grid distance -- the live snapshot pushed earlier this
            // tick holds the small overshoot, whose pace would disagree with the
            // lap's whole-second duration. lapTime is still the completed lap's.
            if (autoLapDistanceM > 0.0f) {
                mTrackData.lapDistance = autoLapDistanceM;
                mTrackData.avgLapSpeed = speedFromTotals(autoLapDistanceM,
                                                         static_cast<float>(mTrackData.lapTime));
                mTrackData.lapPace     = getPace(mTrackData.avgLapSpeed, mSpeedCounter.getMinValid());
                SDK::send_msg<CustomMessage::TrackDataUpd>(mKernel, mTrackData);
            }

            LapEnd end;
            end.trigger = mLapDivSource == LapDivSource::DISTANCE
                              ? SDK::Fit::LapTrigger::Distance
                              : SDK::Fit::LapTrigger::Time;
            saveLap(end, autoLapDistanceM);
            SDK::send_msg<CustomMessage::LapEnded>(mKernel, mTrackData.lapNum);
            notifyLapEnd();
        }

    }

}

void Service::saveLap(const LapEnd& end, float autoLapDistanceM)
{
    // The time counter counts whole seconds. A workout step that ran out
    // ended between two of them: what came after its end (end.carryMs) goes
    // to the next lap, and this lap gets what the last one handed it.
    const uint32_t countedMs = static_cast<uint32_t>(mTimeCounter.getLapValueActive()) * 1000u + mLapCarryMs;
    const uint32_t lapMs     = countedMs > end.carryMs ? countedMs - end.carryMs : 0u;
    const float    lapTime   = static_cast<float>(lapMs) / 1000.0f;
    const int32_t  shiftMs   = static_cast<int32_t>(mLapCarryMs) - static_cast<int32_t>(end.carryMs);
    mLapCarryMs = end.carryMs;

    // A distance auto-lap (autoLapDistanceM > 0) is recorded as exactly the
    // target distance; the overshoot is carried into the next lap below. This
    // keeps lap boundaries on the km/mi grid and makes the reported lap pace
    // agree with the lap duration. A workout step that ran out does the same
    // with what was run beyond its end.
    const bool  gridLap     = autoLapDistanceM > 0.0f;
    const bool  stepLap     = end.ranOut;
    const float counted     = mDistanceCounter.getLapValueActive();
    const float lapDistance = gridLap ? autoLapDistanceM
                            : stepLap ? std::min(end.stepM, counted)
                                      : counted;
    const float lapSpeed    = speedFromTotals(lapDistance, lapTime);

    // Accumulate lap into summary. Its whole seconds are rounded, as the
    // summary rounds the pace, so a 1 km lap's time and pace read the same.
    mSummary.laps.push_back({
        static_cast<std::time_t>((lapMs + 500u) / 1000u),
        lapDistance,
        getPace(lapSpeed, mSpeedCounter.getMinValid())
    });

    // Save lap to the FIT file
    ActivityWriter::LapData fitLap{};

    // Every user stop pauses first -- the GUI pauses on entering the stop menu
    // and the confirm needs a hold -- so the span between that pause and the
    // save is UI time, not activity time. End at the pause instant and trim the
    // same tail from the elapsed span. A mid-activity lap is not paused, so both
    // calls are no-ops there -- though only the GUI guarantees that: the
    // ManualLap handler and the intervals phase advance do not check the state.
    const std::time_t endUtc  = mTimeCounter.getEndValue();
    const std::time_t tailSec = mTimeCounter.getTrailingPause();

    fitLap.timestamp = endUtc;
    fitLap.timeStart = mTimeCounter.getCurrent() - mTimeCounter.getLapValueTotal();
    fitLap.duration  = lapTime;
    fitLap.elapsed   = std::max(static_cast<float>(mTimeCounter.getLapValueTotal() - tailSec)
                                    + static_cast<float>(shiftMs) / 1000.0f,
                                0.0f);

    fitLap.distance  = lapDistance;

    fitLap.speedAvg  = lapSpeed;
    fitLap.speedMax  = mSpeedCounter.getLapMaximum();

    fitLap.hrAvg     = mHrCounter.getLapAverage();
    fitLap.hrMax     = mHrCounter.getLapMaximum();

    fitLap.ascent    = mAltitudeCounter.getLapAscent();
    fitLap.descent   = mAltitudeCounter.getLapDescent();

    fitLap.wktStepIndex = end.wktStepIndex;
    fitLap.intensity    = end.intensity;
    fitLap.trigger      = end.trigger;

    mActivityWriter.addLap(fitLap);
    mTrackData.lapNum++;

    LOG_INFO("Lap_%u saved. UTC: %u\n", mTrackData.lapNum, static_cast<uint32_t>(mTimeCounter.getCurrent()));
    LOG_INFO("Time: %.3f / %u s\n", lapTime, static_cast<uint32_t>(mTimeCounter.getLapValueTotal()));
    LOG_INFO("Distance: %.3f m\n", lapDistance);
    LOG_INFO("Speed: %.3f / %.3f m/s\n", lapSpeed, mSpeedCounter.getLapMaximum());
    LOG_INFO("Heart rate: %.0f / %.0f bpm\n", mHrCounter.getLapAverage(), mHrCounter.getLapMaximum());
    LOG_INFO("Ascent/Descent: %.1f / %.1f m\n", mAltitudeCounter.getLapAscent(), mAltitudeCounter.getLapDescent());

    // Reset lap counters
    mTimeCounter.resetLap();
    if (gridLap || stepLap) {
        mDistanceCounter.advanceLap(lapDistance);
    } else {
        mDistanceCounter.resetLap();
    }
    mSpeedCounter.resetLap();
    mHrCounter.resetLap();
    mAltitudeCounter.resetLap();

    // Clear track data
    mTrackData.lapTime = 0;
    mTrackData.lapDistance = 0.0f;
    mTrackData.maxLapSpeed = 0.0f;
    mTrackData.avgLapSpeed = 0.0f;
    mTrackData.avgLapHR = 0.0f;
    mTrackData.maxLapHR = 0.0f;

    mLapNotEmpty = false;
}

void Service::buildPartialSummary()
{
    // Same end instant as the FIT session, so the .json summary and the
    // .fit for one activity do not disagree by the trimmed tail.
    mSummary.utc       = mTimeCounter.getEndValue();
    mSummary.time      = mTimeCounter.getValueActive();
    mSummary.distance  = mDistanceCounter.getValueActive();
    mSummary.speedAvg  = speedFromTotals(mSummary.distance, mSummary.time);
    mSummary.elevation = mAltitudeCounter.getAscent();
    mSummary.paceAvg   = getPace(mSummary.speedAvg, mSpeedCounter.getMinValid());
    mSummary.hrMax     = mHrCounter.getMaximum();
    mSummary.hrAvg     = mHrCounter.getAverage();
    mSummary.map       = mTrackMapBuilder.build(skMapMaxPoints);
}

void Service::stopTrack(bool discard)
{
    if (mTrackState == Track::State::INACTIVE) {
        return;
    }

    if (!discard && mSessionNotEmpty) {

        if (mTrackState != Track::State::PAUSED) {
            mActivityWriter.pause(mTimeCounter.getCurrent());
        }

        if (mLapNotEmpty) {
            // The lap that ends the activity: a workout step's, if one is
            // still running.
            const LapEnd end = (mIntervalsMode || mWorkoutMode) && mEngine.status().running
                ? workoutLap(mEngine.status().step, SDK::Fit::LapTrigger::SessionEnd)
                : LapEnd{SDK::Fit::LapTrigger::SessionEnd};
            saveLap(end);
        }

        // No final record: the activity ends at the pause instant below, so a
        // record stamped at save time would fall outside the session. The
        // battery sample it used to carry goes with it.

        buildPartialSummary();

        // Save summary
        if (!mActivitySummarySerializer.save(mSummary)) {
            LOG_ERROR("Can't save activity summary\n");
        }
        SDK::send_msg<CustomMessage::Summary>(mKernel, &mSummary);

        // Save FIT file
        ActivityWriter::TrackData fitTrack{};

        // The activity ended when the user paused; see saveLap().
        const std::time_t endUtc  = mTimeCounter.getEndValue();
        const std::time_t tailSec = mTimeCounter.getTrailingPause();

        fitTrack.timestamp = endUtc;
        fitTrack.timeStart = mTimeCounter.getCurrent() - mTimeCounter.getValueTotal();
        fitTrack.duration  = mTimeCounter.getValueActive();
        fitTrack.elapsed   = mTimeCounter.getValueTotal() - tailSec;

        fitTrack.distance  = mDistanceCounter.getValueActive();

        fitTrack.speedAvg  = speedFromTotals(fitTrack.distance, fitTrack.duration);
        fitTrack.speedMax  = mSpeedCounter.getMaximum();

        fitTrack.hrAvg     = mHrCounter.getAverage();
        fitTrack.hrMax     = mHrCounter.getMaximum();

        fitTrack.ascent    = mAltitudeCounter.getAscent();
        fitTrack.descent   = mAltitudeCounter.getDescent();

        // HybridX Run: this run's VO2max, into the history, onto the summary
        // and into the FIT file (NOTES, R4).
        finishVo2(endUtc);
        fitTrack.vo2maxX10       = mVo2Info.runX10;
        fitTrack.vo2maxRecentX10 = mVo2Info.rollingX10;

        if (mActivityWriter.stop(fitTrack)) {
            notifyNewActivity();
            // A scheduled workout counts as done only once its activity is
            // saved, and only if completion was reached. started() again
            // first: if the record could not be saved when the run began,
            // the one held is still the workout before, and completed()
            // would mark that one.
            if (mWorkoutReachedEnd && mWorkoutDate != 0
                && !(mSchedule.started(mWorkoutDate, mWorkoutFile) && mSchedule.completed())) {
                LOG_ERROR("Can't save the workout schedule state\n");
            }
        } else {
            LOG_ERROR("activity save failed\n");
            // Do NOT notify: the .fit is left unfinished, so the crash-recovery
            // marker (if any) stays for the next boot to finalize.
        }
    } else {
        mActivityWriter.discard();
        closeVo2Log(true);   // HybridX Run: a discarded run's log goes too
    }

    mTrackState  = Track::State::INACTIVE;
    mWorkoutMode = false;
    LOG_INFO("Track stopped. UTC: %u\n", static_cast<uint32_t>(mTimeCounter.getEndValue()));
    LOG_INFO("Time: %u / %u s\n", static_cast<uint32_t>(mTimeCounter.getValueActive()), static_cast<uint32_t>(mTimeCounter.getValueTotal()));
    LOG_INFO("Distance: %.3f m\n", mDistanceCounter.getValueActive());
    LOG_INFO("Speed: %.3f / %.3f m/s\n", speedFromTotals(mDistanceCounter.getValueActive(), mTimeCounter.getValueActive()), mSpeedCounter.getMaximum());
    LOG_INFO("Heart rate: %.0f / %.0f bpm\n", mHrCounter.getAverage(), mHrCounter.getMaximum());
    LOG_INFO("Ascent/Descent: %.1f / %.1f m\n", mAltitudeCounter.getAscent(), mAltitudeCounter.getDescent());

    SDK::send_msg<CustomMessage::TrackStateUpd>(mKernel, mTrackState);

    // Persist the outdoor stride LUT synchronously before the task tears down
    // (only writes if >= 1 sample was accepted this session). finalise()
    // returns false both when nothing was accepted (nothing to persist) and on
    // a real write failure; only the latter is worth flagging, so gate the log
    // on having had accepted samples this session.
    const bool hadCalibData = mCalibrator.acceptedThisSession() > 0;
    if (!mCalibrator.finalise() && hadCalibData) {
        LOG_ERROR("Stride calibrator finalise failed; LUT not persisted\n");
    }

    disconnect();
}

void Service::pauseTrack(bool pause)
{
    if (mTrackState == Track::State::INACTIVE) {
        return;
    }

    if (pause && mTrackState == Track::State::ACTIVE) {
        mTimeCounter.pause();
        mDistanceCounter.pause();
        mSpeedCounter.pause();
        mHrCounter.pause();
        mAltitudeCounter.pause();

        mActivityWriter.pause(mTimeCounter.getCurrent());

        mCalibrator.pause();   // stop ingesting, reset steady-state counter

        mTrackState = Track::State::PAUSED;
        LOG_INFO("Track paused. UTC: %u\n", static_cast<uint32_t>(mTimeCounter.getCurrent()));
        SDK::send_msg<CustomMessage::TrackStateUpd>(mKernel, mTrackState);

        buildPartialSummary();
        SDK::send_msg<CustomMessage::Summary>(mKernel, &mSummary);
    } else if (!pause && mTrackState == Track::State::PAUSED) {
        mTimeCounter.resume();
        mDistanceCounter.resume();
        mSpeedCounter.resume();
        // Drop the pre-pause samples: they describe the effort before the break.
        // The scale factor is NOT dropped -- it describes how far this receiver
        // is drifting in these conditions, which a stop does not change, and
        // re-earning it would leave the readout uncorrected meanwhile.
        mSpeedFilter.resetHistory();
        mHrCounter.resume();
        if (mWorkoutMode) {
            // Live speed starts again from nothing: blend from it afresh.
            mSpeedGauge.resume();
            mHrGauge.resume();
        }
        mAltitudeCounter.resume();

        mActivityWriter.resume(mTimeCounter.getCurrent());

        mCalibrator.resume();  // restart steady-state counter from zero
        mLastCalibUtc = 0;     // avoid a spurious cross-pause delta_t

        mTrackState = Track::State::ACTIVE;
        LOG_INFO("Track resumed. UTC: %u\n", static_cast<uint32_t>(mTimeCounter.getCurrent()));
        SDK::send_msg<CustomMessage::TrackStateUpd>(mKernel, mTrackState);
    }
}

Service::LapDivSource Service::getLapDivSource()
{
    if (mSettings.alertDistanceId != Settings::Alerts::Distance::ID_OFF) {
        return LapDivSource::DISTANCE;
    }

    if (mSettings.alertTimeId != Settings::Alerts::Time::ID_OFF) {
        return LapDivSource::TIME;
    }

    return LapDivSource::OFF;
}

void Service::onWristTilt(uint32_t timestampMs)
{
    LOG_DEBUG("Wrist Tilt detected\n");
    backlightOn();
}


// =============================================================================
// Interval training
// =============================================================================

void Service::handleEvent(const CustomMessage::IntervalsNextPhase& /*event*/)
{
    nextStep();
}

void Service::nextStep()
{
    if (mTrackState != Track::State::ACTIVE || !(mIntervalsMode || mWorkoutMode)) {
        return;
    }
    const auto how = mEngine.next(activeTimeMs(), activeDistanceCm());
    if (how == SDK::Workout::Engine::Change::None) {
        return;
    }
    if (mIntervalsMode) {
        onIntervalsStepEnded(how);
    } else {
        onWorkoutStepEnded(how);
    }
}

uint32_t Service::activeTimeMs() const
{
    return static_cast<uint32_t>(mTimeCounter.getValueActive()) * 1000u;
}

uint32_t Service::activeDistanceCm() const
{
    const float meters = mDistanceCounter.getValueActive();
    return meters > 0.0f ? static_cast<uint32_t>(meters * 100.0f) : 0u;
}

void Service::startIntervals()
{
    const Settings::Intervals& cfg = mSettings.intervals;

    LOG_DEBUG("Intervals cfg: repeats=%u warmUp=%u coolDown=%u lastRest=%u "
             "runMetric=%u runTime=%u runDist=%.1f "
             "restMetric=%u restTime=%u restDist=%.1f\n",
             cfg.repeatsNum, cfg.warmUp, cfg.coolDown, cfg.lastRest,
             cfg.runMetric, cfg.runTime, cfg.runDistance,
             cfg.restMetric, cfg.restTime, cfg.restDistance);

    // A zero time or distance leaves the phase open, as before.
    const auto phase = [](Settings::Intervals::Metric metric, uint32_t timeSec, float distanceM) {
        using End = SDK::Workout::IntervalsSpec::Phase::End;
        SDK::Workout::IntervalsSpec::Phase ph;
        if (metric == Settings::Intervals::TIME && timeSec > 0) {
            ph.end    = End::Time;
            ph.timeMs = timeSec * 1000u;
        } else if (metric == Settings::Intervals::DISTANCE && distanceM > 0.0f) {
            ph.end        = End::Distance;
            ph.distanceCm = static_cast<uint32_t>(std::lround(distanceM * 100.0f));
        }
        return ph;
    };

    mIntervalsSpec          = SDK::Workout::IntervalsSpec{};
    mIntervalsSpec.repeats  = cfg.repeatsNum;
    mIntervalsSpec.run      = phase(cfg.runMetric, cfg.runTime, cfg.runDistance);
    mIntervalsSpec.rest     = phase(cfg.restMetric, cfg.restTime, cfg.restDistance);
    mIntervalsSpec.warmUp   = cfg.warmUp;
    mIntervalsSpec.coolDown = cfg.coolDown;
    mIntervalsSpec.lastRest = cfg.lastRest;
    mIntervalsLayout = SDK::Workout::buildIntervals(mIntervalsSpec, mProgram);
    mEngine.start(mProgram, 0, 0);
    mLeadIn.stepStarted();

    mTrackData.intervals = Track::IntervalsData{};
    // totalRepeats is the literal number of RUN-REST cycles the user selected.
    // repeatsNum == 0 means 'Open' (unlimited): totalRepeats stays 0 and the GUI
    // shows just the current repeat with no total.
    mTrackData.intervals.totalRepeats = cfg.repeatsNum;
    updateIntervalsData();
}

void Service::processIntervals()
{
    if (mIntervalsCompleted) {
        return;
    }

    const float speedMs = mSpeedFilter.getSpeed();
    const uint32_t speedMmps = speedMs > 0.0f ? static_cast<uint32_t>(speedMs * 1000.0f) : 0u;
    const auto how = mEngine.update(activeTimeMs(), activeDistanceCm(), speedMmps,
                                    mSpeedFilter.hasCurrentSample());
    if (how != SDK::Workout::Engine::Change::None) {
        onIntervalsStepEnded(how);
        return;
    }
    updateIntervalsData();

    // A beep each second as a timed or measured phase nears its end.
    if (mLeadIn.tick(mEngine.status())) {
        playBuzzerPattern(skLeadInBeepMs);
    }
}

void Service::onIntervalsStepEnded(SDK::Workout::Engine::Change how)
{
    mLeadIn.stepStarted();

    // The step that ended is recorded as its own lap, before the next begins.
    // An automatic end means its time or distance ran out.
    if (mLapNotEmpty) {
        saveLap(endedStepLap(how));
    }

    if (mEngine.status().finished) {
        LOG_INFO("Intervals: workout completed\n");

        // The programmed workout is done, but the session keeps running so the
        // user can record additional manual laps and end it themselves. Drop out
        // of intervals mode so the track screen reverts to normal faces and R2
        // records a lap (not a phase advance).
        mIntervalsCompleted = true;
        mIntervalsMode = false;
        mTrackData.intervalsMode = false;

        notifyStepChange();
        SDK::send_msg<CustomMessage::IntervalsWorkoutCompleted>(mKernel);
        return;
    }

    // Every change of phase alerts the same way, whether the phase ran out
    // or the runner moved on with R2.
    updateIntervalsData();
    SDK::send_msg<CustomMessage::IntervalsPhaseAlert>(mKernel, mTrackData.intervals);
    notifyStepChange();
}

Track::IntervalsPhase Service::intervalsPhase(uint16_t step) const
{
    if (step == mIntervalsLayout.warmUp) {
        return Track::IntervalsPhase::WARM_UP;
    }
    if (step == mIntervalsLayout.coolDown) {
        return Track::IntervalsPhase::COOL_DOWN;
    }
    if (step == mIntervalsLayout.rest) {
        return Track::IntervalsPhase::REST;
    }
    return Track::IntervalsPhase::RUN;
}

void Service::updateIntervalsData()
{
    const SDK::Workout::Engine::Status& st = mEngine.status();
    const SDK::Workout::Step&           step = mProgram.steps[st.step];
    Track::IntervalsData&               iv   = mTrackData.intervals;

    iv.phase = intervalsPhase(st.step);
    switch (step.end) {
    case SDK::Workout::StepEnd::Time:
        iv.metric        = Track::IntervalsMetric::TIME_REMAINING;
        iv.phaseTimerSec = static_cast<std::time_t>(st.remainingMs / 1000u);
        iv.distRemaining = 0.0f;
        break;
    case SDK::Workout::StepEnd::Distance:
        iv.metric        = Track::IntervalsMetric::DISTANCE;
        iv.phaseTimerSec = 0;
        iv.distRemaining = static_cast<float>(st.remainingCm) / 100.0f;
        break;
    default:
        iv.metric        = Track::IntervalsMetric::TIME_OPEN;
        iv.phaseTimerSec = static_cast<std::time_t>(st.stepTimeMs / 1000u);
        iv.distRemaining = 0.0f;
        break;
    }

    const uint32_t repeat = SDK::Workout::intervalsRepeat(mIntervalsSpec, mIntervalsLayout, st);
    iv.repeat = static_cast<uint8_t>(std::min<uint32_t>(repeat, UINT8_MAX));
}

Service::LapEnd Service::workoutLap(uint16_t step, SDK::Fit::LapTrigger trigger) const
{
    LapEnd end;
    end.trigger = trigger;
    uint16_t index = 0;
    if (SDK::Workout::fitStepIndex(mProgram, step, index)) {
        end.wktStepIndex = index;
        // As the file gave it; a file that left it out gets the step's kind.
        const SDK::Workout::Step& s = mProgram.steps[step];
        if (s.raw.intensity != static_cast<uint8_t>(SDK::Fit::Intensity::Invalid)) {
            end.intensity = static_cast<SDK::Fit::Intensity>(s.raw.intensity);
        } else {
            switch (s.intensity) {
            case SDK::Workout::Intensity::Rest:     end.intensity = SDK::Fit::Intensity::Rest; break;
            case SDK::Workout::Intensity::Warmup:   end.intensity = SDK::Fit::Intensity::Warmup; break;
            case SDK::Workout::Intensity::Cooldown: end.intensity = SDK::Fit::Intensity::Cooldown; break;
            default:                                end.intensity = SDK::Fit::Intensity::Active; break;
            }
        }
    }
    return end;
}

Service::LapEnd Service::endedStepLap(SDK::Workout::Engine::Change how) const
{
    const SDK::Workout::Engine::Ended& ended = mEngine.ended();
    LapEnd end = workoutLap(ended.step, lapTrigger(how, ended.step));

    // The engine was given these same totals this tick, and a step that ran
    // out ended short of them; one ended by hand ends here. Its distance is
    // the engine's, so a distance step's lap is exactly that distance.
    if (how == SDK::Workout::Engine::Change::Auto) {
        const uint32_t nowMs = activeTimeMs();
        end.ranOut  = true;
        end.carryMs = ended.atTimeMs < nowMs ? nowMs - ended.atTimeMs : 0u;
        end.stepM   = static_cast<float>(ended.distanceCm) / 100.0f;
    }
    return end;
}

SDK::Fit::LapTrigger Service::lapTrigger(SDK::Workout::Engine::Change how, uint16_t step) const
{
    // An automatic end means the step's time or distance ran out.
    if (how != SDK::Workout::Engine::Change::Auto) {
        return SDK::Fit::LapTrigger::Manual;
    }
    return mProgram.steps[step].end == SDK::Workout::StepEnd::Distance
               ? SDK::Fit::LapTrigger::Distance
               : SDK::Fit::LapTrigger::Time;
}

void Service::notifyStepChange()
{
    LOG_DEBUG("Step change\n");

    // The alert screen, then the next one: keep the backlight on for both.
    backlightOn(skBacklightTimeout * 2);
    playVibroPattern(SDK::Message::RequestVibroPlay::Effect::SHORT_DOUBLE_CLICK_STRONG_1_100);
    playBuzzerPattern(150, 2);
}


// =============================================================================
// Structured workouts
// =============================================================================

SDK::Workout::Ymd Service::localDate()
{
    const std::tm t = mTimeTracker.getLocalTime(std::time(nullptr));
    const int year = t.tm_year + 1900;
    if (year < 2020) {
        return 0;  // the clock has not been set
    }
    return static_cast<SDK::Workout::Ymd>(year) * 10000u
         + static_cast<SDK::Workout::Ymd>(t.tm_mon + 1) * 100u
         + static_cast<SDK::Workout::Ymd>(t.tm_mday);
}

void Service::scanWorkouts()
{
    // The GUI keeps a pointer to the list, and the scan reads each file into
    // mProgram, so the list is only built while no run is going.
    if (mTrackState == Track::State::INACTIVE) {
        mSchedule.load();
        mLibrary.scan(mKernel.fs, localDate(), mReader, mProgram);
        LOG_INFO("Workouts: %u listed, %u left out of a full list\n",
                 static_cast<unsigned>(mLibrary.size()), static_cast<unsigned>(mLibrary.dropped()));
    }
}

void Service::sendWorkoutList()
{
    // Today's workout is offered at launch unless it was completed.
    uint16_t today = CustomMessage::WorkoutList::kNoToday;
    const size_t t = mLibrary.today();
    if (t != SDK::Workout::Library::kNone
        && !mSchedule.isCompleted(mLibrary.entry(t).date, mLibrary.entry(t).file)) {
        today = static_cast<uint16_t>(t);
    }
    SDK::send_msg<CustomMessage::WorkoutList>(mKernel, &mLibrary, today);
}

bool Service::readWorkout(uint16_t index)
{
    char path[SDK::Interface::IFileSystem::skMaxPathLen];
    if (index >= mLibrary.size() || !mLibrary.path(index, path, sizeof(path))) {
        return false;
    }
    auto file = mKernel.fs.file(path);
    if (!file) {
        return false;
    }
    const auto result = mReader.read(*file, mProgram);
    if (result != SDK::Fit::FitWorkoutReader::Result::Ok) {
        LOG_WARNING("Can't read %s: %s\n", path, SDK::Fit::FitWorkoutReader::resultName(result));
        return false;
    }
    return true;
}

void Service::handleEvent(const CustomMessage::WorkoutDetailsRequest& event)
{
    // Only before a run: during one, mProgram holds the workout being run.
    const bool ok = mTrackState == Track::State::INACTIVE && readWorkout(event.index);
    SDK::send_msg<CustomMessage::WorkoutDetails>(mKernel, event.index, ok ? &mProgram : nullptr);
}

bool Service::startWorkout(uint16_t index)
{
    // Read again: the file may have changed since the list was made.
    if (!readWorkout(index)) {
        LOG_WARNING("Workout %u not started; this is a free run\n", static_cast<unsigned>(index));
        return false;
    }
    mEngine.start(mProgram, 0, 0);
    mLeadIn.stepStarted();
    startWorkoutStep();

    const SDK::Workout::Library::Entry& e = mLibrary.entry(index);
    if (e.date != 0) {
        mWorkoutDate = e.date;
        std::memcpy(mWorkoutFile, e.file, sizeof(mWorkoutFile));
        if (!mSchedule.started(e.date, e.file)) {
            LOG_ERROR("Can't save the workout schedule state\n");
        }
    }
    LOG_INFO("Workout started: %s\n", mProgram.name);
    return true;
}

bool Service::hasTrustedHr() const
{
    return mHrCounter.getCurrent() > 20 && mTrackData.hrTrustLevel >= 1 && mTrackData.hrTrustLevel <= 3;
}

bool Service::hrBand(const SDK::Workout::Step& step, float& low, float& high) const
{
    // Zone n is [threshold n-1, threshold n); the last threshold is maximum HR.
    const float maxHr = mHrThresholds[CustomMessage::kHrThresholdsCount - 1];
    switch (step.hrKind) {
    case SDK::Workout::HeartRateTarget::Zone:
        if (step.hrZone < 1 || step.hrZone >= CustomMessage::kHrThresholdsCount) {
            return false;
        }
        low  = mHrThresholds[step.hrZone - 1];
        high = mHrThresholds[step.hrZone];
        break;
    case SDK::Workout::HeartRateTarget::Bpm:
        low  = step.hrLow;
        high = step.hrHigh;
        break;
    case SDK::Workout::HeartRateTarget::PercentMax:
        low  = step.hrLow * maxHr / 100.0f;
        high = step.hrHigh * maxHr / 100.0f;
        break;
    }
    // A file may give the ends either way round.
    if (low > high) {
        std::swap(low, high);
    }
    return high > 0.0f;
}

void Service::startWorkoutStep()
{
    const SDK::Workout::Step& s = mProgram.steps[mEngine.status().step];
    mZoneAlerts.stepStarted();
    mAlertRepeat = nullptr;   // the last step's alert does not repeat into this one
    mBandLow  = 0.0f;
    mBandHigh = 0.0f;

    // Both gauges run on every step: an untargeted one still has a value and
    // a step average to show.
    if (s.target == SDK::Workout::Target::Speed && s.speedLowMmps > 0 && s.speedHighMmps > 0) {
        mBandLow  = static_cast<float>(std::min(s.speedLowMmps, s.speedHighMmps)) / 1000.0f;
        mBandHigh = static_cast<float>(std::max(s.speedLowMmps, s.speedHighMmps)) / 1000.0f;
        mSpeedGauge.startStep(mBandLow, mBandHigh);
    } else {
        mSpeedGauge.startStep();
    }

    float low = 0.0f;
    float high = 0.0f;
    if (s.target == SDK::Workout::Target::HeartRate && hrBand(s, low, high)) {
        mBandLow  = low;
        mBandHigh = high;
        mHrGauge.startStep(low, high);
    } else {
        mHrGauge.startStep();
    }
}

SDK::Workout::Zone Service::targetZone() const
{
    switch (mProgram.steps[mEngine.status().step].target) {
    case SDK::Workout::Target::Speed:     return mSpeedGauge.zone();
    case SDK::Workout::Target::HeartRate: return mHrGauge.zone();
    default:                              return SDK::Workout::Zone::Unknown;
    }
}

void Service::processWorkout()
{
    const float    speedMs   = mSpeedFilter.getSpeed();
    const uint32_t speedMmps = speedMs > 0.0f ? static_cast<uint32_t>(speedMs * 1000.0f) : 0u;
    const auto how = mEngine.update(activeTimeMs(), activeDistanceCm(), speedMmps,
                                    mSpeedFilter.hasCurrentSample());
    if (how != SDK::Workout::Engine::Change::None) {
        onWorkoutStepEnded(how);
        return;
    }
    const SDK::Workout::Engine::Status& st = mEngine.status();
    mWorkoutReachedEnd = mWorkoutReachedEnd || st.reachedEnd;

    // The filter's validity rides over a second that brings no fresh sample;
    // the gauge treats a false as a lost signal, and so it must stay until
    // the distance across a lost fix has come in.
    mSpeedGauge.tick(speedMs, mSpeedFilter.isValid() && mGpsCatchUp.caughtUp(),
                     st.stepDistanceCm);
    mHrGauge.tick(mHrCounter.getCurrent(), hasTrustedHr());

    const auto alert = mZoneAlerts.tick(targetZone());
    const bool beep  = mLeadIn.tick(st);
    if (alert != SDK::Workout::ZoneAlerts::Alert::None) {
        playZoneAlert(alert);
    } else if (beep) {
        playBuzzerPattern(skLeadInBeepMs);
    }
    sendWorkoutData();
}

void Service::onWorkoutStepEnded(SDK::Workout::Engine::Change how)
{
    mLeadIn.stepStarted();

    // The step that ended is recorded as its own lap, before the next begins.
    if (mLapNotEmpty) {
        saveLap(endedStepLap(how));
    }
    mWorkoutReachedEnd = mWorkoutReachedEnd || mEngine.status().reachedEnd;

    if (mEngine.status().finished) {
        LOG_INFO("Workout completed\n");
        // The run carries on as a free run, as after Intervals.
        endWorkout();
        notifyStepChange();
        SDK::send_msg<CustomMessage::IntervalsWorkoutCompleted>(mKernel);
        return;
    }

    // Every step change alerts the same way, whether the step ran out or the
    // runner moved on.
    startWorkoutStep();
    sendWorkoutSteps();
    notifyStepChange();
    sendWorkoutData();
}

void Service::handleEvent(const CustomMessage::WorkoutEnd& /*event*/)
{
    if (!mWorkoutMode || mTrackState == Track::State::INACTIVE) {
        return;
    }
    // Completion already reached stands; ending before it does not count.
    // The step in progress ends here as a manual lap. next() is not used: on
    // the last step it would count the workout as completed.
    mWorkoutReachedEnd = mWorkoutReachedEnd || mEngine.status().reachedEnd;
    if (mLapNotEmpty) {
        saveLap(workoutLap(mEngine.status().step, SDK::Fit::LapTrigger::Manual));
    }
    LOG_INFO("Workout ended\n");
    endWorkout();
}

void Service::endWorkout()
{
    mEngine.stop();
    mWorkoutMode = false;
    mTrackData.workoutMode = false;
}

void Service::sendWorkoutData()
{
    const SDK::Workout::Engine::Status& st = mEngine.status();
    const SDK::Workout::Step&           s  = mProgram.steps[st.step];

    auto msg = SDK::make_msg<CustomMessage::WorkoutData>(mKernel);
    if (!msg) {
        return;
    }
    msg->remainingMs  = st.remainingMs;
    msg->remainingCm  = st.remainingCm;
    msg->stepTimeMs   = st.stepTimeMs;
    msg->rep          = st.rep;
    msg->reps         = st.reps;
    msg->stepEnd      = static_cast<uint8_t>(s.end);
    msg->target       = static_cast<uint8_t>(s.target);
    msg->intensity    = static_cast<uint8_t>(s.intensity);
    msg->leadIn       = st.leadIn;
    msg->low          = mBandLow;
    msg->high         = mBandHigh;
    msg->zone         = static_cast<uint8_t>(targetZone());

    if (s.target == SDK::Workout::Target::Speed && mBandHigh > 0.0f) {
        // The arc is laid out on pace, with faster to the right. At or below
        // the floor the live pace has, the speed goes as 0, which shows no
        // pace, as the live pace does.
        const auto  pace  = [](float mps) { return mps > 0.05f ? 1000.0f / mps : 1.0e6f; };
        const float speed = mSpeedGauge.valueMps();
        msg->value = speed > mSpeedFilter.getMinValid() ? speed : 0.0f;
        msg->arc   = 1.0f - SDK::Workout::arcFraction(pace(msg->value), pace(mBandHigh),
                                                      pace(mBandLow), skArcMarginSecPerKm);
    } else if (s.target == SDK::Workout::Target::HeartRate && mBandHigh > 0.0f) {
        msg->value = mHrGauge.valueBpm();
        msg->arc   = SDK::Workout::arcFraction(msg->value, mBandLow, mBandHigh, skArcMarginBpm);
    }
    msg.send();
}

void Service::sendWorkoutSteps()
{
    const SDK::Workout::Engine::Status& st = mEngine.status();
    sendWorkoutStep(&mProgram.steps[st.step], false);
    uint16_t next = 0;
    sendWorkoutStep(mEngine.peekNext(next) ? &mProgram.steps[next] : nullptr, true);
}

void Service::sendWorkoutStep(const SDK::Workout::Step* step, bool next)
{
    auto msg = SDK::make_msg<CustomMessage::WorkoutStep>(mKernel);
    if (!msg) {
        return;
    }
    msg->next = next;
    msg->none = step == nullptr;
    if (step) {
        const char* notes = step->notes[0] != '\0' ? step->notes : step->name;
        std::strncpy(msg->notes, notes, sizeof(msg->notes) - 1);
        SDK::Workout::stepDuration(*step, mIsImperial, msg->duration, sizeof(msg->duration));
        SDK::Workout::stepTarget(*step, mIsImperial, msg->target, sizeof(msg->target));
        msg->intensity = static_cast<uint8_t>(step->intensity);
        if (!next) {
            msg->rep  = mEngine.status().rep;
            msg->reps = mEngine.status().reps;
        }
    }
    msg.send();
}

void Service::playAlertTwice(const SDK::Message::RequestBuzzerPlay::Note notes[], uint8_t count)
{
    static constexpr uint32_t kGapMs = 1000;

    // Both playings do not fit one message, so the repeat is a second
    // message, sent once the first has played and the gap has passed. It is
    // not sent sooner: a message that arrives while notes are playing cuts
    // the current note short. The service loop runs at least every 500 ms,
    // so the gap is 1-1.5 s.
    if (count > SDK::Message::RequestBuzzerPlay::skMaxNotes) {
        count = SDK::Message::RequestBuzzerPlay::skMaxNotes;
    }
    if (count == 0) {
        return;
    }
    playBuzzerNotes(notes, count);

    uint32_t playMs = 0;
    for (uint8_t i = 0; i < count; ++i) {
        playMs += notes[i].time;
    }
    mAlertRepeat        = notes;
    mAlertRepeatCount   = count;
    mAlertRepeatFromMs  = mKernel.sys.getTimeMs();
    mAlertRepeatAfterMs = playMs + kGapMs;
}

void Service::sendAlertRepeat()
{
    if (mAlertRepeat == nullptr) {
        return;
    }
    // Once the run is paused or stopped, or the workout has ended, the repeat
    // no longer applies.
    if (!mWorkoutMode || mTrackState != Track::State::ACTIVE) {
        mAlertRepeat = nullptr;
        return;
    }
    if (SDK::Timer::elapsed(mKernel.sys.getTimeMs(), mAlertRepeatFromMs) < mAlertRepeatAfterMs) {
        return;
    }
    playBuzzerNotes(mAlertRepeat, mAlertRepeatCount);
    mAlertRepeat = nullptr;
}

void Service::playZoneAlert(SDK::Workout::ZoneAlerts::Alert alert)
{
    // Pitch, rhythm and loudness tell them apart: too slow is long and falls
    // from C7 to C6 twice, too fast is short and rises from C8 to C9 twice,
    // and back in range rises through a C7 major triad (4 : 5 : 6). A kernel
    // without pitch support plays every note at its default pitch, so the
    // rhythms alone still tell them apart. Each plays twice, about 1 s apart.
    static constexpr uint16_t kLowHz  = 1047;  // C6
    static constexpr uint16_t kHighHz = 8372;  // C9
    static constexpr uint16_t kMidHz  = 4186;  // C8
    static constexpr uint16_t kRootHz = 2093;  // C7
    // Each silence keeps the pitch before it, so the buzzer is not retuned for it.
    static const SDK::Message::RequestBuzzerPlay::Note kBackIn[] = {
        {120, 100, 0, kRootHz},
        {30, 0, 0, kRootHz},
        {120, 100, 0, kRootHz * 5 / 4},
        {30, 0, 0, kRootHz * 5 / 4},
        {240, 100, 0, kRootHz * 3 / 2},
    };
    static const SDK::Message::RequestBuzzerPlay::Note kBelow[] = {
        {300, 100, 0, kRootHz},
        {150, 0, 0, kRootHz},
        {300, 100, 0, kLowHz},
        {150, 0, 0, kLowHz},
        {300, 100, 0, kRootHz},
        {150, 0, 0, kRootHz},
        {300, 100, 0, kLowHz},
    };
    static const SDK::Message::RequestBuzzerPlay::Note kAbove[] = {
        {80, 100, 0, kMidHz},
        {80, 0, 0, kMidHz},
        {80, 100, 0, kHighHz},
        {80, 0, 0, kHighHz},
        {80, 100, 0, kMidHz},
        {80, 0, 0, kMidHz},
        {80, 100, 0, kHighHz},
    };

    backlightOn();
    switch (alert) {
    case SDK::Workout::ZoneAlerts::Alert::Above:  // too fast, or heart rate too high
        playAlertTwice(kAbove, sizeof(kAbove) / sizeof(kAbove[0]));
        playVibroPattern(SDK::Message::RequestVibroPlay::Effect::DOUBLE_CLICK_100);
        break;
    case SDK::Workout::ZoneAlerts::Alert::Below:  // too slow, or heart rate too low
        playAlertTwice(kBelow, sizeof(kBelow) / sizeof(kBelow[0]));
        playVibroPattern(SDK::Message::RequestVibroPlay::Effect::ALERT_750MS_100);
        break;
    case SDK::Workout::ZoneAlerts::Alert::BackIn:  // brief and cheerful
        playAlertTwice(kBackIn, sizeof(kBackIn) / sizeof(kBackIn[0]));
        playVibroPattern(SDK::Message::RequestVibroPlay::Effect::STRONG_CLICK_100);
        break;
    default:
        break;
    }
}

// -- HybridX Run: the VO2max estimate (docs/NOTES.md) ----------------------------------

void Service::loadVo2()
{
    // Read once at start, as the SDK intends (Docs/app-config-fields.md 7.2):
    // a change on the phone applies the next time the app opens. On the heap,
    // once, as HybridX Race and Streak do: not in the 1 Hz path.
    // Kept (R5), so a change made on the watch is written back to the same
    // file the phone writes, as HybridX Race does.
    mConfig.reset(new SDK::AppConfig(mKernel, RunConfig::kFileName, RunConfig::kFields, RunConfig::kFieldCount));
    mVo2Input.birthYear     = static_cast<uint16_t>(mConfig->getInt(RunConfig::kBirthYear));
    mVo2Input.birthMonth    = static_cast<uint8_t>(mConfig->getInt(RunConfig::kBirthMonth));
    mVo2Input.enteredMaxHr  = static_cast<uint8_t>(mConfig->getInt(RunConfig::kMaxHr));
    mVo2Input.enteredRestHr = static_cast<uint8_t>(mConfig->getInt(RunConfig::kRestingHr));
    mVo2TuningLog           = mConfig->getBool(RunConfig::kTuningLog);
    LOG_INFO("VO2 profile: born %u/%u, max HR %u, resting HR %u (0 = not set), tuning log %u\n",
             mVo2Input.birthMonth, mVo2Input.birthYear, mVo2Input.enteredMaxHr, mVo2Input.enteredRestHr,
             mVo2TuningLog);

    char buf[RunVo2::Vo2History::kMaxJsonBytes];
    size_t len = RunVo2::SafeFile::read(mKernel.fs, kVo2File, false, buf, sizeof(buf));
    if (len == 0 || !mVo2History.fromJson(buf, len)) {
        len = RunVo2::SafeFile::read(mKernel.fs, kVo2File, true, buf, sizeof(buf));
        if (len == 0 || !mVo2History.fromJson(buf, len)) {
            mVo2History.clear();
            LOG_INFO("No VO2max history yet\n");
        } else {
            LOG_WARNING("vo2.json unreadable: using the backup\n");
        }
    }
    mVo2Input.autoMaxHr   = mVo2History.autoMaxHr();
    mVo2Input.watchRestHr = mVo2History.watchRestHr();   // until the sensor reports

    // Runs HybridX Trail has recorded since this app last opened count too.
    ingestTrailRuns();

    // Also at start (R6), so the glance gets the version 2 file (with the
    // trend) after an update, before the next run is saved.
    writeSharedVo2();

    mVo2Info            = CustomMessage::Vo2Info{};
    mVo2Info.rollingX10 = mVo2History.rollingX10();
    mVo2Info.runs       = mVo2History.size();
    LOG_INFO("VO2max history: %u runs, shown %u (x10), auto max HR %u from %u runs, resting HR %u\n",
             mVo2Info.runs, mVo2Info.rollingX10, mVo2Input.autoMaxHr, mVo2History.runMaxCount(),
             mVo2Input.watchRestHr);
}

void Service::feedVo2()
{
    const uint32_t nowMs = mKernel.sys.getTimeMs();
    // Unsigned deltas: the ms clock wraps.
    const bool hrFresh    = mHrSeen && static_cast<uint32_t>(nowMs - mHrLastMs) <= kVo2FreshMs;
    const bool speedFresh = mSpeedSeen && static_cast<uint32_t>(nowMs - mSpeedLastMs) <= kVo2FreshMs;
    const bool gradeFresh = mGradeSeen && static_cast<uint32_t>(nowMs - mGradeLastMs) <= kVo2FreshMs;

    const float hr    = mHrCounter.getCurrent();
    const float trust = mTrackData.hrTrustLevel;

    RunVo2::Sample s;
    s.active        = (mTrackState == Track::State::ACTIVE);
    // The corrected speed, not the receiver's raw one: the AG3335's speed over ground reads
    // 0.7% (open road) to 5.8% (tree cover) low against the position track, and the ACSM cost is
    // proportional to speed, so the raw figure biased VO2max low by about the same amount.
    // getInstantSpeed() is the latest sample times the filter's scale, at the same 1 Hz as the
    // raw feed was; hasCurrentSample() is false on a tick without a current fix. The filter was
    // ticked earlier in processTrack(). (R6b)
    s.speedMs       = mSpeedFilter.getInstantSpeed();
    s.speedValid    = mSpeedFilter.hasCurrentSample() && speedFresh;
    s.speedScale    = mSpeedFilter.getScale();
    s.deadReckoning = mGpsDeadReckoning;
    s.gradePct      = mGradeData.gradePct;
    s.gradeValid    = mGradeData.gradeValid && gradeFresh;   // stale: taken as level
    s.hrBpm         = (hrFresh && hr > 0.0f && hr < 255.0f) ? static_cast<uint8_t>(hr + 0.5f) : 0;
    s.hrTrust       = trust;   // the raw value: the core applies RunLVGL's 1-3 test
    mVo2Run.addSecond(s);

    // The tuning log: one row per closed window (about once a minute).
    RunVo2::WindowLog w;
    if (mVo2Run.popWindowLog(w) && mVo2LogFile) {
        writeVo2Log(RunVo2::Log::windowLine(w, mVo2LogLine, sizeof(mVo2LogLine)));
    }
}

void Service::finishVo2(std::time_t endUtc)
{
    // This run's sustained max joins the last runs'; the auto max is the
    // second-highest of them (Vo2Config kAutoMaxConfirmRuns), so one artefact
    // cannot raise it. Recorded even when a max HR is entered, so clearing
    // that later finds candidates; used only when none is entered.
    mVo2History.addRunMax(mVo2Run.sustainedMaxHr());
    mVo2Input.autoMaxHr = mVo2History.autoMaxHr();

    const RunVo2::Profile   profile = RunVo2::resolveProfile(mVo2Input, endUtc);
    const RunVo2::RunResult result  = mVo2Run.estimate(profile);
    const RunVo2::WindowCounts& c   = mVo2Run.counts();
    LOG_INFO("VO2 windows: %u kept, %u warm-up, %u gaps, %u unsteady, %u grade, %u overflow\n",
             c.accepted, c.warmUp, c.gaps, c.unsteady, c.grade, c.overflow);
    LOG_INFO("VO2 run: status %u (profile %u), %u (x10) from %u windows (%u too easy, %u above max, %u implausible); "
             "max HR %u (source %u), rest %u, run max %u\n",
             static_cast<unsigned>(result.status), static_cast<unsigned>(result.profileStatus),
             result.vo2x10, result.windowsUsed, result.tooEasy, result.aboveMax, result.implausible,
             profile.maxHr, static_cast<unsigned>(profile.maxSource), profile.restHr,
             mVo2Run.sustainedMaxHr());

    {
        // The run's average pace as if level (L3 shows it; for now the log has it).
        const float gap = mVo2Run.avgGradeAdjustedSpeedMs();
        LOG_INFO("Average grade-adjusted pace: %u s/km over %lu s moving\n",
                 gap > 0.0f ? static_cast<unsigned>(1000.0f / gap + 0.5f) : 0u,
                 static_cast<unsigned long>(mVo2Run.movingSec()));
    }

    if (result.status == RunVo2::RunStatus::Ok) {
        mVo2History.add(RunVo2::RunRecord{static_cast<uint32_t>(endUtc), result.vo2x10, result.windowsUsed});
    }
    if (mVo2LogFile) {
        writeVo2Log(RunVo2::Log::resultLine(result, profile, mVo2Run.sustainedMaxHr(),
                                            mVo2LogLine, sizeof(mVo2LogLine)));
        closeVo2Log(false);
    }

    saveVo2Files();

    mVo2Info.rollingX10    = mVo2History.rollingX10();
    mVo2Info.runs          = mVo2History.size();
    mVo2Info.hasRun        = true;
    mVo2Info.runStatus     = static_cast<uint8_t>(result.status);
    mVo2Info.profileStatus = static_cast<uint8_t>(result.profileStatus);
    mVo2Info.runX10        = result.status == RunVo2::RunStatus::Ok ? result.vo2x10 : 0;
    mVo2Info.windowsUsed   = result.windowsUsed;
    mVo2Info.maxHr         = profile.maxHr;
    mVo2Info.maxSource     = static_cast<uint8_t>(profile.maxSource);
    mVo2Info.restHr        = profile.restHr;
    sendVo2Info();
}

void Service::writeSharedVo2()
{
    char buf[RunVo2::Vo2History::kMaxSharedJsonBytes];
    const size_t len = mVo2History.toSharedJson(buf, sizeof(buf));
    if (len == 0 || !mKernel.fs.mkdir(kSharedDir)
        || !RunVo2::SafeFile::write(mKernel.fs, kSharedFile, buf, len)) {
        LOG_WARNING("Could not save the public vo2max.json\n");
    }
}

void Service::saveVo2Files()
{
    char buf[RunVo2::Vo2History::kMaxJsonBytes];
    const size_t len = mVo2History.toJson(buf, sizeof(buf));
    if (len == 0 || !RunVo2::SafeFile::write(mKernel.fs, kVo2File, buf, len)) {
        LOG_ERROR("Could not save vo2.json\n");
    }
    writeSharedVo2();
}

namespace
{

/// One of Trail's files: the primary, else its backup, whichever has a header
/// that reads. Returns the length, or 0.
size_t readTrailSlot(SDK::Interface::IFileSystem& fs, uint8_t slot, char* buf, size_t cap,
                     RunVo2::WindowFileInfo& info)
{
    for (int backup = 0; backup < 2; ++backup) {
        const size_t len = RunVo2::SafeFile::read(fs, RunVo2::kWindowFilePaths[slot], backup != 0, buf, cap);
        if (len > 0) {
            RunVo2::WindowFileReader reader(buf, len);
            if (reader.open(info)) {
                return len;
            }
        }
    }
    info = RunVo2::WindowFileInfo{};
    return 0;
}

}  // namespace

void Service::ingestTrailRuns()
{
    // Each run Trail left (newest three) above the history's Trail mark, oldest
    // first: its windows are judged by this app's rules and profile, exactly as a
    // run recorded here, and the run joins the history in time order. Once, at
    // start: the apps never run together. A temporary 8 KB buffer, not a stack
    // array (10 KB stack).
    std::unique_ptr<char[]> buf(new (std::nothrow) char[RunVo2::kWindowFileBytes]);
    if (!buf) {
        LOG_WARNING("No memory to read Trail's runs\n");
        return;
    }

    uint32_t ids[RunVo2::kWindowFileSlots] = {};
    for (uint8_t i = 0; i < RunVo2::kWindowFileSlots; ++i) {
        RunVo2::WindowFileInfo info;
        if (readTrailSlot(mKernel.fs, i, buf.get(), RunVo2::kWindowFileBytes, info) > 0) {
            ids[i] = info.runId;
        }
    }

    const std::time_t now = std::time(nullptr);
    bool changed = false;
    for (uint8_t step = 0; step < RunVo2::kWindowFileSlots; ++step) {
        const int slot = RunVo2::nextWindowFileSlot(ids, RunVo2::kWindowFileSlots, mVo2History.trailMarkUtc());
        if (slot < 0) {
            break;
        }
        ids[slot] = 0;

        // Without a profile there is nothing to score against: leave the run
        // for another day (the mark is not moved).
        if (RunVo2::resolveProfile(mVo2Input, now).status != RunVo2::ProfileStatus::Ok) {
            LOG_INFO("Trail run waiting: no VO2 profile yet\n");
            break;
        }

        RunVo2::WindowFileInfo info;
        const size_t len = readTrailSlot(mKernel.fs, static_cast<uint8_t>(slot), buf.get(),
                                         RunVo2::kWindowFileBytes, info);
        if (len == 0) {
            continue;
        }
        const RunVo2::IngestStatus st =
            RunVo2::judgeWindowFile(buf.get(), len, mVo2History.trailMarkUtc(), mVo2Run, info);
        if (st == RunVo2::IngestStatus::NotAFile || st == RunVo2::IngestStatus::AlreadyCounted) {
            continue;
        }
        mVo2History.setTrailMarkUtc(info.runId);   // counted, or corrupt and never retried
        changed = true;
        if (st == RunVo2::IngestStatus::Corrupt) {
            LOG_WARNING("Trail run %lu: a window is out of range, run skipped\n",
                        static_cast<unsigned long>(info.runId));
            continue;
        }

        mVo2History.addRunMax(mVo2Run.sustainedMaxHr());
        mVo2Input.autoMaxHr = mVo2History.autoMaxHr();
        const RunVo2::Profile   profile = RunVo2::resolveProfile(mVo2Input, now);
        const RunVo2::RunResult result  = mVo2Run.estimate(profile);
        const RunVo2::WindowCounts& c   = mVo2Run.counts();
        LOG_INFO("Trail run %lu: %u windows, %u kept, %u unsteady, %u grade; status %u, %u (x10)\n",
                 static_cast<unsigned long>(info.runId), info.count, c.accepted, c.unsteady, c.grade,
                 static_cast<unsigned>(result.status), result.vo2x10);
        if (result.status == RunVo2::RunStatus::Ok) {
            mVo2History.addSorted(RunVo2::RunRecord{info.runId, result.vo2x10, result.windowsUsed,
                                                    static_cast<uint8_t>(RunVo2::RunSource::Trail)});
        }
    }
    mVo2Run.reset();

    if (changed) {
        mVo2Input.autoMaxHr = mVo2History.autoMaxHr();
        saveVo2Files();
    }
}

void Service::sendVo2Info()
{
    fillVo2Page();
    SDK::send_msg<CustomMessage::Vo2Upd>(mKernel, mVo2Info);
}

void Service::fillVo2Page()
{
    CustomMessage::Vo2Info& v = mVo2Info;
    v.rollingX10 = mVo2History.rollingX10();
    v.beforeX10  = mVo2History.rollingX10Before();
    v.runs       = mVo2History.size();

    // The last runs, newest first, by local date.
    v.rowCount = 0;
    for (uint8_t i = mVo2History.size(); i > 0 && v.rowCount < CustomMessage::Vo2Info::kMaxRows; --i) {
        const RunVo2::RunRecord& r  = mVo2History.at(static_cast<uint8_t>(i - 1));
        const std::tm            tm = mTimeTracker.getLocalTime(static_cast<std::time_t>(r.utc));
        CustomMessage::Vo2Info::Row& row = v.rows[v.rowCount++];
        row.day    = static_cast<uint8_t>(tm.tm_mday);
        row.month  = static_cast<uint8_t>(tm.tm_mon + 1);
        row.vo2x10 = r.vo2x10;
    }

    const RunVo2::Profile now = RunVo2::resolveProfile(mVo2Input, mTimeTracker.getExpectedUTC());
    v.nowStatus     = static_cast<uint8_t>(now.status);
    v.nowAge        = now.ageYears;
    v.nowMaxHr      = now.maxHr;
    v.nowMaxSource  = static_cast<uint8_t>(now.maxSource);
    v.nowRestHr     = now.restHr;
    v.nowRestSource = static_cast<uint8_t>(now.restSource);

    v.birthYear  = mVo2Input.birthYear;
    v.birthMonth = mVo2Input.birthMonth;
    v.setMaxHr   = mVo2Input.enteredMaxHr;
    v.setRestHr  = mVo2Input.enteredRestHr;
    v.tuningLog  = mVo2TuningLog;
}

void Service::handleEvent(const CustomMessage::Vo2ProfileSave& event)
{
    mVo2Input.birthYear     = event.birthYear;
    mVo2Input.birthMonth    = event.birthMonth;
    mVo2Input.enteredMaxHr  = event.maxHr;
    mVo2Input.enteredRestHr = event.restHr;
    mVo2TuningLog           = event.tuningLog;
    LOG_INFO("VO2 profile set on the watch: born %u/%u, max HR %u, resting HR %u, tuning log %u\n",
             event.birthMonth, event.birthYear, event.maxHr, event.restHr, event.tuningLog);

    // Written back to the file the phone writes (setInt clamps to the field's
    // range), so the two agree; last writer wins, as in HybridX Race.
    if (mConfig) {
        mConfig->setInt(RunConfig::kBirthYear, event.birthYear);
        mConfig->setInt(RunConfig::kBirthMonth, event.birthMonth);
        mConfig->setInt(RunConfig::kMaxHr, event.maxHr);
        mConfig->setInt(RunConfig::kRestingHr, event.restHr);
        mConfig->setBool(RunConfig::kTuningLog, event.tuningLog);
        if (!mConfig->save()) {
            LOG_WARNING("Could not save app_config.json\n");
        }
    }
    sendVo2Info();
}

void Service::openVo2Log(std::time_t startUtc)
{
    closeVo2Log(false);
    if (!mVo2TuningLog) {
        return;
    }
    // Named as the run's activity file is (local time), so the two pair up.
    const std::tm t = mTimeTracker.getLocalTime(startUtc);
    char path[48];
    std::snprintf(path, sizeof(path), "Logs/vo2_%04d%02d%02dT%02d%02d%02d.csv",
                  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    if (!mKernel.fs.mkdir("Logs")) {
        LOG_WARNING("Could not create Logs/\n");
    }
    mVo2LogFile = mKernel.fs.file(path);
    if (!mVo2LogFile || !mVo2LogFile->open(true, true)) {
        LOG_WARNING("Could not open the tuning log %s\n", path);
        mVo2LogFile.reset();
        return;
    }
    LOG_INFO("Tuning log: %s\n", path);
    const RunVo2::Profile p = RunVo2::resolveProfile(mVo2Input, startUtc);
    writeVo2Log(RunVo2::Log::contextLine(startUtc, p, mVo2LogLine, sizeof(mVo2LogLine)));
    writeVo2Log(RunVo2::Log::headerLine(mVo2LogLine, sizeof(mVo2LogLine)));
}

void Service::writeVo2Log(size_t len)
{
    if (!mVo2LogFile || len == 0) {
        return;
    }
    size_t written = 0;
    if (!mVo2LogFile->write(mVo2LogLine, len, written) || written != len || !mVo2LogFile->flush()) {
        // A full or failing disk must not touch the run: stop logging it.
        LOG_WARNING("Tuning log write failed; logging stopped for this run\n");
        closeVo2Log(false);
    }
}

void Service::closeVo2Log(bool remove)
{
    if (!mVo2LogFile) {
        return;
    }
    if (mVo2LogFile->isOpen()) {
        mVo2LogFile->close();
    }
    if (remove) {
        mVo2LogFile->remove();
    }
    mVo2LogFile.reset();
}
