/**
 ******************************************************************************
 * @file    ScreenManager.hpp
 * @brief   Owns the active screen and switches between them.
 *
 * Copied from HybridX Race: a switch is requested with goTo() and performed on
 * the next lv_timer_handler() pass, never inside the callback that asked, so
 * a screen is never deleted under its own feet. The new screen is built
 * before the old one is freed; the LVGL pool peak is logged on every switch
 * (hybridx-race NOTES 5.6 for why that matters).
 ******************************************************************************
 */

#ifndef STREAK_SCREEN_MANAGER_HPP
#define STREAK_SCREEN_MANAGER_HPP

#include <cstdint>

class Model;
class Screen;

enum class ScreenId : uint8_t {
    Home,         ///< the mountain, the streak and this week
    Summit,       ///< a summit reached
    Shield,       ///< a week missed: spend a shield?
    FreshStart,   ///< the streak reset; the climb is kept
    Menu,         ///< This week, Log a session, Trophy case, Settings
    Week,         ///< this week's sessions, and why each counts or not
    Confirm,      ///< undo / exclude / include one of them?
    Log,          ///< log a session the watch did not record
    Trophy,       ///< summits, badges, bests
    Settings,     ///< the goal
    Value,        ///< one setting's choices
    Clock,        ///< the watch has lost the time
};

class ScreenManager
{
public:
    static ScreenManager& instance();

    void start(Model& model, ScreenId first);
    void goTo(ScreenId id);

private:
    ScreenManager() = default;

    static void asyncCb(void* user);
    void switchNow(ScreenId id);
    Screen* create(ScreenId id);

    Model*   mModel      = nullptr;
    Screen*  mCurrent    = nullptr;
    ScreenId mPending    = ScreenId::Home;
    bool     mPendingSet = false;
};

#endif // STREAK_SCREEN_MANAGER_HPP
