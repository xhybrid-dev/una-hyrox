/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   X Weigh-in background logic.
 *
 * Started from the SDK's Waypoint tutorial service (its lifecycle and its
 * SDK::AppConfig handling). What it does:
 *   - at start: reads the phone settings, brings the log and its SharedData
 *     copy in step (LogStore::reconcile), and reads the log into the trend;
 *   - when the screen opens: reads the UNA profile (height, and the weight the
 *     weigh-in screen starts from before there is a log), and sends the home
 *     state and the last 30 days;
 *   - on SAVE_WEIGHIN: files the weigh-in under the local date and time,
 *     appends it to both files, feeds the trend and sends everything again.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "Service.hpp"

#include "BodyMetrics.hpp"
#include "Calendar.hpp"
#include "LogStore.hpp"
#include "Trend.hpp"
#include "WeighEntry.hpp"
#include "WeighProfile.hpp"

#include "SDK/Messages/MessageGuard.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include <ctime>

namespace {

/// How long to wait for the kernel's answer to RequestSystemSettings, ms.
/// The SDK's Workout and Treadmill examples use the same.
constexpr uint32_t kSettingsTimeoutMs = 100;

/// The trend, built from the log. Static rather than a member or a local:
/// about 1.2 KB, kept off the service's stack and off the heap.
XWeighIn::Trend sTrend;

const char *reconcileText(XWeighIn::LogStore::Reconcile r)
{
    using XWeighIn::LogStore::Reconcile;
    switch (r) {
        case Reconcile::Nothing:   return "in step";
        case Reconcile::Restored:  return "restored from the SharedData copy";
        case Reconcile::Refreshed: return "SharedData copy refreshed";
        case Reconcile::Failed:    return "FAILED to bring the copy in step";
    }
    return "?";
}

} // namespace

Service::Service(SDK::Kernel &kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mBirthYear(0)
    , mBirthMonth(0)
    , mSex(0)
    , mHeightCm(0)
    , mProfileHundredths(0)
    , mRejected(0)
    , mGUIStarted(false)
{
    (void)kernel;
}

Service::Now Service::now() const
{
    Now n;
    n.utc = std::time(nullptr);
    std::tm local {};
    localtime_r(&n.utc, &local);
    n.clockOk = XWeighIn::clockIsSet(n.utc);
    n.day     = XWeighIn::daysFromCivil(local.tm_year + 1900, static_cast<uint32_t>(local.tm_mon + 1),
                                        static_cast<uint32_t>(local.tm_mday));
    n.minute  = static_cast<uint16_t>(local.tm_hour * 60 + local.tm_min);
    return n;
}

void Service::loadConfiguration()
{
    if (!mConfig) {
        return;
    }
    // getInt() clamps to the declared bounds, so these casts cannot truncate.
    mBirthYear  = static_cast<uint16_t>(mConfig->getInt(WeighInConfig::kBirthYear));
    mBirthMonth = static_cast<uint8_t>(mConfig->getInt(WeighInConfig::kBirthMonth));
    mSex        = static_cast<uint8_t>(mConfig->getInt(WeighInConfig::kSex));

    LOG_INFO("settings %s: birth %u/%u, sex %u\n",
             mConfig->isLoaded() ? "loaded" : "not found (defaults)",
             static_cast<unsigned>(mBirthMonth), static_cast<unsigned>(mBirthYear),
             static_cast<unsigned>(mSex));
}

void Service::readProfile()
{
    auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel);
    if (!msg || !msg.send(kSettingsTimeoutMs) || !msg.ok()) {
        LOG_WARNING("RequestSystemSettings was not answered\n");
        return;
    }
    mHeightCm          = msg->heightCm;
    mProfileHundredths = XWeighIn::kgToHundredths(msg->weightKg);
    LOG_INFO("profile: height %lu cm, weight %ld.%02ld kg\n",
             static_cast<unsigned long>(mHeightCm),
             static_cast<long>(mProfileHundredths / 100),
             static_cast<long>(mProfileHundredths % 100));
}

void Service::loadLog()
{
    using namespace XWeighIn;

    const LogStore::Reconcile r = LogStore::reconcile(mKernel.fs);
    LOG_INFO("log: %s\n", reconcileText(r));

    sTrend.reset();
    const LogStore::ScanResult scan = LogStore::scan(mKernel.fs, LogStore::kLogFile, sTrend);
    mRejected = scan.rejected;

    const Summary s = sTrend.summary();
    LOG_INFO("log: %s, %lu weigh-ins on %lu days, %lu lines rejected, %lu skipped\n",
             scan.opened ? "read" : "none yet",
             static_cast<unsigned long>(scan.entries), static_cast<unsigned long>(s.days),
             static_cast<unsigned long>(scan.rejected), static_cast<unsigned long>(s.skipped));
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
    loadLog();

    while (true) {
        SDK::MessageBase *msg;
        if (mKernel.comm.getMessage(msg, 1000)) {
            switch (msg->getType()) {
            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
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
                mGUIStarted = false;
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;

            case CustomMessage::SAVE_WEIGHIN: {
                auto *save = static_cast<CustomMessage::SaveWeighIn *>(msg);
                saveWeighIn(save->hundredths);
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
    readProfile();
    sendHome();
    sendGraph();
}

void Service::saveWeighIn(int32_t hundredths)
{
    using namespace XWeighIn;
    using CustomMessage::SaveOutcome;

    const Now n = now();
    SaveOutcome outcome = SaveOutcome::Saved;

    Entry e;
    e.day        = n.day;
    e.minute     = n.minute;
    e.hundredths = hundredths;

    if (!n.clockOk) {
        // Without a date the weigh-in cannot be filed; refuse rather than log
        // it under 1970.
        outcome = SaveOutcome::ClockNotSet;
    } else if (!isValidEntry(e)) {
        outcome = SaveOutcome::Invalid;
    } else if (!LogStore::appendBoth(mKernel.fs, e)) {
        outcome = SaveOutcome::WriteFailed;
    } else {
        sTrend.onEntry(e);
    }

    LOG_INFO("weigh-in %ld.%02ld kg: %u\n",
             static_cast<long>(hundredths / 100), static_cast<long>(hundredths % 100),
             static_cast<unsigned>(outcome));

    SDK::send_msg<CustomMessage::SaveResult>(mKernel, outcome, hundredths);
    if (outcome == SaveOutcome::Saved) {
        sendHome();
        sendGraph();
    }
}

void Service::sendHome()
{
    using namespace XWeighIn;

    auto msg = SDK::make_msg<CustomMessage::HomeState>(mKernel);
    if (!msg) {
        return;
    }
    const Now     n = now();
    const Summary s = sTrend.summary();

    CustomMessage::HomeData &h = msg->home;
    h.latest        = s.latest;
    h.trend         = s.trend;
    h.weekChange    = s.weekChange;
    h.hasWeekChange = s.hasWeekChange;
    h.lastDay       = s.lastDay;
    h.days          = static_cast<uint16_t>(s.days > 0xFFFF ? 0xFFFF : s.days);
    h.rejected      = static_cast<uint16_t>(mRejected > 0xFFFF ? 0xFFFF : mRejected);
    h.clockSet      = n.clockOk;
    h.today         = n.clockOk ? n.day : 0;
    h.entryStart    = startValue(s.latest, mProfileHundredths);
    h.entryFrom     = static_cast<uint8_t>(s.latest > 0             ? CustomMessage::EntryFrom::Log
                                           : mProfileHundredths > 0 ? CustomMessage::EntryFrom::Profile
                                                                    : CustomMessage::EntryFrom::Default);
    h.heightCm      = mHeightCm;
    h.ageYears      = ageAt(mBirthYear, mBirthMonth, n.utc);
    h.sex           = mSex;
    h.bmiTenths     = bmiTenths(s.trend, mHeightCm);
    h.bmrKcal       = bmrKcal(s.trend, mHeightCm, h.ageYears, mSex);

    msg.send();
}

void Service::sendGraph()
{
    auto msg = SDK::make_msg<CustomMessage::GraphData>(mKernel);
    if (!msg) {
        return;
    }
    XWeighIn::fillWindow(sTrend, msg->window);
    msg.send();
}
