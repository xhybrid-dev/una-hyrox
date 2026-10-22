/**
 ******************************************************************************
 * @file    LiveScreen.hpp
 * @brief   The session on the wrist: three faces inside a ring in the phase's
 *          colour, and the lap toast.
 *
 * Faces (L1 back, L2 forward, round the three):
 *   - Main:   the phase, the lap time (large), round and total time, heart
 *             rate, and the recovery line;
 *   - Totals: time so far in each phase, and the total;
 *   - Clock:  the time of day and the battery.
 * R2 is the lap button (a 3 s lockout stops a double tap), R1 opens the action
 * menu. The toast shows for about 2 s after a lap, over any face; any click
 * dismisses it, and R2 still works through it. This screen never exits the
 * app on idle: a session is not abandoned because nobody pressed anything.
 ******************************************************************************
 */

#ifndef LIVE_SCREEN_HPP
#define LIVE_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class LiveScreen : public Screen
{
public:
    explicit LiveScreen(Model& model);
    ~LiveScreen() override;

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    void onSessionUpdate(const Sauna::SessionSnapshot& snapshot) override;
    void onLapEvent(const Sauna::LapEventData& event) override;

    enum Face : uint8_t { FaceMain, FaceTotals, FaceClock, FaceCount };

protected:
    void build() override;

private:
    static constexpr uint8_t kTotalRows = 4;

    // One face exists at a time, built when it is shown and deleted when it is
    // left, as the toast is: the LVGL pool is small (36 KB) and a screen change
    // holds the old and the new at once, so the live screen keeps to what is
    // on show.
    void showFace(uint8_t face);
    void destroyFace();
    void buildMain();
    void buildTotals();
    void buildClock();
    void redraw();
    void redrawMain(const Sauna::SessionSnapshot& s);
    void redrawTotals(const Sauna::SessionSnapshot& s);
    void redrawClock(const Sauna::SessionSnapshot& s);
    void showToast(const Sauna::LapEventData& e);
    void hideToast();
    static void toastCb(lv_timer_t* t);

    uint8_t mFace = FaceMain;

    lv_obj_t* mRing = nullptr;

    // The face on show (only its own pointers are set)
    lv_obj_t* mFaceRoot  = nullptr;
    lv_obj_t* mPhase     = nullptr;
    lv_obj_t* mLapTime   = nullptr;
    lv_obj_t* mInfo      = nullptr;
    lv_obj_t* mHr        = nullptr;
    lv_obj_t* mRecovery  = nullptr;
    lv_obj_t* mRowName[kTotalRows]  = {};
    lv_obj_t* mRowValue[kTotalRows] = {};
    lv_obj_t* mClockPhase = nullptr;
    lv_obj_t* mClock      = nullptr;
    lv_obj_t* mBatteryPct = nullptr;
    std::unique_ptr<Widgets::Title>   mTotalsTitle;
    std::unique_ptr<Widgets::Battery> mBattery;

    // The lap toast, while it shows
    lv_obj_t*   mToast      = nullptr;
    lv_timer_t* mToastTimer = nullptr;

    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // LIVE_SCREEN_HPP
