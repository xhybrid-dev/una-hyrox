/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   X Weigh-in background logic. W0: the first look.
 *
 * Started from the SDK's Waypoint tutorial service (its lifecycle and its
 * SDK::AppConfig handling); the GPS is gone. What it gathers:
 *   - the UNA profile's height and weight, from RequestSystemSettings (the
 *     SDK's Workout example reads the weight the same way);
 *   - the watch's daily resting HR, from HEART_RATE_METRICS_DAILY (as
 *     HybridX Run does);
 *   - birth year, birth month and sex, the phone settings.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "Service.hpp"

#include "WeighProfile.hpp"

#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateMetrics.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include <ctime>

namespace {

/// How often the daily HR metrics report, ms. With the default of 0 the
/// simulator's sensor never reports (hybridx-run NOTES, R2).
constexpr float kHrMetricsPeriodMs = 60000.0f;

/// How long to wait for the kernel's answer to RequestSystemSettings, ms.
/// The SDK's Workout and Treadmill examples use the same.
constexpr uint32_t kSettingsTimeoutMs = 100;

} // namespace

Service::Service(SDK::Kernel &kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mSensorHrMetrics(SDK::Sensor::Type::HEART_RATE_METRICS_DAILY, kHrMetricsPeriodMs)
    , mBirthYear(0)
    , mBirthMonth(0)
    , mState {}
    , mGUIStarted(false)
{
    (void)kernel;
}

void Service::loadConfiguration()
{
    if (!mConfig) {
        return;
    }

    // getInt() clamps to the declared bounds, so these casts cannot truncate.
    mBirthYear  = static_cast<uint16_t>(mConfig->getInt(WeighInConfig::kBirthYear));
    mBirthMonth = static_cast<uint8_t>(mConfig->getInt(WeighInConfig::kBirthMonth));
    mState.sex  = static_cast<uint8_t>(mConfig->getInt(WeighInConfig::kSex));
    mState.configLoaded = mConfig->isLoaded();

    LOG_INFO("settings %s: birth %u/%u, sex %u\n",
             mState.configLoaded ? "loaded" : "not found (defaults)",
             static_cast<unsigned>(mBirthMonth), static_cast<unsigned>(mBirthYear),
             static_cast<unsigned>(mState.sex));
}

void Service::readProfile()
{
    mState.settingsRead     = false;
    mState.heightCm         = 0;
    mState.weightHundredths = 0;

    auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel);
    if (!msg) {
        LOG_WARNING("no message for RequestSystemSettings\n");
        return;
    }
    if (!msg.send(kSettingsTimeoutMs) || !msg.ok()) {
        LOG_WARNING("RequestSystemSettings was not answered\n");
        return;
    }

    mState.settingsRead     = true;
    mState.heightCm         = msg->heightCm;
    mState.weightHundredths = XWeighIn::kgToHundredths(msg->weightKg);

    // The raw values, for the W0 record: the weight's hundredths show the
    // profile's resolution. Whole and fraction separately: no %f.
    LOG_INFO("profile: height %lu cm, weight %ld.%02ld kg, imperial %s\n",
             static_cast<unsigned long>(msg->heightCm),
             static_cast<long>(mState.weightHundredths / 100),
             static_cast<long>(mState.weightHundredths % 100),
             msg->imperialUnits ? "yes" : "no");
}

void Service::run()
{
    LOG_INFO("thread started\n");

    // Read the configuration here rather than in the constructor. The file the
    // companion app wrote is read once, so a change made on the phone applies
    // the next time this app starts.
    mConfig.reset(new SDK::AppConfig(mKernel, WeighInConfig::kFileName,
                                     WeighInConfig::kFields,
                                     WeighInConfig::kFieldCount));
    loadConfiguration();

    mSensorHrMetrics.connect();

    while (true) {
        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, 1000)) {
            switch (msg->getType()) {
            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
                mSensorHrMetrics.disconnect();
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
                onStartGUI();
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped, exiting service\n");
                // The kernel does not stop a service when its GUI closes, and
                // nothing else will ever reclaim the thread. X Weigh-in has no
                // reason to run without its screen.
                onStopGUI();
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto event = static_cast<SDK::Message::Sensor::EventData *>(msg);
                SDK::Sensor::DataBatch data(event->data, event->count,
                                            event->stride);
                onSdlNewData(event->handle, data);
            } break;

            default:
                break;
            }

            // Release message after processing
            mKernel.comm.releaseMessage(msg);
        }
    }

    LOG_INFO("thread stopped\n");
}

void Service::onStartGUI()
{
    mGUIStarted = true;

    // Read the profile each time the screen opens, so a weight changed in the
    // UNA app shows without restarting the watch (one of Gate W0's checks).
    readProfile();

    const std::time_t now = std::time(nullptr);
    mState.clockSet = XWeighIn::clockIsSet(now);
    mState.ageYears = XWeighIn::ageAt(mBirthYear, mBirthMonth, now);
    if (!mState.clockSet) {
        LOG_WARNING("The clock is not set: no age until it is\n");
    }

    sendFirstLook();
}

void Service::onStopGUI()
{
    mGUIStarted = false;
    mSensorHrMetrics.disconnect();
}

void Service::onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch &data)
{
    if (!mSensorHrMetrics.matchesDriver(handle)) {
        return;
    }

    SDK::SensorDataParser::HeartRateMetrics parser(data[0]);
    if (!parser.isDataValid()) {
        return;
    }

    const float   rhr = parser.getRhr();
    const uint8_t bpm = (rhr > 0.0f && rhr < 255.0f) ? static_cast<uint8_t>(rhr + 0.5f) : 0;
    if (bpm == mState.restHr) {
        return;
    }

    LOG_INFO("Resting HR from the watch: %u\n", static_cast<unsigned>(bpm));
    mState.restHr = bpm;
    if (mGUIStarted) {
        sendFirstLook();
    }
}

void Service::sendFirstLook()
{
    auto look = SDK::make_msg<CustomMessage::FirstLook>(mKernel);
    if (!look) {
        return;
    }
    look->state = mState;
    look.send();
}
