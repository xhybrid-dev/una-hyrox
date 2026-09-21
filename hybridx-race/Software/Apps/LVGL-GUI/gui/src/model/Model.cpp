/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of HybridX Race and its link to the service process.
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#include <cstring>

#define LOG_MODULE_PRX   "Model"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

Model::Model()
        : modelListener(nullptr)
        , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(this);

    std::memcpy(mHrThresholds, CustomMessage::kHrThresholdsDefault, sizeof(mHrThresholds));
}

// =============================================================================
// Controls
// =============================================================================

void Model::handleKeyEvent(uint8_t key)
{
    if (isClick(key)) {
        resetIdleTimer();
    }
}

void Model::resetIdleTimer()
{
    mIdleTimer = App::Config::kScreenTimeoutSteps;
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

// =============================================================================
// Date and time
// =============================================================================

void Model::getDate(uint8_t &month, uint8_t &day, uint8_t &weekday) const
{
    month = static_cast<uint8_t>(mTime.tm_mon);
    day = static_cast<uint8_t>(mTime.tm_mday);
    weekday = static_cast<uint8_t>(mTime.tm_wday);
}

void Model::getTime(uint8_t &h, uint8_t &m, uint8_t &s) const
{
    h = static_cast<uint8_t>(mTime.tm_hour);
    m = static_cast<uint8_t>(mTime.tm_min);
    s = static_cast<uint8_t>(mTime.tm_sec);
}

// =============================================================================
// Settings
// =============================================================================

void Model::saveSettings(const Settings &settings)
{
    mSettings = settings;
    SDK::send_msg<CustomMessage::SettingsSave>(mKernel, mSettings);
}

void Model::setFormat(Race::Format format)
{
    mSettings.format = format;
    // The format is only persisted when the race starts, which is also when
    // the service writes it: no need to round-trip a settings save here.
}

// =============================================================================
// Race control
// =============================================================================

void Model::raceStart()
{
    SDK::send_msg<CustomMessage::RaceStart>(mKernel, mSettings.format);
}

void Model::raceSplit()
{
    // Brief 7.4: the split is stamped here, at the press, not in the service.
    // A GUI tick is 100 ms, and that latency would otherwise bias every split.
    SDK::send_msg<CustomMessage::RaceSplit>(mKernel, mKernel.sys.getTimeMs());
}

void Model::raceUndoSplit()
{
    SDK::send_msg<CustomMessage::RaceUndoSplit>(mKernel);
}

void Model::racePause()
{
    SDK::send_msg<CustomMessage::RacePause>(mKernel);
}

void Model::raceResume()
{
    SDK::send_msg<CustomMessage::RaceResume>(mKernel);
}

void Model::raceFinishEarly()
{
    SDK::send_msg<CustomMessage::RaceFinishEarly>(mKernel);
}

void Model::raceUndoFinish()
{
    SDK::send_msg<CustomMessage::RaceUndoFinish>(mKernel);
}

void Model::raceSave()
{
    SDK::send_msg<CustomMessage::RaceSave>(mKernel);
}

void Model::raceDiscard()
{
    SDK::send_msg<CustomMessage::RaceDiscard>(mKernel);
}

void Model::requestSummary()
{
    SDK::send_msg<CustomMessage::SummaryRequest>(mKernel);
}

// =============================================================================
// Lifecycle
// =============================================================================

void Model::onStart()
{
    LOG_INFO("Started\n");
    mIsRunning = true;
    resetIdleTimer();
}

void Model::onFrame()
{
    if (mIsRunning) {
        decIdleTimer();
    }
}

void Model::onResume()
{
    mIsRunning = true;
    resetIdleTimer();
}

void Model::onSuspend()
{
    mIsRunning = false;
    if (modelListener != nullptr) {
        modelListener->onSuspend();
    }
}

void Model::onStop()
{
    mIsRunning = false;
}

void Model::decIdleTimer()
{
    if (mIdleTimer > 0u) {
        if (--mIdleTimer == 0u && modelListener != nullptr) {
            modelListener->onIdleTimeout();
        }
    }
}

bool Model::isClick(uint8_t key)
{
    namespace Btn = SDK::GUI::Button;
    return key == Btn::L1 || key == Btn::L2 || key == Btn::R1 || key == Btn::R2;
}

// =============================================================================
// Messages from the service
// =============================================================================

bool Model::customMessageHandler(SDK::MessageBase *message)
{
    // State is always updated; with no screen bound the notifications go to a
    // listener that ignores them.
    static ModelListener sNullListener;
    ModelListener *listener = (modelListener != nullptr) ? modelListener : &sNullListener;

    switch (message->getType()) {

    case CustomMessage::SETTINGS_UPDATE: {
        const auto *msg = static_cast<CustomMessage::SettingsUpd *>(message);
        mSettings = msg->settings;
        mTimeFormat12h = msg->is12HourFormat;
        if (msg->hrThresholdsCount > 0u) {
            std::memcpy(mHrThresholds, msg->hrThresholds, sizeof(mHrThresholds));
            mHrThresholdsCount = msg->hrThresholdsCount;
        }
        listener->onSettings(mSettings);
    } break;

    case CustomMessage::LOCAL_TIME: {
        const auto *msg = static_cast<CustomMessage::LocalTime *>(message);
        mTime.tm_hour = msg->hour;
        mTime.tm_min = msg->minute;
        mTime.tm_sec = msg->second;
        mTime.tm_mon = msg->month;
        mTime.tm_mday = msg->day;
        mTime.tm_wday = msg->weekday;
        listener->onTime(msg->hour, msg->minute, msg->second);
        listener->onDate(0u, msg->month, msg->day, msg->weekday);
    } break;

    case CustomMessage::BATTERY: {
        const auto *msg = static_cast<CustomMessage::Battery *>(message);
        mBatteryLevel = msg->level;
        listener->onBatteryLevel(mBatteryLevel);
    } break;

    case CustomMessage::HR_UPDATE: {
        const auto *msg = static_cast<CustomMessage::HrUpdate *>(message);
        mIdleHr = msg->bpm;
        listener->onIdleHr(mIdleHr);
    } break;

    case CustomMessage::RACE_STATE_UPDATE: {
        const auto *msg = static_cast<CustomMessage::RaceStateUpd *>(message);
        mTrackState = msg->state;
        listener->onRaceState(mTrackState);
    } break;

    case CustomMessage::RACE_DATA_UPDATE: {
        const auto *msg = static_cast<CustomMessage::RaceDataUpd *>(message);
        mRaceData = msg->data;
        listener->onRaceData(mRaceData);
    } break;

    case CustomMessage::SPLIT_EVENT: {
        const auto *msg = static_cast<CustomMessage::SplitEvent *>(message);
        mLastSplit = msg->split;
        listener->onSplit(mLastSplit);
    } break;

    case CustomMessage::RACE_FINISHED: {
        const auto *msg = static_cast<CustomMessage::RaceFinished *>(message);
        mRaceCompleted = msg->completed;
        listener->onRaceFinished(mRaceCompleted);
    } break;

    case CustomMessage::SUMMARY_META: {
        const auto *msg = static_cast<CustomMessage::SummaryMeta *>(message);
        // Meta arrives first and resets the accumulator; the pages that follow
        // fill it in. Copied, never referenced: the GUI owns this.
        mSummary = ActivitySummary {};
        mSummary.format = msg->format;
        mSummary.roxzone = msg->roxzone;
        mSummary.completed = msg->completed;
        mSummary.totalMs = msg->totalMs;
        mSummary.runsMs = msg->runsMs;
        mSummary.stationsMs = msg->stationsMs;
        mSummary.roxzoneMs = msg->roxzoneMs;
        mSummary.hrAvg = msg->hrAvg;
        mSummary.hrMax = msg->hrMax;
        mSummary.count = 0u;  // counted up as pages land
        mSummary.valid = (msg->segmentCount == 0u);
    } break;

    case CustomMessage::SUMMARY_PAGE: {
        const auto *msg = static_cast<CustomMessage::SummaryPage *>(message);
        for (uint8_t i = 0u; i < msg->count; ++i) {
            const uint8_t index = static_cast<uint8_t>(msg->firstIndex + i);
            if (index >= Race::kMaxSegments) {
                break;  // no MMU: never write past the array
            }
            SegmentSummary &out = mSummary.segments[index];
            out.type = static_cast<uint8_t>(msg->entries[i].desc.type);
            out.round = msg->entries[i].desc.round;
            out.stationId = msg->entries[i].desc.stationId;
            out.durationMs = msg->entries[i].activeMs;
            out.hrAvg = msg->entries[i].hrAvg;
            out.hrMax = msg->entries[i].hrMax;

            if (index + 1u > mSummary.count) {
                mSummary.count = static_cast<uint8_t>(index + 1u);
            }
        }
        mSummary.valid = (mSummary.count > 0u);
        listener->onSummary(mSummary);
    } break;

    case CustomMessage::ACCESSORY_STATUS: {
        const auto *msg = static_cast<CustomMessage::AccessoryStatusUpd *>(message);
        mAccessoryState = msg->state;
        listener->onAccessoryStatus(msg->state, msg->name);
    } break;

    default:
        break;
    }

    return true;
}
