/**
 ******************************************************************************
 * @file    Screen.cpp
 * @brief   The screen base, the switcher and the page frame (see Screens.hpp).
 ******************************************************************************
 */

#include "gui/screens/Screens.hpp"

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

#define LOG_MODULE_PRX      "Screens"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace Draw  = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;

// -- Screen ------------------------------------------------------------------

Screen::Screen(Model& model)
    : mModel(model)
{
}

Screen::~Screen()
{
    mButtons.reset();
    if (mRoot) {
        lv_obj_delete(mRoot);
        mRoot = nullptr;
    }
}

void Screen::create()
{
    if (mRoot) {
        return;
    }
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &Screen::keyEventCb, LV_EVENT_KEY, this);
    build();
}

void Screen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<Screen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

// -- ScreenManager -------------------------------------------------------------

ScreenManager& ScreenManager::instance()
{
    static ScreenManager sInstance;
    return sInstance;
}

void ScreenManager::start(Model& model, ScreenId first)
{
    mModel = &model;
    switchNow(first);
}

void ScreenManager::goTo(ScreenId id)
{
    mPending = id;
    if (!mPendingSet) {
        if (lv_async_call(&ScreenManager::asyncCb, this) != LV_RESULT_OK) {
            // LVGL's pool had no room for the request: leave the flag clear so
            // the next goTo() tries again.
            LOG_ERROR("Switch to screen %u could not be scheduled\n", static_cast<unsigned>(id));
            return;
        }
        mPendingSet = true;
    }
}

void ScreenManager::asyncCb(void* user)
{
    auto* self = static_cast<ScreenManager*>(user);
    self->mPendingSet = false;
    self->switchNow(self->mPending);
}

void ScreenManager::switchNow(ScreenId id)
{
    Screen* next = create(id);
    if (!next) {
        return;
    }
    Screen* old = mCurrent;
    if (old) {
        mModel->bind(nullptr);
    }

    // Build and show the new screen, then free the old one (RunLVGL's order).
    next->create();
    lv_screen_load(next->root());
    mCurrent = next;
    delete old;

    mModel->bind(next);
    next->onShow();

    // Peak use of LVGL's static pool (LV_MEM_SIZE), per screen, to size it by.
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    LOG_INFO("screen %u: LVGL pool %u/%u B used, peak %u%%\n",
             static_cast<unsigned>(id),
             static_cast<unsigned>(mon.total_size - mon.free_size),
             static_cast<unsigned>(mon.total_size),
             static_cast<unsigned>(mon.max_used) * 100u / static_cast<unsigned>(mon.total_size));
}

Screen* ScreenManager::create(ScreenId id)
{
    Model& m = *mModel;
    switch (id) {
        case ScreenId::Home:    return new HomeScreen(m);
        case ScreenId::Graph:   return new GraphScreen(m);
        case ScreenId::Recent:  return new RecentScreen(m);
        case ScreenId::Body:    return new BodyScreen(m);
        case ScreenId::WeighIn: return new WeighInScreen(m);
        case ScreenId::Saved:   return new SavedScreen(m);
    }
    return nullptr;
}

// -- PageScreen ------------------------------------------------------------------

void PageScreen::buildFrame(const char* title)
{
    if (title != nullptr && title[0] != '\0') {
        Draw::label(mRoot, &poppins_semibold_20, title, 40, 22, 160, LV_TEXT_ALIGN_CENTER, Color::LIME);
    }

    // Page dots, centred near the foot of the round display.
    constexpr int32_t kDotY   = 210;
    constexpr int32_t kStep   = 14;
    const int32_t     firstX  = 120 - (kStep * (kPageCount - 1)) / 2;
    for (uint8_t i = 0; i < kPageCount; ++i) {
        Draw::dot(mRoot, firstX + i * kStep, kDotY, 3,
                  i == mPage ? Color::WHITE : Color::GRAY_DARK);
    }

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::WHITE,    // UP: previous page
                  SDK::LVGL::Buttons::WHITE,    // DOWN: next page
                  SDK::LVGL::Buttons::GREEN,    // SELECT: weigh in
                  SDK::LVGL::Buttons::WHITE);   // BACK: leave
}

void PageScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: {
            const uint8_t p = static_cast<uint8_t>((mPage + kPageCount - 1) % kPageCount);
            mModel.setPage(p);
            ScreenManager::instance().goTo(static_cast<ScreenId>(p));
        } break;
        case Btn::L2: {
            const uint8_t p = static_cast<uint8_t>((mPage + 1) % kPageCount);
            mModel.setPage(p);
            ScreenManager::instance().goTo(static_cast<ScreenId>(p));
        } break;
        case Btn::R1:
            ScreenManager::instance().goTo(ScreenId::WeighIn);
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            break;   // press and release codes: nothing to do on a page
    }
}
