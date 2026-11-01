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

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);

    Result      mResult;
    lv_timer_t* mDismiss = nullptr;
    lv_obj_t*   mText    = nullptr;   ///< the confirmation's text; the Streak line replaces it
    std::unique_ptr<Widgets::Title> mTitle;
};

#endif // TRACK_RESULT_SCREEN_HPP
