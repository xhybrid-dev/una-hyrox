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
    void onRunStats(const RunVo2::RunStatsResult& stats) override;  // HybridX Run: the detail faces
    void onBests() override;                                         // HybridX Run: the PB tags

protected:
    void build() override;

private:
    // HybridX Run: (X Run's FACE_VO2 is not in X Training) FACE_FORM (cadence and
    // step length).
    // Then the run detail: FACE_ZONES (time in heart-rate zones), FACE_SPLITS (best splits and the
    // halves) and FACE_PACING (grade-adjusted pace and drift).
    // X Training (TP8): FACE_REVIEW, how the session kept to its paces, only after a planned session
    // that was judged (hasReview()); it is skipped, and its dot left out, otherwise.
    enum Face : uint8_t { FACE_MAP = 0, FACE_OVERVIEW, FACE_HEARTRATE, FACE_FORM, FACE_ZONES,
                          FACE_SPLITS, FACE_PACING, FACE_REVIEW, FACE_LAPS, FACE_COUNT };

    static constexpr uint8_t kLapPageSize = 3;   ///< rows advanced per L1/L2 press
    static constexpr uint8_t kLapVisible  = 5;   ///< rows on screen at once
    static constexpr uint8_t kLapColumns  = 5;   ///< index, distance, units, time, pace

    // HybridX Run (as Trail's run screen, its NOTES T3.2): only the face on
    // display exists. RunLVGL built all four up front, which put the summary
    // at 82% of LVGL's pool; a fifth face did not fit.
    void buildFace(uint8_t face);
    void destroyFace();
    void fillFace();
    void fillDetail();   ///< HybridX Run: the zones, splits and pacing faces
    void buildFaceMap();
    void buildFaceOverview();
    void buildFaceHeartRate();
    void buildFaceForm();
    void buildFaceZones();
    void buildFaceSplits();
    void buildFacePacing();
    void buildFaceReview();
    void fillReview();
    bool hasReview() const;                 ///< TP8: a planned session that was judged, after a save
    uint8_t faceCount() const;              ///< the faces before the lap pages that are shown
    uint8_t slotOf(uint8_t face) const;     ///< a face's dot, with an unshown review face left out
    uint8_t nextFace(uint8_t face) const;   ///< the face after @p face, skipping the review face when unshown
    uint8_t prevFace(uint8_t face) const;
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

    // Form (HybridX Run)
    lv_obj_t* mFormCadence = nullptr;
    lv_obj_t* mFormStep    = nullptr;

    // Run detail (HybridX Run)
    static constexpr uint8_t kZoneRows = 5;
    lv_obj_t* mZoneBar[kZoneRows]  = {};
    lv_obj_t* mZoneTime[kZoneRows] = {};
    lv_obj_t* mZoneBelow           = nullptr;
    lv_obj_t* mBestTime[3]         = {};
    lv_obj_t* mBestTag[3]          = {};   ///< HybridX Run: "PB" when this run set the personal best
    lv_obj_t* mHalfPace[2]         = {};
    lv_obj_t* mGapPace             = nullptr;
    lv_obj_t* mDrift               = nullptr;
    lv_obj_t* mDriftNote           = nullptr;

    // How it went (X Training, TP8)
    lv_obj_t* mRevPct   = nullptr;
    lv_obj_t* mRevReps  = nullptr;
    lv_obj_t* mRevMiss  = nullptr;
    lv_obj_t* mRevFade  = nullptr;
    lv_obj_t* mRevFeel  = nullptr;

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
