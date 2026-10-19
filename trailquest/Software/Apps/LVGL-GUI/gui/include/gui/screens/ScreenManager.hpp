/**
 ******************************************************************************
 * @file    ScreenManager.hpp
 * @brief   Owns the screen on display and switches between them.
 *
 * A switch is requested with goTo() and performed on the next pass of LVGL's
 * timers (lv_async_call), never inside the key or message callback that asked
 * for it, so a screen is not deleted under its own feet (HybridX Trail's
 * ScreenManager). The new screen's empty root is loaded and the old screen
 * freed before the new one is built, so LVGL's pool only ever holds one
 * screen's widgets (Trail's lesson: building first ran the pool out).
 *
 * No heap: a screen is built in one of two static slots, the one the screen
 * on display is not using.
 ******************************************************************************
 */

#ifndef TRAILQUEST_SCREEN_MANAGER_HPP
#define TRAILQUEST_SCREEN_MANAGER_HPP

#include <cstdint>

#include "View.hpp"

class Model;
class Screen;

enum class ScreenId : uint8_t {
    Title,
    Map,
    Obstacle,
    Duel,
    Offer,
    Pack,
    Clear,
    Story,
};

class ScreenManager
{
public:
    static ScreenManager& instance();

    /// Bind the model and show the first screen.
    void start(Model& model, ScreenId first);

    /// Request a switch; performed on the next frame.
    void goTo(ScreenId id);

    /// The screen for where the game stands.
    static ScreenId screenFor(const TrailQuest::WorldView& w);

    /// Log the use of LVGL's pool after building @p what.
    static void logPool(const char* what);

private:
    ScreenManager() = default;
    static void asyncCb(void* user);
    void    switchNow(ScreenId id);
    Screen* create(ScreenId id, void* slot);

    Model*   mModel      = nullptr;
    Screen*  mCurrent    = nullptr;
    uint8_t  mSlot       = 0;   ///< The slot mCurrent lives in.
    ScreenId mPending    = ScreenId::Title;
    bool     mPendingSet = false;
};

#endif // TRAILQUEST_SCREEN_MANAGER_HPP
