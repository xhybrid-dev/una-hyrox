/**
 ******************************************************************************
 * @file    TrackSummaryScreen.hpp
 * @brief   Activity summary: map, overview, heart rate, VO2max (HybridX Run)
 *          and a paged lap list, browsed with L1/L2.
 *
 * Port of the Run app's TrackSummaryView and its SummaryFace* containers.
 * Reached from the action menu while paused (R2 returns there) or after a
 * save (R1 leaves the app).
 ******************************************************************************
 */

#ifndef TRACK_SUMMARY_SCREEN_HPP
#define TRACK_SUMMARY_SCREEN_HPP

#include <memory>
#include <vector>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackSummaryScreen : public Screen
{
public:
    explicit TrackSummaryScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onActivitySummary(const ActivitySummary& summary) override;
    void onVo2Info(const CustomMessage::Vo2Info& info) override;   // HybridX Run

protected:
    void build() override;

private:
    // HybridX Run: FACE_VO2 added between heart rate and the laps.
    enum Face : uint8_t { FACE_MAP = 0, FACE_OVERVIEW, FACE_HEARTRATE, FACE_VO2, FACE_LAPS, FACE_COUNT };

    static constexpr uint8_t kLapPageSize = 3;   ///< rows advanced per L1/L2 press
    static constexpr uint8_t kLapVisible  = 5;   ///< rows on screen at once
    static constexpr uint8_t kLapColumns  = 5;   ///< index, distance, units, time, pace

    // HybridX Run (as Trail's run screen, its NOTES T3.2): only the face on
    // display exists. RunLVGL built all four up front, which put the summary
    // at 82% of LVGL's pool; a fifth face did not fit.
    void buildFace(uint8_t face);
    void destroyFace();
    void fillFace();
    void buildFaceMap();
    void buildFaceOverview();
    void buildFaceHeartRate();
    void buildFaceVo2();
    void buildFaceLaps();

    void setSummary(const ActivitySummary& s);
    void showFace(uint8_t face);
    void updateIndicator();
    void fillLapRows();
    void backToTrack();

    lv_obj_t* mFace = nullptr;   ///< the one face built, or nullptr

    // Map + overview headers
    lv_obj_t* mMapDistance      = nullptr;
    lv_obj_t* mMapUnits         = nullptr;
    lv_obj_t* mOverviewDistance = nullptr;
    lv_obj_t* mOverviewUnits    = nullptr;
    lv_obj_t* mAvgPace          = nullptr;
    lv_obj_t* mTimer            = nullptr;
    std::unique_ptr<Widgets::Map> mMap;

    // Heart rate
    lv_obj_t* mMaxHr = nullptr;
    lv_obj_t* mAvgHr = nullptr;

    // VO2max (HybridX Run)
    lv_obj_t* mVo2Run         = nullptr;
    lv_obj_t* mVo2RunLabel    = nullptr;
    lv_obj_t* mVo2RecentLabel = nullptr;

    // Laps
    lv_obj_t* mLapsCount = nullptr;
    lv_obj_t* mLapsTotal = nullptr;
    lv_obj_t* mLapRows[kLapVisible][kLapColumns] = {};

    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<Widgets::ScrollIndicator> mIndicator;
    std::unique_ptr<Widgets::Title>           mTitle;

    const ActivitySummary*         mSummary = nullptr;
    const std::vector<LapSummary>* mLaps    = nullptr;

    uint8_t mFaceId     = FACE_MAP;
    bool    mIsImperial = false;
    bool    mPaused     = false;
    uint8_t mLapPages   = 0;
    uint8_t mLapPage    = 0;
};

#endif // TRACK_SUMMARY_SCREEN_HPP
