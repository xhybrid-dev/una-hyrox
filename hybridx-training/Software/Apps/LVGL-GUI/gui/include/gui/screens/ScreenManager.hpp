/**
 ******************************************************************************
 * @file    ScreenManager.hpp
 * @brief   Owns the active screen and switches between them.
 *
 * The LVGL counterpart of the TouchGFX FrontendApplication's goto*Screen()
 * calls. A switch is requested with goTo() and performed on the next
 * lv_timer_handler() pass (via lv_async_call), never inside the event or
 * message callback that asked for it, so the requesting screen is not deleted
 * under its own feet. Screens are built on entry and destroyed on exit, as
 * TouchGFX views are.
 ******************************************************************************
 */

#ifndef SCREEN_MANAGER_HPP
#define SCREEN_MANAGER_HPP

#include <cstdint>

class Model;
class Screen;

enum class ScreenId : uint8_t {
    Main,
    PlanToday,      // X Training: today's planned session
    PlanWeek,       // X Training: This week
    PlanDay,        // X Training: a day This week opened
    PlanOverview,   // X Training: Plan
    PlanPaces,      // X Training: Paces
    NewPlan,        // X Training: New plan's choices
    NewPlanFiveK,   // X Training: the 5K time picker
    NewPlanDays,    // X Training: the run days
    NewPlanRace,    // X Training: the race date
    NewPlanConfirm, // X Training: the new plan, to start
    PlanAsk,        // X Training: the phone's new plan, start it or keep yours
    PlanChanges,    // X Training (TP5): move, skip, pause or end the plan
    PlanOffer,      // X Training (TP5): an offer, or a trial's question, answered
    // Structured workouts
    WorkoutToday,
    WorkoutList,
    WorkoutMenu,
    WorkoutDetails,
    // Settings
    MenuSettings,
    MenuAlerts,
    MenuAlertDistance,
    MenuAlertTime,
    MenuAlertDistanceSaved,
    MenuAlertTimeSaved,
    // Activity
    TrackStartConfirm,
    TrackIntervalsCountdown,
    Track,
    TrackIntervalsAlert,
    TrackIntervalsCompleted,
    TrackWorkoutStep,
    TrackAction,
    TrackHoldConfirm,
    TrackLap,
    TrackSaved,
    TrackDiscarded,
    TrackSummary,
    TreadmillDistance,   // HybridX Run: the console's distance after a treadmill run
};

class ScreenManager
{
public:
    static ScreenManager& instance();

    /// Log the use of LVGL's pool after building @p what (every screen switch
    /// does; HybridX Run's summary also does on changing face, as Trail's run
    /// screen).
    static void logPool(const char* what);

    /// Bind the model and show the first screen.
    void start(Model& model, ScreenId first);

    /// Request a switch; performed on the next frame. The Track screen is
    /// reached through the step card while one waits to be shown.
    void goTo(ScreenId id);

    /// The screen on show (the text-fit audit sends it keys).
    Screen* current() const { return mCurrent; }

private:
    ScreenManager() = default;

    static void asyncCb(void* user);
    void switchNow(ScreenId id);
    Screen* create(ScreenId id);

    Model*   mModel   = nullptr;
    Screen*  mCurrent = nullptr;
    ScreenId mPending = ScreenId::Main;
    bool     mPendingSet = false;
};

#endif // SCREEN_MANAGER_HPP
