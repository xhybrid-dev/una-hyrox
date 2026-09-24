/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   The Streak Probe's service (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <new>

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"

#include "ProbeMessages.hpp"
#include "WeekMath.hpp"

#define LOG_MODULE_PRX   "Probe"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{
constexpr const char* kReportFile  = "probe.txt";
constexpr const char* kHistoryFile = "probe-history.txt";

/// The report, built line by line. Static so it is off the 10 KB stack; a
/// report that outgrows it is cut short and says so.
constexpr size_t kReportMax = 12 * 1024;
char             sReport[kReportMax];

/// The runner's fixed buffers are static for the same reason.
alignas(Probe::Runner) uint8_t sRunnerStorage[sizeof(Probe::Runner)];

char sChunk[256];
} // namespace

Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel)
{
}

void Service::run()
{
    LOG_INFO("Started\n");
    probe();

    const uint32_t startMs = mKernel.sys.getTimeMs();
    while (true) {
        SDK::MessageBase* msg = nullptr;
        if (mKernel.comm.getMessage(msg, skWaitMs)) {
            switch (msg->getType()) {
                case SDK::MessageType::COMMAND_APP_STOP:
                    mKernel.comm.releaseMessage(msg);
                    return;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                    mGuiStarted = true;
                    sendResult();
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                    mGuiStarted = false;
                    break;

                default:
                    break;
            }
            mKernel.comm.releaseMessage(msg);
        }
        if (!mGuiStarted && mKernel.sys.getTimeMs() - startMs >= skStartupGraceMs) {
            LOG_INFO("No GUI: exiting\n");
            return;
        }
    }
}

// -- Probe::Host ------------------------------------------------------------------

void Service::line(const char* text)
{
    LOG_INFO("%s\n", text);
    const size_t len = std::strlen(text);
    if (mReportLen + len + 2 > kReportMax) {
        static constexpr char kCut[] = "(report cut short)\n";
        if (mReportLen + sizeof(kCut) <= kReportMax && mReportLen > 0 && sReport[mReportLen - 1] != ')') {
            std::memcpy(sReport + mReportLen, kCut, sizeof(kCut) - 1);
            mReportLen += sizeof(kCut) - 1;
        }
        return;
    }
    std::memcpy(sReport + mReportLen, text, len);
    mReportLen += len;
    sReport[mReportLen++] = '\n';
}

uint32_t Service::nowMs()
{
    return mKernel.sys.getTimeMs();
}

// -- The run ------------------------------------------------------------------------

void Service::probe()
{
    mReportLen = 0;
    line("HybridX Streak probe, app " APP_NAME);
    clock();
    settings();
    glance();

    // Run the file-system checks, keeping the service's own fields.
    Probe::Result own = mResult;
    auto* runner = new (sRunnerStorage) Probe::Runner(mKernel.fs, *this);
    runner->run(mResult);
    runner->~Runner();
    mResult.glance         = own.glance;
    mResult.glanceWidth    = own.glanceWidth;
    mResult.glanceHeight   = own.glanceHeight;
    mResult.glanceControls = own.glanceControls;
    mResult.utcOffsetMin   = own.utcOffsetMin;
    mResult.utc            = own.utc;
    mResult.run            = historyRuns() + 1u;

    save();
}

void Service::clock()
{
    const std::time_t utc = std::time(nullptr);
    std::tm           local {};
    localtime_r(&utc, &local);

    // The zone offset: local calendar time read back as if it were UTC,
    // minus UTC (NOTES E.5; the same sum as ActivityWriter::epochToLocal).
    const int64_t localDay  = Streak::WeekMath::daysFromCivil(local.tm_year + 1900, local.tm_mon + 1u, local.tm_mday);
    const int64_t localSecs = localDay * 86400 + local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec;
    mResult.utc          = static_cast<uint32_t>(utc);
    mResult.utcOffsetMin = static_cast<int32_t>((localSecs - static_cast<int64_t>(utc)) / 60);

    snprintf(mStamp, sizeof(mStamp), "%04d-%02d-%02d %02d:%02d", local.tm_year + 1900, local.tm_mon + 1,
             local.tm_mday, local.tm_hour, local.tm_min);
    char text[96];
    snprintf(text, sizeof(text), "Clock: local %s (weekday %d), UTC %lu, offset %ld min", mStamp, local.tm_wday,
             static_cast<unsigned long>(utc), static_cast<long>(mResult.utcOffsetMin));
    line(text);
}

void Service::settings()
{
    char text[128];
    auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel);
    if (msg && msg.send(100) && msg.ok()) {
        snprintf(text, sizeof(text), "Settings: language %u, %s, %s, %s; targets %lu steps, %lu active min",
                 static_cast<unsigned>(msg->languageId), msg->imperialUnits ? "imperial" : "metric",
                 msg->timeFormat ? "12 h" : "24 h", msg->dateMonthFirst ? "month first" : "day first",
                 static_cast<unsigned long>(msg->steps), static_cast<unsigned long>(msg->activityMin));
    } else {
        snprintf(text, sizeof(text), "Settings: no reply");
    }
    line(text);
}

void Service::glance()
{
    char text[96];
    auto gc = SDK::make_msg<SDK::Message::RequestGlanceConfig>(mKernel);
    if (gc && gc.send(100) && gc.ok()) {
        mResult.glance         = Probe::Check::Ok;
        mResult.glanceWidth    = gc->width;
        mResult.glanceHeight   = gc->height;
        mResult.glanceControls = static_cast<uint16_t>(gc->maxControls);
        snprintf(text, sizeof(text), "Glance config (asked by a Utility app): %dx%d, %lu controls", gc->width,
                 gc->height, static_cast<unsigned long>(gc->maxControls));
    } else {
        mResult.glance = Probe::Check::Failed;
        snprintf(text, sizeof(text), "Glance config (asked by a Utility app): no reply");
    }
    line(text);
}

uint16_t Service::historyRuns()
{
    auto file = mKernel.fs.file(kHistoryFile);
    if (!file || !file->open(false, false)) {
        return 0;
    }
    uint16_t lines = 0;
    size_t   got   = 0;
    while (file->read(sChunk, sizeof(sChunk), got) && got > 0) {
        for (size_t i = 0; i < got; ++i) {
            if (sChunk[i] == '\n' && lines < UINT16_MAX) {
                ++lines;
            }
        }
    }
    file->close();
    return lines;
}

void Service::save()
{
    // probe.txt: this run's full report.
    if (auto file = mKernel.fs.file(kReportFile); file && file->open(true, true)) {
        size_t written = 0;
        file->write(sReport, mReportLen, written);
        file->close();
        LOG_INFO("Wrote %s (%u bytes)\n", kReportFile, static_cast<unsigned>(written));
    } else {
        LOG_ERROR("Cannot write %s\n", kReportFile);
    }

    // probe-history.txt: one line per run, appended.
    char   entry[320];
    size_t len = Probe::Runner::historyLine(mResult, mStamp, entry, sizeof(entry) - 1);
    entry[len++] = '\n';
    if (auto file = mKernel.fs.file(kHistoryFile); file && file->open(true, false)) {
        size_t written = 0;
        file->seek(file->size());
        file->write(entry, len, written);
        file->close();
    } else {
        LOG_ERROR("Cannot write %s\n", kHistoryFile);
    }
}

void Service::sendResult()
{
    SDK::send_msg<CustomMessage::ProbeResult>(mKernel, mResult);
}
