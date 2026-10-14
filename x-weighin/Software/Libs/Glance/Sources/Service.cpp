/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   X Weigh-in's glance (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <ctime>

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"

#include "Calendar.hpp"
#include "GlanceLayout.hpp"
#include "GlanceView.hpp"
#include "LogStore.hpp"
#include "Trend.hpp"
#include "WeighProfile.hpp"

#define LOG_MODULE_PRX   "Glance"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{
GlancePoint_t at(int32_t x, int32_t y)
{
    return { static_cast<uint16_t>(x < 0 ? 0 : x), static_cast<uint16_t>(y < 0 ? 0 : y) };
}

GlanceSize_t size(int32_t w, int32_t h)
{
    return { static_cast<uint16_t>(w < 0 ? 0 : w), static_cast<uint16_t>(h < 0 ? 0 : h) };
}

// Static rather than locals: the trend is about 1.2 KB and the layout about
// 1 KB, kept off the glance service's 10 KB stack and off the heap.
XWeighIn::Trend          sTrend;
XWeighIn::Glance::View   sView;
XWeighIn::Glance::Layout sLayout;
} // namespace

Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel)
{
}

void Service::run()
{
    LOG_INFO("Started\n");

    while (true) {
        SDK::MessageBase* msg = nullptr;
        if (!mKernel.comm.getMessage(msg)) {
            continue;
        }

        switch (msg->getType()) {
            case SDK::MessageType::EVENT_GLANCE_START:
                if (!mBuilt) {
                    if (!configure()) {
                        LOG_ERROR("No glance area\n");
                        mKernel.comm.releaseMessage(msg);
                        return;
                    }
                    build();
                }
                break;

            case SDK::MessageType::EVENT_GLANCE_TICK:
                pushIfChanged();
                break;

            case SDK::MessageType::COMMAND_APP_STOP:
            case SDK::MessageType::EVENT_GLANCE_STOP:
                mKernel.comm.releaseMessage(msg);
                return;

            default:
                break;
        }
        mKernel.comm.releaseMessage(msg);
    }
}

bool Service::configure()
{
    if (auto gc = SDK::make_msg<SDK::Message::RequestGlanceConfig>(mKernel)) {
        if (gc.send(100) && gc.ok() && gc->width > 0 && gc->height > 0 && gc->maxControls >= 2) {
            mWidth       = gc->width;
            mHeight      = gc->height;
            mMaxControls = gc->maxControls;
            mForm.setWidth(static_cast<uint16_t>(mWidth));
            mForm.setHeight(static_cast<uint16_t>(mHeight));
            return true;
        }
    }
    return false;
}

void Service::build()
{
    mBuilt = true;

    // The app's SharedData copy of the log. None yet: the app has never saved
    // a weigh-in, and the view says so.
    sTrend.reset();
    const XWeighIn::LogStore::ScanResult scan =
        XWeighIn::LogStore::scan(mKernel.fs, XWeighIn::LogStore::kSharedFile, sTrend);

    const std::time_t utc = std::time(nullptr);
    std::tm           local {};
    localtime_r(&utc, &local);
    const int32_t today = XWeighIn::daysFromCivil(local.tm_year + 1900, static_cast<uint32_t>(local.tm_mon + 1),
                                                  static_cast<uint32_t>(local.tm_mday));
    const bool clockSet = XWeighIn::clockIsSet(utc);
    XWeighIn::Glance::makeView(sTrend, today, clockSet, sView);
    LOG_INFO("log: %s, %lu weigh-ins, %lu lines rejected; clock %s\n", scan.opened ? "read" : "none yet",
             static_cast<unsigned long>(scan.entries), static_cast<unsigned long>(scan.rejected),
             clockSet ? "set" : "NOT set");

    XWeighIn::Glance::layout(sView, mWidth, mHeight, mMaxControls, sLayout);
    LOG_INFO("Glance area %dx%d, %u controls: layout %u, %u controls\n", mWidth, mHeight,
             static_cast<unsigned>(mMaxControls), static_cast<unsigned>(sLayout.kind),
             static_cast<unsigned>(sLayout.count));

    using XWeighIn::Glance::Spec;
    for (uint8_t i = 0; i < sLayout.count; ++i) {
        const Spec& c = sLayout.items[i];
        switch (c.type) {
            case Spec::Type::Text:
                mForm.createText().init(at(c.x, c.y), size(c.w, c.h), c.text, static_cast<GlanceFont_t>(c.font),
                                        static_cast<GlanceColor_t>(c.colour), static_cast<GlanceAlignH_t>(c.align));
                break;
            case Spec::Type::Rect:
                mForm.createRect().init(at(c.x, c.y), size(c.w, c.h), static_cast<GlanceColor_t>(c.colour),
                                        static_cast<GlanceColor_t>(c.colour), true);
                break;
        }
    }
}

void Service::pushIfChanged()
{
    if (!mBuilt || !mForm.isInvalid()) {
        return;
    }
    if (auto upd = SDK::make_msg<SDK::Message::RequestGlanceUpdate>(mKernel)) {
        upd->name           = APP_NAME;
        upd->controls       = mForm.data();
        upd->controlsNumber = static_cast<uint32_t>(mForm.size());
        upd.send(100);
    }
    mForm.setValid();
}
