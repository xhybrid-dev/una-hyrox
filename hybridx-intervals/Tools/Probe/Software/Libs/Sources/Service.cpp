/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   The Intervals Probe's service (see the header).
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

#define LOG_MODULE_PRX   "Probe"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{
constexpr const char* kReportFile  = "probe.txt";
constexpr const char* kHistoryFile = "probe-history.txt";

/// The report, built line by line. Static so it is off the 10 KB stack.
constexpr size_t kReportMax = 4 * 1024;
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
        // Unsigned subtraction stays right across the millisecond clock's wrap.
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
        return;   // a report this short should never happen; drop silently rather than corrupt
    }
    std::memcpy(sReport + mReportLen, text, len);
    mReportLen += len;
    sReport[mReportLen++] = '\n';
}

// -- The run ------------------------------------------------------------------------

void Service::probe()
{
    mReportLen = 0;
    line("HybridX Intervals probe, app " APP_NAME);
    clock();

    auto* runner = new (sRunnerStorage) Probe::Runner(mKernel.fs, *this);
    Probe::Result own = mResult;   // keep the clock fields the runner doesn't touch
    runner->run(mResult);
    runner->~Runner();
    mResult.utc          = own.utc;
    mResult.utcOffsetMin = own.utcOffsetMin;
    mResult.run          = historyRuns() + 1u;

    line(Probe::verdictName(mResult.verdict));
    save();
}

void Service::clock()
{
    const std::time_t utc = std::time(nullptr);
    std::tm            local {};
    localtime_r(&utc, &local);

    // Local calendar time read back as if it were UTC, minus UTC: the zone
    // offset (hybridx-streak NOTES E.5, the same sum ActivityWriter uses).
    // Days-from-civil (Howard Hinnant), inlined: this probe has no Core lib to
    // borrow WeekMath from.
    int32_t  y     = local.tm_year + 1900;
    uint32_t m     = static_cast<uint32_t>(local.tm_mon + 1);
    const uint32_t d = static_cast<uint32_t>(local.tm_mday);
    y -= m <= 2 ? 1 : 0;
    const int32_t  era = (y >= 0 ? y : y - 399) / 400;
    const uint32_t yoe  = static_cast<uint32_t>(y - era * 400);
    const uint32_t mp   = m > 2 ? m - 3u : m + 9u;
    const uint32_t doy  = (153u * mp + 2u) / 5u + d - 1u;
    const uint32_t doe  = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    const int64_t  localDay  = era * 146097 + static_cast<int64_t>(doe) - 719468;
    const int64_t  localSecs = localDay * 86400 + local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec;
    mResult.utc          = static_cast<uint32_t>(utc);
    mResult.utcOffsetMin = static_cast<int32_t>((localSecs - static_cast<int64_t>(utc)) / 60);

    snprintf(mStamp, sizeof(mStamp), "%04d-%02d-%02d %02d:%02d:%02d", local.tm_year + 1900, local.tm_mon + 1,
             local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
    char text[96];
    snprintf(text, sizeof(text), "Clock: local %s, UTC %lu, offset %ld min", mStamp,
             static_cast<unsigned long>(utc), static_cast<long>(mResult.utcOffsetMin));
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
    if (auto file = mKernel.fs.file(kReportFile); file && file->open(true, true)) {
        size_t written = 0;
        file->write(sReport, mReportLen, written);
        file->close();
        LOG_INFO("Wrote %s (%u bytes)\n", kReportFile, static_cast<unsigned>(written));
    } else {
        LOG_ERROR("Cannot write %s\n", kReportFile);
    }

    char entry[256];
    const int len = snprintf(entry, sizeof(entry) - 1, "run %u | %s | %s | files %u | newest %s (%u B) | %s",
                             static_cast<unsigned>(mResult.run), mStamp, Probe::verdictName(mResult.verdict),
                             static_cast<unsigned>(mResult.fileCount),
                             mResult.newestName[0] ? mResult.newestName : "-",
                             static_cast<unsigned>(mResult.newestSize), mResult.preview);
    size_t elen = len > 0 ? static_cast<size_t>(len) : 0;
    if (elen >= sizeof(entry) - 1) {
        elen = sizeof(entry) - 2;
    }
    entry[elen++] = '\n';

    if (auto file = mKernel.fs.file(kHistoryFile); file && file->open(true, false)) {
        size_t written = 0;
        file->seek(file->size());
        file->write(entry, elen, written);
        file->close();
    } else {
        LOG_ERROR("Cannot write %s\n", kHistoryFile);
    }
}

void Service::sendResult()
{
    SDK::send_msg<CustomMessage::ProbeResult>(mKernel, mResult);
}
