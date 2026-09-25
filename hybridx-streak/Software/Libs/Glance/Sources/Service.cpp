/**
 ******************************************************************************
 * @file    Service.cpp
 * @brief   HybridX Streak's glance (see the header).
 ******************************************************************************
 */

#include "Service.hpp"

#include <cstdio>
#include <ctime>
#include <new>

#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Messages/MessageGuard.hpp"

#include "ActivityScanner.hpp"
#include "GlanceLayout.hpp"
#include "StateCodec.hpp"
#include "StreakModel.hpp"
#include "WeekMath.hpp"

#define LOG_MODULE_PRX   "Glance"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{
constexpr const char* kPublicFile     = "../SharedData/HybridX/streak.json";
constexpr int64_t     kClockFloorUtc  = 1767225600;   ///< 2026-01-01, as the app (PLAN 6.1)

GlancePoint_t at(int32_t x, int32_t y)
{
    return { static_cast<uint16_t>(x < 0 ? 0 : x), static_cast<uint16_t>(y < 0 ? 0 : y) };
}

GlanceSize_t size(int32_t w, int32_t h)
{
    return { static_cast<uint16_t>(w < 0 ? 0 : w), static_cast<uint16_t>(h < 0 ? 0 : h) };
}

// Placed at start-up: far larger than the glance service's stack.
alignas(Streak::StreakModel) uint8_t     sModelStorage[sizeof(Streak::StreakModel)];
alignas(Streak::ActivityScanner) uint8_t sScannerStorage[sizeof(Streak::ActivityScanner)];
Streak::Found  sFound[Service::kMaxNew];
char           sScratch[Streak::StateCodec::kMaxBytes];
Glance::Layout sLayout;
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
    auto* model   = new (sModelStorage) Streak::StreakModel();
    auto* scanner = new (sScannerStorage) Streak::ActivityScanner();

    // The app's public copy; none means the app has never been opened.
    Glance::State        state = Glance::State::Normal;
    Streak::HomeView     view;
    const auto           source = Streak::StateCodec::load(mKernel.fs, kPublicFile, model->state(), sScratch,
                                                           sizeof(sScratch));
    if (source == Streak::StateCodec::Source::None) {
        state = Glance::State::NoStreak;
    } else {
        const std::time_t utc = std::time(nullptr);
        std::tm           local {};
        localtime_r(&utc, &local);
        const int32_t today = Streak::WeekMath::daysFromCivil(local.tm_year + 1900,
                                                               static_cast<uint32_t>(local.tm_mon + 1),
                                                               static_cast<uint32_t>(local.tm_mday));
        if (static_cast<int64_t>(utc) < kClockFloorUtc) {
            view       = model->view(today);
            view.flags = static_cast<uint8_t>(view.flags | Streak::HomeView::kClockUnset);
        } else {
            // Project to now: what the app would show if opened, not saved.
            int32_t from = 0, to = 0;
            model->scanWindow(today, from, to);
            const size_t   n = scanner->scan(mKernel.fs, "HybridXStreak", from, to, *model, sFound, kMaxNew);
            Streak::Events ignored;
            model->update(today, sFound, n, ignored);
            view = model->view(today);
            if (model->state().pendingMissed > 0) {
                view.flags = static_cast<uint8_t>(view.flags | Streak::HomeView::kDecisionPending);
            }
            LOG_INFO("Projected with %u new activities\n", static_cast<unsigned>(n));
        }
    }

    Glance::layout(view, state, mWidth, mHeight, mMaxControls, sLayout);
    LOG_INFO("Glance area %dx%d, %u controls: layout %u, %u controls\n", mWidth, mHeight,
             static_cast<unsigned>(mMaxControls), static_cast<unsigned>(sLayout.kind),
             static_cast<unsigned>(sLayout.count));

    for (uint8_t i = 0; i < sLayout.count; ++i) {
        const Glance::Spec& c = sLayout.items[i];
        switch (c.type) {
            case Glance::Spec::Type::Text:
                mForm.createText().init(at(c.x, c.y), size(c.w, c.h), c.text, static_cast<GlanceFont_t>(c.font),
                                        static_cast<GlanceColor_t>(c.colour), static_cast<GlanceAlignH_t>(c.align));
                break;
            case Glance::Spec::Type::Line:
                mForm.createLine().init(at(c.x, c.y), at(c.x2, c.y2), static_cast<GlanceColor_t>(c.colour));
                break;
            case Glance::Spec::Type::Rect:
                mForm.createRect().init(at(c.x, c.y), size(c.w, c.h), static_cast<GlanceColor_t>(c.colour),
                                        static_cast<GlanceColor_t>(c.colour), c.fill);
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
