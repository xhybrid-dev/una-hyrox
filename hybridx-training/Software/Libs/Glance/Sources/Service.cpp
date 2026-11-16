/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   X Training's glance (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstdio>
#include <ctime>

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"

#include "GlanceLayout.hpp"
#include "GlanceView.hpp"

#define LOG_MODULE_PRX   "Glance"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{
/// The watch's clock before this (2026-01-01) was never set, as the app's and Streak's glance have it.
constexpr int64_t kClockFloorUtc = 1767225600;

GlancePoint_t at(int32_t x, int32_t y)
{
    return { static_cast<uint16_t>(x < 0 ? 0 : x), static_cast<uint16_t>(y < 0 ? 0 : y) };
}

GlanceSize_t size(int32_t w, int32_t h)
{
    return { static_cast<uint16_t>(w < 0 ? 0 : w), static_cast<uint16_t>(h < 0 ? 0 : h) };
}

/// Read @p path whole into @p out (at most cap - 1 bytes, then a '\0').
/// Returns the length, 0 if it could not be opened or is empty.
size_t readFile(const SDK::Kernel& kernel, const char* path, char* out, size_t cap)
{
    out[0]    = '\0';
    auto file = kernel.fs.file(path);
    if (!file || !file->open(false, false)) {
        return 0;
    }
    size_t total = 0;
    size_t got   = 0;
    while (total < cap - 1 && file->read(out + total, cap - 1 - total, got) && got > 0) {
        total += got;
    }
    file->close();
    out[total] = '\0';
    return total;
}

/// Today's date as yyyymmdd in the watch's zone; 0 while the clock has not been set.
uint32_t localToday()
{
    const std::time_t utc = std::time(nullptr);
    if (static_cast<int64_t>(utc) < kClockFloorUtc) {
        return 0;
    }
    std::tm local {};
    localtime_r(&utc, &local);
    return static_cast<uint32_t>(local.tm_year + 1900) * 10000u + static_cast<uint32_t>(local.tm_mon + 1) * 100u
         + static_cast<uint32_t>(local.tm_mday);
}

// Static rather than locals: about 3 KB between them, kept off the glance service's 10 KB stack and
// off the heap.
char                     sText[TrainingGlance::kMaxFileBytes + 1];
TrainingGlance::View     sView;
TrainingGlance::Layout   sLayout;
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

    // The app's shared file. None yet: the app has not been opened since the glance was added, and
    // the view says so. The app saves by a rename, keeping the old copy as .bak, so a look during
    // a save reads that.
    size_t len = readFile(mKernel, TrainingGlance::kSharedFile, sText, sizeof(sText));
    if (len == 0) {
        char bak[64];
        std::snprintf(bak, sizeof(bak), "%s.bak", TrainingGlance::kSharedFile);
        len = readFile(mKernel, bak, sText, sizeof(sText));
    }
    const uint32_t today = localToday();
    TrainingGlance::makeView(sText, len, today, sView);
    LOG_INFO("training_today.json: %u bytes, today %lu: state %u\n", static_cast<unsigned>(len),
             static_cast<unsigned long>(today), static_cast<unsigned>(sView.state));

    TrainingGlance::layout(sView, mWidth, mHeight, mMaxControls, sLayout);
    LOG_INFO("Glance area %dx%d, %u controls: layout %u, %u controls\n", mWidth, mHeight,
             static_cast<unsigned>(mMaxControls), static_cast<unsigned>(sLayout.kind),
             static_cast<unsigned>(sLayout.count));

    using TrainingGlance::Spec;
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
        upd->controlsNumber = static_cast<uint32_t>(mForm.size());
        upd->controls       = mForm.data();
        upd.send(100);
    }
    mForm.setValid();
}
