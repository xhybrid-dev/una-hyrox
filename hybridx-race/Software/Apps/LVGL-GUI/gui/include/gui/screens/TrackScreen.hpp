/**
 ******************************************************************************
 * @file    TrackScreen.hpp
 * @brief   The race screen (brief 8.2 item 3).
 *
 * Two faces, cycled with L1/L2:
 *   Main   -- segment label and number, segment time large, total time, heart
 *             rate with its zone, and what is coming next.
 *   Status -- time of day and battery.
 *
 * R2 splits. R1 opens the action menu. The race screen has no idle timeout:
 * a race must never be ended because nobody touched the watch.
 ******************************************************************************
 */

#ifndef TRACK_SCREEN_HPP
#define TRACK_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackScreen : public Screen
{
public:
    explicit TrackScreen(Model& model);

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    void onRaceData(const Track::Data& data) override;
    void onRaceState(const Track::State& state) override;
    void onSplit(const Track::SplitEvent& split) override;
    void onRaceFinished(bool completed) override;
    void onTime(uint8_t hour, uint8_t minute, uint8_t sec) override;
    void onBatteryLevel(uint8_t level) override;

protected:
    void build() override;

private:
    enum class Face : uint8_t { Main = 0, Status, Count };

    void showFace(Face face);
    void redraw();

    Face mFace = Face::Main;

    // Main face
    lv_obj_t* mMainRoot    = nullptr;
    lv_obj_t* mSegment     = nullptr;
    lv_obj_t* mSegmentNum  = nullptr;
    lv_obj_t* mSegmentTime = nullptr;
    lv_obj_t* mTotalTime   = nullptr;
    lv_obj_t* mNextUp      = nullptr;
    lv_obj_t* mHr          = nullptr;

    // Status face
    lv_obj_t* mStatusRoot = nullptr;
    lv_obj_t* mClock      = nullptr;
    lv_obj_t* mBattery    = nullptr;

    std::unique_ptr<Widgets::Title>          mTitle;
    std::unique_ptr<Widgets::Buttons>        mButtons;
    std::unique_ptr<Widgets::HeartRateZone>  mHrZone;
};

#endif // TRACK_SCREEN_HPP
