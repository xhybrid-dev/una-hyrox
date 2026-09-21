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
    /// Finished is the "race over, not yet saved" screen of brief 8.2 item 6;
    /// Saved and Discarded are the two-second confirmations after it.
    enum class Result : uint8_t { Finished, Saved, Discarded };

    TrackResultScreen(Model& model, Result result);
    ~TrackResultScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override;

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);

    Result      mResult;
    lv_obj_t*   mTotal = nullptr;
    lv_obj_t*   mHint  = nullptr;
    bool        mCanUndo = false;
    uint32_t    mAutoSaveTicks = 0;
    lv_timer_t* mDismiss = nullptr;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // TRACK_RESULT_SCREEN_HPP
