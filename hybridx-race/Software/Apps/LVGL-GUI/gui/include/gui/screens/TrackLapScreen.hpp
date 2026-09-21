/**
 ******************************************************************************
 * @file    TrackLapScreen.hpp
 * @brief   Split toast: the name and time of the segment just finished, shown
 *          briefly, then back to the race (brief 8.2 item 4).
 *
 * It does not block the next split beyond the lockout: R2 here goes straight
 * back to the race screen, so a fast transition is never swallowed by the
 * toast.
 ******************************************************************************
 */

#ifndef TRACK_LAP_SCREEN_HPP
#define TRACK_LAP_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackLapScreen : public Screen
{
public:
    explicit TrackLapScreen(Model& model);
    ~TrackLapScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);
    void backToRace();

    lv_timer_t* mDismiss = nullptr;
    lv_obj_t*   mSegment = nullptr;
    lv_obj_t*   mTime    = nullptr;
    std::unique_ptr<Widgets::Title> mTitle;
};

#endif // TRACK_LAP_SCREEN_HPP
