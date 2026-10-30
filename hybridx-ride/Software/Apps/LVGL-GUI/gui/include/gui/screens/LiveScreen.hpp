/**
 ******************************************************************************
 * @file    LiveScreen.hpp
 * @brief   The ride on the wrist: faces inside a ring in the heart rate's
 *          zone colour, and the lap toast.
 *
 * Faces (L1 back, L2 forward, round the lot). Indoors there are four:
 *   - Main:   the lap number, the lap time (large), the total time, heart
 *             rate, and its zone;
 *   - Totals: total time, calories, laps and the auto lap;
 *   - Zones:  time so far in each heart-rate zone;
 *   - Clock:  the time of day and the battery.
 * Outdoors the main face leads with the speed (large), then the distance and
 * the total time, heart rate and zone; a Lap face follows it (lap time, lap
 * distance, lap average speed); the totals become time, distance, climb and
 * calories; the zones and the clock are the same. Units follow the watch
 * profile (km, km/h and metres, or miles, mph and feet).
 * R2 is the lap button (a 3 s lockout stops a double tap), R1 opens the action
 * menu. The toast shows for about 2 s after a lap (R2 or the auto lap), over
 * any face; any click dismisses it, and R2 still works through it. This screen
 * never exits the app on idle: a ride is not abandoned because nobody
 * pressed anything.
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

    void onSessionUpdate(const Ride::SessionSnapshot& snapshot) override;
    void onLapEvent(const Ride::LapEventData& event) override;

    enum Face : uint8_t { FaceMain, FaceLap, FaceTotals, FaceZones, FaceClock, FaceCount };

protected:
    void build() override;

private:
    static constexpr uint8_t kTotalRows = 5;   ///< The zones face's five; the totals face uses four

    // One face exists at a time, built when it is shown and deleted when it is
    // left, as the toast is: the LVGL pool is small (36 KB) and a screen change
    // holds the old and the new at once, so the live screen keeps to what is
    // on show.
    void showFace(uint8_t face);
    void destroyFace();
    void buildMain();
    void buildTotals();
    void buildZones();
    void buildClock();
    void redraw();
    void redrawMain(const Ride::SessionSnapshot& s);
    void redrawLap(const Ride::SessionSnapshot& s);
    /// The face @p dir (+1 or -1) steps to: the Lap face is outdoor only.
    uint8_t stepFace(int dir) const;
    void redrawTotals(const Ride::SessionSnapshot& s);
    void redrawZones(const Ride::SessionSnapshot& s);
    void redrawClock(const Ride::SessionSnapshot& s);
    void showToast(const Ride::LapEventData& e);
    void hideToast();
    static void toastCb(lv_timer_t* t);

    uint8_t mFace = FaceMain;

    lv_obj_t* mRing = nullptr;

    // The face on show (only its own pointers are set)
    lv_obj_t* mFaceRoot  = nullptr;
    lv_obj_t* mLapNo     = nullptr;
    lv_obj_t* mLapTime   = nullptr;
    lv_obj_t* mInfo      = nullptr;
    lv_obj_t* mHr        = nullptr;
    lv_obj_t* mZone      = nullptr;
    lv_obj_t* mRowName[kTotalRows]  = {};
    lv_obj_t* mRowValue[kTotalRows] = {};
    lv_obj_t* mClockLap   = nullptr;
    lv_obj_t* mClock      = nullptr;
    lv_obj_t* mBatteryPct = nullptr;
    std::unique_ptr<Widgets::Title>   mFaceTitle;
    std::unique_ptr<Widgets::Battery> mBattery;

    // The lap toast, while it shows
    lv_obj_t*   mToast      = nullptr;
    lv_timer_t* mToastTimer = nullptr;

    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // LIVE_SCREEN_HPP
