/**
 ******************************************************************************
 * @file    ScreenManager.cpp
 * @brief   Owns the active screen and switches between them (see the header).
 ******************************************************************************
 */

#include "gui/screens/ScreenManager.hpp"

#include "lvgl.h"

#define LOG_MODULE_PRX      "ScreenManager"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "gui/model/Model.hpp"
#include "gui/screens/FreshStartScreen.hpp"
#include "gui/screens/HomeScreen.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/screens/ShieldScreen.hpp"
#include "gui/screens/SummitScreen.hpp"

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
        LOG_WARNING("No screen for id %u\n", static_cast<unsigned>(id));
        return;
    }

    Screen* old = mCurrent;
    if (old) {
        old->onHide();
        mModel->bind(nullptr);
    }

    next->create();
    lv_screen_load(next->root());
    mCurrent = next;

    if (old) {
        delete old;
    }

    mModel->bind(next);
    next->onShow();

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    LOG_INFO("LVGL pool: %u/%u B used, peak %u%%, frag %u%%\n",
             static_cast<unsigned>(mon.total_size - mon.free_size),
             static_cast<unsigned>(mon.total_size),
             static_cast<unsigned>(mon.max_used) * 100u / static_cast<unsigned>(mon.total_size),
             static_cast<unsigned>(mon.frag_pct));
}

Screen* ScreenManager::create(ScreenId id)
{
    Model& m = *mModel;
    switch (id) {
        case ScreenId::Home:       return new HomeScreen(m);
        case ScreenId::Summit:     return new SummitScreen(m);
        case ScreenId::Shield:     return new ShieldScreen(m);
        case ScreenId::FreshStart: return new FreshStartScreen(m);
    }
    return nullptr;
}
