/**
 ******************************************************************************
 * @file    ScreenManager.cpp
 * @brief   Owns the active screen and switches between them.
 ******************************************************************************
 */

#include "gui/screens/ScreenManager.hpp"

#include "lvgl.h"

#define LOG_MODULE_PRX      "ScreenManager"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "gui/model/Model.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/screens/MainScreen.hpp"
#include "gui/screens/MenuSettingsScreen.hpp"
#include "gui/screens/TrackStartConfirmScreen.hpp"
#include "gui/screens/TrackScreen.hpp"
#include "gui/screens/TrackActionScreen.hpp"
#include "gui/screens/TrackHoldConfirmScreen.hpp"
#include "gui/screens/TrackLapScreen.hpp"
#include "gui/screens/TrackResultScreen.hpp"
#include "gui/screens/TrackSummaryScreen.hpp"

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
            // LVGL's pool had no room for the request. Leave the flag clear
            // so the next goTo() tries again rather than waiting for a call
            // that will never come.
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

    // Build and show the new screen, then free the old one: the same order
    // as the TouchGFX MVP application. Both widget trees exist for the
    // duration of the switch, so the pool must hold the largest such pair;
    // the peak logged below is what to size it by.
    next->create();
    lv_screen_load(next->root());
    mCurrent = next;

    if (old) {
        // ~Screen() runs the derived destructors (which stop timers and
        // animations) before it deletes the LVGL tree, so a widget destructor
        // may still touch its objects.
        delete old;
    }

    mModel->bind(next);
    next->onShow();

    // Peak use of LVGL's static pool (LV_MEM_SIZE in lv_conf.h), logged per
    // screen so the pool can be sized to what the app actually needs.
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
        case ScreenId::Main:              return new MainScreen(m);
        case ScreenId::MenuSettings:      return new MenuSettingsScreen(m);

        case ScreenId::RaceStartConfirm:  return new TrackStartConfirmScreen(m);
        case ScreenId::Race:              return new TrackScreen(m);
        case ScreenId::RaceAction:        return new TrackActionScreen(m);
        case ScreenId::RaceHoldConfirm:   return new TrackHoldConfirmScreen(m);
        case ScreenId::RaceSplit:         return new TrackLapScreen(m);
        case ScreenId::RaceFinished:      return new TrackResultScreen(m, TrackResultScreen::Result::Finished);
        case ScreenId::RaceSaved:         return new TrackResultScreen(m, TrackResultScreen::Result::Saved);
        case ScreenId::RaceDiscarded:     return new TrackResultScreen(m, TrackResultScreen::Result::Discarded);
        case ScreenId::RaceSummary:       return new TrackSummaryScreen(m);
    }
    return nullptr;
}
