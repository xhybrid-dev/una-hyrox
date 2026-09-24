/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   HybridX Streak's glance (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstdio>

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"

#define LOG_MODULE_PRX   "Glance"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{
/// Controls the full layout uses: the mountain (4 lines), its flag (1 rect),
/// three text lines. Below this the glance falls back to text alone.
constexpr uint32_t kFullLayoutControls = 8;

GlancePoint_t at(int32_t x, int32_t y)
{
    return { static_cast<uint16_t>(x < 0 ? 0 : x), static_cast<uint16_t>(y < 0 ? 0 : y) };
}

GlanceSize_t size(int32_t w, int32_t h)
{
    return { static_cast<uint16_t>(w < 0 ? 0 : w), static_cast<uint16_t>(h < 0 ? 0 : h) };
}
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
            LOG_INFO("Glance area %dx%d, %u controls\n", mWidth, mHeight, static_cast<unsigned>(mMaxControls));
            return true;
        }
    }
    return false;
}

void Service::build()
{
    mBuilt = true;

    // Demonstration content for the S0 first look (see the header).
    const char* kStreak = "7 week streak";
    const char* kWeek   = "2 of 3 this week";

    char probe[32];
    snprintf(probe, sizeof(probe), "area %dx%d, %u ctl", mWidth, mHeight, static_cast<unsigned>(mMaxControls));

    if (mMaxControls < kFullLayoutControls) {
        // Text alone: the streak, and this week.
        mForm.createText().init(at(0, 0), size(mWidth, mHeight / 2), kStreak, GLANCE_FONT_POPPINS_SEMIBOLD_20,
                                GLANCE_COLOR_GREEN, GLANCE_ALIGN_H_CENTER);
        mForm.createText().init(at(0, mHeight / 2), size(mWidth, mHeight / 2), kWeek, GLANCE_FONT_POPPINS_MEDIUM_18,
                                GLANCE_COLOR_WHITE, GLANCE_ALIGN_H_CENTER);
        return;
    }

    // A little mountain on the left, drawn in lines: the two flanks, the snow
    // line, and a flag on the top. The glance has no filled triangles.
    const int32_t m    = mHeight < 60 ? mHeight : 60;       // the mountain's box
    const int32_t x0   = 8;
    const int32_t base = mHeight - 4;
    const int32_t top  = mHeight - m + 14;
    const int32_t apex = x0 + m / 2;
    mForm.createLine().init(at(x0, base), at(apex, top), GLANCE_COLOR_TEAL);
    mForm.createLine().init(at(apex, top), at(x0 + m, base), GLANCE_COLOR_TEAL);
    mForm.createLine().init(at(apex - 6, top + 9), at(apex + 6, top + 9), GLANCE_COLOR_WHITE);
    mForm.createLine().init(at(apex, top), at(apex, top - 11), GLANCE_COLOR_WHITE);
    mForm.createRect().init(at(apex + 1, top - 11), size(9, 6), GLANCE_COLOR_GREEN, GLANCE_COLOR_GREEN, true);

    // The words on the right.
    const int32_t tx = x0 + m + 10;
    const int32_t tw = mWidth - tx;
    mForm.createText().init(at(tx, 0), size(tw, 24), kStreak, GLANCE_FONT_POPPINS_SEMIBOLD_20, GLANCE_COLOR_GREEN);
    mForm.createText().init(at(tx, 23), size(tw, 22), kWeek, GLANCE_FONT_POPPINS_MEDIUM_18, GLANCE_COLOR_WHITE);
    mForm.createText().init(at(tx, mHeight - 13), size(tw, 13), probe, GLANCE_FONT_POPPINS_MEDIUM_10,
                            GLANCE_COLOR_GRAY);
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
