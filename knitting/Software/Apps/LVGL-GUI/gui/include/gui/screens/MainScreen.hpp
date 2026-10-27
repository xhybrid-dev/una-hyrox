/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Knitting's one screen: the rows in this set (big), the set number,
 *          and the total rows (small).
 *
 *          R1 taps count a row; holding R1 takes the last row back.
 *          L1 starts the next set.
 *          L2 taps only show a hint; holding L2 resets everything.
 *          R2 exits.
 *
 *          The holds are timed here, from the press and release codes, by
 *          HoldTimer (the kernel sends no long-press code). The bottom line
 *          is the total, and borrows its place for a hold's prompt and for
 *          a short notice once something has been done.
 ******************************************************************************
 */

#ifndef MAIN_SCREEN_HPP
#define MAIN_SCREEN_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "HoldTimer.hpp"
#include "RowCounter.hpp"
#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class MainScreen : public ModelListener
{
public:
    /// How long the L2 hold takes to reset, and the R1 hold to undo a row. The reset is
    /// short because the kernel answers a hold on L2 with the music controls, at a time the
    /// SDK does not give; 1.5 s was too long and the music came first (NOTES, "The L2 hold").
    static constexpr uint32_t kResetHoldMs = 800;
    static constexpr uint32_t kUndoHoldMs  = 800;
    /// How long a hint or a notice stays on the bottom line.
    static constexpr uint32_t kNoticeMs    = 1500;

    explicit MainScreen(Model& model);
    ~MainScreen();

    MainScreen(const MainScreen&)            = delete;
    MainScreen& operator=(const MainScreen&) = delete;

    /// The LVGL screen object, to pass to lv_screen_load().
    lv_obj_t* root() const { return mRoot; }

    // ModelListener
    void onCounterUpdate(const Knitting::CounterState& state) override;
    void onSuspend() override;

private:
    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);
    static void tickCb(lv_timer_t* t);

    void show(const Knitting::CounterState& state);
    void tick();
    void showNotice(const char* text, uint32_t nowMs, uint32_t color);
    /// Draw the bottom line and the progress bar for the current moment.
    void refreshStatus(uint32_t nowMs);
    /// Run the 40 ms timer while a hold or a notice needs it, no longer.
    void updateTimer(uint32_t nowMs);

    Model&    mModel;
    lv_obj_t* mRoot     = nullptr;
    lv_obj_t* mRows     = nullptr;
    lv_obj_t* mSet      = nullptr;
    lv_obj_t* mStatus   = nullptr;   ///< "Total N", or a prompt, or a notice
    lv_obj_t* mBarTrack = nullptr;
    lv_obj_t* mBarFill  = nullptr;

    Knitting::CounterState mCounter {};
    Knitting::HoldTimer    mReset { kResetHoldMs };
    Knitting::HoldTimer    mUndo  { kUndoHoldMs };

    lv_timer_t* mTimer       = nullptr;
    char        mNotice[24]  = {};
    uint32_t    mNoticeColor = 0;
    bool        mHasNotice   = false;
    uint32_t    mNoticeAt    = 0;

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // MAIN_SCREEN_HPP
