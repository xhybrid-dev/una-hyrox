/**
 ******************************************************************************
 * @file    ScreenManager.cpp
 * @brief   Owns the screen on display and switches between them.
 ******************************************************************************
 */

#include "gui/screens/ScreenManager.hpp"

#include <cstddef>
#include <new>

#include "lvgl.h"

#define LOG_MODULE_PRX      "Screens"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "gui/model/Model.hpp"
#include "gui/screens/ClearScreen.hpp"
#include "gui/screens/DuelScreen.hpp"
#include "gui/screens/MapScreen.hpp"
#include "gui/screens/ObstacleScreen.hpp"
#include "gui/screens/OfferScreen.hpp"
#include "gui/screens/PackScreen.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/screens/StoryScreen.hpp"
#include "gui/screens/TitleScreen.hpp"

namespace
{

template <typename... T>
constexpr size_t maxSize()
{
    size_t m = 0;
    ((m = sizeof(T) > m ? sizeof(T) : m), ...);
    return m;
}

constexpr size_t kSlotBytes =
    maxSize<TitleScreen, MapScreen, ObstacleScreen, DuelScreen, OfferScreen, PackScreen, ClearScreen, StoryScreen>();

// Two slots: the screen on display, and the one being built to replace it.
alignas(alignof(std::max_align_t)) uint8_t sSlots[2][kSlotBytes];

} // namespace

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
            // so the next goTo() tries again.
            LOG_ERROR("switch to screen %u could not be scheduled\n", static_cast<unsigned>(id));
            return;
        }
        mPendingSet = true;
    }
}

ScreenId ScreenManager::screenFor(const TrailQuest::WorldView& w)
{
    using TrailQuest::RunPhase;
    if (!w.hasGame) {
        return ScreenId::Title;
    }
    switch (w.phase) {
        case RunPhase::Obstacle:   return ScreenId::Obstacle;
        case RunPhase::InDuel:     return ScreenId::Duel;
        case RunPhase::JoinOffer:  return ScreenId::Offer;
        case RunPhase::StageClear:
        case RunPhase::Finished:   return ScreenId::Clear;
        case RunPhase::AtNode:
        default:                   return ScreenId::Map;
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
    const uint8_t slot = mCurrent ? static_cast<uint8_t>(mSlot ^ 1u) : 0u;
    Screen* next = create(id, sSlots[slot]);
    if (!next) {
        LOG_WARNING("no screen for id %u\n", static_cast<unsigned>(id));
        return;
    }
    Screen* old = mCurrent;
    if (old) {
        mModel->bind(nullptr);
    }
    next->createRoot();
    lv_screen_load(next->root());
    mCurrent = next;
    mSlot    = slot;
    if (old) {
        // The derived destructors stop timers before ~Screen deletes the tree.
        old->~Screen();
    }
    next->create();
    mModel->bind(next);
    next->onShow();
    logPool("screen");
}

void ScreenManager::logPool(const char* what)
{
    // Peak use of LVGL's static pool, logged per screen so it can be sized to
    // what the app really needs.
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    LOG_INFO("LVGL pool (%s): %u/%u B used, peak %u%%, frag %u%%\n", what,
             static_cast<unsigned>(mon.total_size - mon.free_size), static_cast<unsigned>(mon.total_size),
             static_cast<unsigned>(mon.max_used) * 100u / static_cast<unsigned>(mon.total_size),
             static_cast<unsigned>(mon.frag_pct));
}

Screen* ScreenManager::create(ScreenId id, void* slot)
{
    Model& m = *mModel;
    switch (id) {
        case ScreenId::Title:    return new (slot) TitleScreen(m);
        case ScreenId::Map:      return new (slot) MapScreen(m);
        case ScreenId::Obstacle: return new (slot) ObstacleScreen(m);
        case ScreenId::Duel:     return new (slot) DuelScreen(m);
        case ScreenId::Offer:    return new (slot) OfferScreen(m);
        case ScreenId::Pack:     return new (slot) PackScreen(m);
        case ScreenId::Clear:    return new (slot) ClearScreen(m);
        case ScreenId::Story:    return new (slot) StoryScreen(m);
    }
    return nullptr;
}
