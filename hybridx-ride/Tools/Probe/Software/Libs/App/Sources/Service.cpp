/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   The bike probe's service (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "ProbeText.hpp"

#include "SDK/Messages/AccessoryMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"

#define LOG_MODULE_PRX      "Probe"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{
constexpr uint32_t kHrLogEveryMs = 5000;   ///< the heart rate goes in the CSV at most this often
}

Service::Service(SDK::Kernel& kernel)
    : mKernel(SDK::KernelProviderService::GetInstance().getKernel())
    , mSensorHr(SDK::Sensor::Type::HEART_RATE_EX, 1000, 1000)
{
    (void)kernel;
}

uint32_t Service::now() const
{
    return static_cast<uint32_t>(mKernel.sys.getTimeMs() - mStartMs);   // unsigned: the clock wraps
}

void Service::run()
{
    LOG_INFO("bike probe started\n");
    mStartMs = mKernel.sys.getTimeMs();

    while (true) {
        SDK::MessageBase* msg;
        if (!mKernel.comm.getMessage(msg, 1000)) {
            continue;
        }
        switch (msg->getType()) {
            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
                release();
                mSensorHr.disconnect();
                if (mCsv) {
                    mCsv->close();
                }
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
                openCsv();
                mSensorHr.connect();
                line("R1: ask the kernel");
                sendState();
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped, exiting service\n");
                release();
                mSensorHr.disconnect();
                if (mCsv) {
                    mCsv->close();
                }
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::EVENT_ACCESSORY_STATUS: {
                auto* e = static_cast<SDK::Message::Accessory::EventStatus*>(msg);
                char name[sizeof(e->name) + 1] = {};
                std::memcpy(name, e->name, sizeof(e->name));   // NUL-terminated whatever the kernel sent
                onStatus(e->kind, e->state, name);
            } break;

            case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                auto* e = static_cast<SDK::Message::Sensor::EventData*>(msg);
                SDK::Sensor::DataBatch batch(e->data, e->count, e->stride);
                onSensor(e->handle, batch);
            } break;

            case CustomMessage::NEXT_REQUEST:
                nextRequest();
                break;

            default:
                break;
        }
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::nextRequest()
{
    // Release what the last step asked for, then ask for the next kinds. The reserved bits are
    // plain numbers here: the SDK defines only HRM (SDK/Messages/AccessoryMessages.hpp).
    release();
    mStep = mNext;
    mNext = static_cast<uint8_t>((mNext + 1) % RideProbe::kStepCount);
    const RideProbe::Step& s = RideProbe::kSteps[mStep];

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::Accessory::RequestPrepare>();
    if (msg == nullptr) {
        line("no message to send");
        return;
    }
    msg->kinds = s.kinds;
    mKernel.comm.sendMessage(msg);
    mKernel.comm.releaseMessage(msg);
    mRequestMs = now();

    char t[40];
    std::snprintf(t, sizeof(t), "Ask 0x%lx %s", static_cast<unsigned long>(s.kinds), s.label);
    line(t);
    csv("request", s.kinds, -1, s.label);
    LOG_INFO("RequestPrepare kinds 0x%lx (%s)\n", static_cast<unsigned long>(s.kinds), s.label);
    sendState();
}

void Service::release()
{
    auto* msg = mKernel.comm.allocateMessage<SDK::Message::Accessory::RequestRelease>();
    if (msg != nullptr) {
        msg->kinds = 0;   // everything
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::onStatus(uint32_t kind, uint8_t state, const char* name)
{
    char t[40];
    RideProbe::statusLine(t, sizeof(t), static_cast<uint32_t>(now() - mRequestMs), kind, state, name);
    line(t);
    csv("status", kind, state, name);
    LOG_INFO("EventStatus kind 0x%lx state %u name \"%s\"\n", static_cast<unsigned long>(kind),
             static_cast<unsigned>(state), name);
}

void Service::onSensor(uint16_t handle, SDK::Sensor::DataBatch& data)
{
    if (!mSensorHr.matchesDriver(handle) || data.size() == 0) {
        return;
    }
    SDK::SensorDataParser::HeartRateEx p(data[data.size() - 1]);
    if (!p.isDataValid()) {
        return;
    }
    const uint8_t src = static_cast<uint8_t>(p.getSource());
    const float   ext = p.getExternalBpm();
    const uint8_t bpm = (ext > 0.0f && ext < 255.0f) ? static_cast<uint8_t>(ext + 0.5f) : 0;
    const bool changed = src != mHrSource || (bpm == 0) != (mExtBpm == 0);
    mHrSource = src;
    mExtBpm   = bpm;
    if (changed || static_cast<uint32_t>(now() - mLastHrLog) >= kHrLogEveryMs) {
        mLastHrLog = now();
        csv("hr", 0, -1, "");
        if (changed) {
            char t[40];
            std::snprintf(t, sizeof(t), "HR source %u, strap %u bpm", static_cast<unsigned>(src),
                          static_cast<unsigned>(bpm));
            line(t);
        }
        sendState();
    }
}

void Service::line(const char* text)
{
    auto msg = SDK::make_msg<CustomMessage::ProbeLine>(mKernel);
    if (!msg) {
        return;
    }
    std::strncpy(msg->text, text, sizeof(msg->text) - 1);
    msg.send();
}

void Service::openCsv()
{
    if (mCsv) {
        return;
    }
    // Named by the UTC second it began, so no time-zone code is needed.
    char path[48];
    std::snprintf(path, sizeof(path), "Logs/probe_%lu.csv", static_cast<unsigned long>(std::time(nullptr)));
    mKernel.fs.mkdir("Logs");
    mCsv = mKernel.fs.file(path);
    if (!mCsv || !mCsv->open(true, true)) {
        mCsv.reset();
        LOG_WARNING("could not open %s\n", path);
        return;
    }
    size_t w = 0;
    mCsv->write(RideProbe::kCsvHeader, std::strlen(RideProbe::kCsvHeader), w);
    mCsv->flush();
    LOG_INFO("logging to %s\n", path);
}

void Service::csv(const char* event, uint32_t kind, int state, const char* name)
{
    if (!mCsv) {
        return;
    }
    char row[128];
    const uint32_t request = mStep < RideProbe::kStepCount ? RideProbe::kSteps[mStep].kinds : 0;
    const size_t len = RideProbe::csvRow(row, sizeof(row), now(), event, request, kind, state, name, mHrSource,
                                         mExtBpm);
    size_t w = 0;
    if (!mCsv->write(row, len, w) || w != len || !mCsv->flush()) {
        LOG_WARNING("CSV write failed; logging stopped\n");
        mCsv->close();
        mCsv.reset();
    }
}

void Service::sendState()
{
    auto msg = SDK::make_msg<CustomMessage::ProbeState>(mKernel);
    if (!msg) {
        return;
    }
    msg->step     = mStep;
    msg->next     = mNext;
    msg->hrSource = mHrSource;
    msg->extBpm   = mExtBpm;
    msg->logging  = static_cast<bool>(mCsv);
    msg.send();
}
