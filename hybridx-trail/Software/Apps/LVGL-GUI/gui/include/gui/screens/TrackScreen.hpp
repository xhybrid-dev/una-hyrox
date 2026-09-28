/**
 ******************************************************************************
 * @file    TrackScreen.hpp
 * @brief   The live activity screen: the totals, lap and status faces, plus the
 *          intervals face when the workout is in intervals mode. L1/L2 page
 *          faces, R1 opens the action menu, R2 marks a lap (or advances the
 *          interval phase).
 *
 * Port of the Run app's TrackView/TrackPresenter and its TrackFace* containers.
 *
 * HybridX Trail adds two map faces (near and far) and a navigation face, and a
 * banner over every face for off course, back on course and route complete.
 * Only the face on display exists: paging deletes it and builds the next.
 * RunLVGL built every face up front, but its four faces take some 24 KB of
 * LVGL's 40 KB pool (SDK lv_conf.h), which leaves no room for the map.
 ******************************************************************************
 */

#ifndef TRACK_SCREEN_HPP
#define TRACK_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/RouteMap.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackScreen : public Screen
{
public:
    explicit TrackScreen(Model& model);
    ~TrackScreen() override;

    // Screen
    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    // ModelListener
    void onTrackData(const Track::Data& data) override;
    void onBatteryLevel(uint8_t level) override;
    void onTime(uint8_t hour, uint8_t minute, uint8_t sec) override;
    void onLapChanged(uint8_t lapEnd) override;
    void onIntervalsPhaseAlert() override;
    void onIntervalsWorkoutCompleted() override;
    void onGpsFix(bool acquired) override;
    void onAccessoryStatus(uint8_t state, const char* name) override;
    // HybridX Trail
    void onNav(const Trail::Navigator::Status& s) override;
    void onNavAlert(Trail::OffCourse::Event e) override;

protected:
    void build() override;

private:
    using FaceId = App::MenuNav::TrackView::Id;

    /// What a face shows: both map faces are one kind at two zooms.
    enum class Kind : uint8_t { None, Intervals, Total, Lap, Status, Map, Nav };
    static Kind kindOf(uint16_t id);

    void dropFace();
    void buildFace(Kind kind);
    void fillFace();
    void buildFaceMap();
    void buildFaceNav();
    void buildBanner();
    void updateMap(const Trail::Navigator::Status& s);
    void updateNav(const Trail::Navigator::Status& s);
    void showBanner(const char* text, uint32_t colour, uint32_t forMs);
    static void bannerTimerCb(lv_timer_t* t);
    uint8_t faceIndex(uint16_t id) const;
    void buildFaceIntervals();
    void buildFaceTotal();
    void buildFaceLap();
    void buildFaceStatus();
    void showFace(uint16_t id);
    uint16_t firstFace() const;
    void setTime(uint8_t h, uint8_t m);
    void updateHrIcon();
    void setIntervalsPhase(const Track::IntervalsData& iv);

    // The face on display (240 x 240), behind the indicator, buttons and banner
    lv_obj_t* mFace = nullptr;
    Kind      mKind = Kind::None;

    // Intervals face
    std::unique_ptr<Widgets::Title>          mIntervalsTitle;
    std::unique_ptr<Widgets::IntervalsTimer> mIntervalsTimer;
    lv_obj_t* mIvRepeats   = nullptr;
    lv_obj_t* mIvRunIcon   = nullptr;
    lv_obj_t* mIvPaceIcon  = nullptr;
    lv_obj_t* mIvHeartIcon = nullptr;
    lv_obj_t* mIvPace      = nullptr;
    lv_obj_t* mIvHr        = nullptr;

    // Totals face
    lv_obj_t* mPaceValue     = nullptr;
    lv_obj_t* mDistanceValue = nullptr;
    lv_obj_t* mDistanceUnits = nullptr;
    lv_obj_t* mTimerValue    = nullptr;

    // Lap face
    lv_obj_t* mHrValue       = nullptr;
    lv_obj_t* mLapPaceValue  = nullptr;
    lv_obj_t* mLapDistValue  = nullptr;
    lv_obj_t* mLapTimerValue = nullptr;
    std::unique_ptr<Widgets::HeartRateZone> mHrZone;

    // Status face
    lv_obj_t* mDayTime  = nullptr;
    lv_obj_t* mMeridiem = nullptr;
    lv_obj_t* mPercent  = nullptr;
    std::unique_ptr<Widgets::Battery>         mBattery;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;

    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<Widgets::ScrollIndicator> mIndicator;

    // HybridX Trail: map faces
    std::unique_ptr<Widgets::RouteMap> mMap;
    lv_obj_t* mMapScale = nullptr;   ///< "250 m": the zoom
    lv_obj_t* mMapToGo  = nullptr;   ///< "8.25 km to go"
    // Navigation face
    lv_obj_t* mNavToGo     = nullptr;
    lv_obj_t* mNavToGoUnit = nullptr;
    lv_obj_t* mNavDone   = nullptr;
    lv_obj_t* mNavTotal  = nullptr;
    lv_obj_t* mNavStatus = nullptr;
    lv_obj_t* mNavFoot   = nullptr;
    // The banner over every face: off course (stays), back on / finished (timed)
    lv_obj_t*   mBanner      = nullptr;
    lv_obj_t*   mBannerText  = nullptr;
    lv_timer_t* mBannerTimer = nullptr;
    bool        mOffBanner   = false;
    // The faces this run has, in order (the map ones only with a route)
    uint16_t mFaces[App::MenuNav::TrackView::ID_COUNT] {};
    uint8_t  mFaceCount = 0;

    bool     mIntervalsMode = false;
    bool     mHasRoute      = false;
    uint16_t mFaceId        = FaceId::ID_TRACK1;
    bool     mIsImperial    = false;
    bool     mIs12Hour      = false;
    uint8_t  mHrThresholds[App::Config::kHrThresholdsCount] = {};
    uint8_t  mHrThresholdCount = 0;
    uint8_t  mAccessoryState   = 0;
    uint8_t  mHrSource         = 0;
};

#endif // TRACK_SCREEN_HPP
