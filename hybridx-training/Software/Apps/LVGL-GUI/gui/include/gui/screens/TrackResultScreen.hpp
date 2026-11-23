/**
 ******************************************************************************
 * @file    TrackResultScreen.hpp
 * @brief   "Saved" / "Discarded" confirmation shown for two seconds.
 *
 * Port of the Run app's TrackSavedView and TrackDiscardedView, which differ
 * only in text, icon, the service command they issue and where they go next.
 ******************************************************************************
 */

#ifndef TRACK_RESULT_SCREEN_HPP
#define TRACK_RESULT_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackResultScreen : public Screen
{
public:
    enum class Result : uint8_t { Saved, Discarded };

    TrackResultScreen(Model& model, Result result);
    ~TrackResultScreen() override;

    void onShow() override;
    void onStreakLine() override;   // the Streak line after a save
    void onPlanSaved() override;    // X Training: how the planned session counts

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);
    void showStreakLine();   ///< the Streak line (and the load) in place of the text
    void startDismiss(uint32_t ms);

    Result      mResult;
    lv_timer_t* mDismiss = nullptr;
    lv_obj_t*   mTick    = nullptr;   ///< the tick or cross; "Load 39" takes its place
    lv_obj_t*   mLoad    = nullptr;   ///< the session's training load, when saved with one
    lv_obj_t*   mForm    = nullptr;   ///< where form now stands, under the load (Streak NOTES "S10")
    lv_obj_t*   mText    = nullptr;   ///< the confirmation's text; the Streak line replaces it
    bool        mPlanShown     = false;   ///< X Training: the plan's lines are on show
    bool        mStreakPending = false;   ///< a Streak line came while they were: it follows
    char        mPlanText[112] = "";   ///< up to three lines (the week, how it went, what is next)
    std::unique_ptr<Widgets::Title> mTitle;
};

#endif // TRACK_RESULT_SCREEN_HPP
