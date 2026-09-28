/**
 ******************************************************************************
 * @file    TrackScreen.cpp
 * @brief   The live activity screen (see TrackScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Format.hpp"
#include "gui/Strings.hpp"
#include "gui/RouteFormat.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/Utils/ClockTime.hpp"

using namespace SDK::GUI;
using F = Theme::Font;

namespace
{

// Status face clock geometry (TrackFaceStatus.hpp).
constexpr int32_t kTimeY       = 63;
constexpr int32_t kMeridiemY   = 105;
constexpr int32_t kMeridiemGap = 5;

// HybridX Trail: map zooms, metres from the runner to the screen edge
// (MapView::kZoomRadiiM[1] and [3]).
constexpr uint16_t kNearRadiusM = 250;
constexpr uint16_t kFarRadiusM  = 1000;
constexpr uint32_t kBackOnMs    = 4000;
constexpr int32_t  kToGoY       = 30;   ///< navigation face: the big "to go" row
constexpr int32_t  kUnitGap     = 6;
constexpr int32_t  kBannerX     = 20;   ///< the banner: wide, and low enough for the round screen
constexpr int32_t  kBannerY     = 46;
constexpr int32_t  kBannerW     = 200;
constexpr uint32_t kFinishedMs  = 8000;

/// A label that stays readable over the map: black behind it.
void onBlack(lv_obj_t* label)
{
    lv_obj_set_style_bg_color(label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(label, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(label, 6, LV_PART_MAIN);
}

// Interval phase accents (TrackFaceIntervals).
constexpr uint32_t kIvNeutral = Color::WHITE;
constexpr uint32_t kIvRun     = Color::CYAN;
constexpr uint32_t kIvRest    = Color::YELLOW_DARK;

const char* phaseTitle(Track::IntervalsPhase phase)
{
    switch (phase) {
        case Track::IntervalsPhase::RUN:       return "RUN";
        case Track::IntervalsPhase::REST:      return "REST";
        case Track::IntervalsPhase::COOL_DOWN: return "COOL DOWN";
        default:                               return "WARM UP";
    }
}

void setHidden(lv_obj_t* obj, bool hidden)
{
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}
} // namespace

TrackScreen::TrackScreen(Model& model)
    : Screen(model)
{
}

void TrackScreen::build()
{
    mIndicator = std::make_unique<Widgets::ScrollIndicator>(mRoot, Widgets::ScrollIndicator::kSmall);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::NONE, Widgets::Buttons::AMBER);

    buildBanner();
    // The face itself is built by showFace(), in onShow().
}

TrackScreen::~TrackScreen()
{
    if (mBannerTimer) {
        lv_timer_delete(mBannerTimer);
        mBannerTimer = nullptr;
    }
}

TrackScreen::Kind TrackScreen::kindOf(uint16_t id)
{
    switch (id) {
        case FaceId::ID_INTERVALS: return Kind::Intervals;
        case FaceId::ID_MAP_NEAR:
        case FaceId::ID_MAP_FAR:   return Kind::Map;
        case FaceId::ID_NAV:       return Kind::Nav;
        case FaceId::ID_TRACK1:    return Kind::Total;
        case FaceId::ID_TRACK2:    return Kind::Lap;
        case FaceId::ID_TRACK3:    return Kind::Status;
        default:                   return Kind::Total;
    }
}

void TrackScreen::dropFace()
{
    if (!mFace) {
        return;
    }
    // The widgets first, while their objects still exist (as ~Screen does).
    mIntervalsTitle.reset();
    mIntervalsTimer.reset();
    mHrZone.reset();
    mBattery.reset();
    mSensorRow.reset();
    mMap.reset();
    lv_obj_delete(mFace);
    mFace = nullptr;
    mKind = Kind::None;

    mIvRepeats = mIvRunIcon = mIvPaceIcon = mIvHeartIcon = mIvPace = mIvHr = nullptr;
    mPaceValue = mDistanceValue = mDistanceUnits = mTimerValue = nullptr;
    mHrValue = mLapPaceValue = mLapDistValue = mLapTimerValue = nullptr;
    mDayTime = mMeridiem = mPercent = nullptr;
    mMapScale = mMapToGo = nullptr;
    mNavToGo = mNavToGoUnit = mNavDone = mNavTotal = mNavStatus = mNavFoot = nullptr;
}

void TrackScreen::buildFace(Kind kind)
{
    switch (kind) {
        case Kind::Intervals: buildFaceIntervals(); break;
        case Kind::Total:     buildFaceTotal(); break;
        case Kind::Lap:       buildFaceLap(); break;
        case Kind::Status:    buildFaceStatus(); break;
        case Kind::Map:       buildFaceMap(); break;
        case Kind::Nav:       buildFaceNav(); break;
        case Kind::None:      return;
    }
    mKind = kind;
    // Behind the scroll indicator, the button hints and the banner.
    lv_obj_move_background(mFace);
    ScreenManager::logPool("face");
}

void TrackScreen::fillFace()
{
    onTrackData(mModel.getTrackData());
    if (mKind == Kind::Status) {
        uint8_t h = 0, m = 0, s = 0;
        mModel.getTime(h, m, s);
        setTime(h, m);
        onBatteryLevel(mModel.getBatteryLevel());
        onGpsFix(mModel.hasGpsFix());
        updateHrIcon();
    }
    onNav(mModel.nav());
}

void TrackScreen::buildFaceMap()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    mMap     = std::make_unique<Widgets::RouteMap>(f, 0, 0, 240, 240);
    mMap->setRoute(mModel.routePoints(), mModel.routePointCount());
    mMapScale = Theme::label(f, F::Regular16, "", 85, 14, 70, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    onBlack(mMapScale);
    mMapToGo = Theme::label(f, F::Medium18, "", 45, 204, 150);
    onBlack(mMapToGo);
}

void TrackScreen::buildFaceNav()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    Theme::label(f, F::Italic18, "To go", 0, 10, 240);
    // SemiBold 40 is digits only: the unit is its own label, the pair centred in updateNav().
    mNavToGo     = Theme::label(f, F::SemiBold40, Strings::kNoValue, 0, kToGoY, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT);
    mNavToGoUnit = Theme::label(f, F::Regular18, "", 0, kToGoY + 22, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT);
    Theme::hline(f, 35, 79, 170);
    Theme::label(f, F::Italic18, "Done", 20, 88, 88);
    mNavDone = Theme::label(f, F::SemiBold30, Strings::kNoValue, 9, 111, 118);
    Theme::vline(f, 119, 79, 80);
    Theme::label(f, F::Italic18, "Route", 132, 88, 88);
    mNavTotal = Theme::label(f, F::SemiBold30, Strings::kNoValue, 114, 111, 118);
    Theme::hline(f, 35, 159, 170);
    mNavStatus = Theme::label(f, F::SemiBold25, "", 20, 166, 200);
    mNavFoot   = Theme::label(f, F::Italic18, "", 40, 202, 160, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void TrackScreen::buildBanner()
{
    mBanner = Theme::container(mRoot, kBannerX, kBannerY, kBannerW, 34);
    lv_obj_set_style_bg_opa(mBanner, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(mBanner, Theme::rgb(Color::RED), LV_PART_MAIN);
    lv_obj_set_style_radius(mBanner, 10, LV_PART_MAIN);
    mBannerText = Theme::label(mBanner, F::SemiBold20, "", 0, 5, kBannerW);
    lv_obj_add_flag(mBanner, LV_OBJ_FLAG_HIDDEN);
}

void TrackScreen::buildFaceIntervals()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    mIvRepeats     = Theme::label(f, F::Italic18, "", 80, 202, 80);
    mIvRunIcon     = Theme::image(f, &img_runningman_46x46, 97, 167);
    mIvHr          = Theme::label(f, F::SemiBold35, Strings::kNoValue, 40, 162, 160);
    mIvPace        = Theme::label(f, F::SemiBold35, Strings::kNoValue, 40, 162, 160);
    mIvHeartIcon   = Theme::image(f, &img_heart_30x30, 105, 137);
    mIvPaceIcon    = Theme::image(f, &img_pace_30x30, 105, 137);
    mIntervalsTimer = std::make_unique<Widgets::IntervalsTimer>(f, 25, 41);
    mIntervalsTitle = std::make_unique<Widgets::Title>(f, "WARM UP");
}

void TrackScreen::buildFaceTotal()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    Theme::label(f, F::Italic18, "Pace", 0, 10, 240);
    // Value boxes are wider than the design's so a slow pace ("22:00") is not clipped.
    mPaceValue = Theme::label(f, F::SemiBold40, Strings::kNoValue, 0, 30, 240);
    Theme::hline(f, 35, 79, 170);
    Theme::label(f, F::Italic18, "Distance", 0, 88, 240);
    mDistanceValue = Theme::label(f, F::SemiBold35, Strings::kNoValue, 53, 111, 134);
    mDistanceUnits = Theme::label(f, F::Regular18, Strings::kKm, 178, 129, 45, LV_TEXT_ALIGN_LEFT);
    Theme::hline(f, 35, 159, 170);
    mTimerValue = Theme::label(f, F::SemiBold35, "0:00:00", 35, 162, 170);
    Theme::label(f, F::Italic18, "Timer", 0, 204, 240);
}

void TrackScreen::buildFaceLap()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    mHrZone  = std::make_unique<Widgets::HeartRateZone>(f, 15, 4);
    mHrValue = Theme::label(f, F::SemiBold40, Strings::kNoValue, 75, 29, 90);
    Theme::label(f, F::Regular18, "HR", 160, 50, 45, LV_TEXT_ALIGN_LEFT);
    Theme::hline(f, 35, 79, 170);
    Theme::label(f, F::Italic18, "Lap Pace", 20, 88, 88);
    // Same centres as the design (68 and 173) but wide enough for "22:00".
    mLapPaceValue = Theme::label(f, F::SemiBold35, Strings::kNoValue, 9, 111, 118);
    Theme::vline(f, 119, 79, 80);
    Theme::label(f, F::Italic18, "Lap Dist.", 132, 88, 88);
    mLapDistValue = Theme::label(f, F::SemiBold35, Strings::kNoValue, 114, 111, 118);
    Theme::hline(f, 35, 159, 170);
    mLapTimerValue = Theme::label(f, F::SemiBold35, "0:00:00", 35, 162, 170);
    Theme::label(f, F::Italic18, "Lap Time", 75, 204, 91);
}

void TrackScreen::buildFaceStatus()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(f, 0, 20, 240, 24);
    Theme::hline(f, 35, 62, 170);
    // Digits and AM/PM are sized to their text and centred as a group in setTime().
    mDayTime = Theme::label(f, F::SemiBold60, "--:--", 0, kTimeY, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT);
    mMeridiem = Theme::label(f, F::Medium18, "AM", 0, kMeridiemY, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT);
    lv_obj_add_flag(mMeridiem, LV_OBJ_FLAG_HIDDEN);
    Theme::hline(f, 35, 136, 170);
    mBattery = std::make_unique<Widgets::Battery>(f, 76, 153);
    mPercent = Theme::label(f, F::Medium25, "0%", 42, 187, 157);
}

uint16_t TrackScreen::firstFace() const
{
    return mFaceCount > 0 ? mFaces[0] : static_cast<uint16_t>(FaceId::ID_TRACK1);
}

uint8_t TrackScreen::faceIndex(uint16_t id) const
{
    for (uint8_t i = 0; i < mFaceCount; ++i) {
        if (mFaces[i] == id) {
            return i;
        }
    }
    return 0;
}

void TrackScreen::onShow()
{
    mModel.menu().track.action.reset();

    const Track::Data& data = mModel.getTrackData();
    mIntervalsMode    = data.intervalsMode;
    mHasRoute         = mModel.hasRoute();
    mIsImperial       = mModel.isUnitsImperial();
    mIs12Hour         = mModel.is12HourFormat();
    mHrThresholdCount = mModel.getHrThresholdsCount() < App::Config::kHrThresholdsCount
                            ? mModel.getHrThresholdsCount()
                            : static_cast<uint8_t>(App::Config::kHrThresholdsCount);
    memcpy(mHrThresholds, mModel.getHrThresholds(), mHrThresholdCount);
    mAccessoryState = mModel.getAccessoryState();
    mHrSource       = data.hrSource;

    // This run's faces, in order: intervals (an intervals workout only), then
    // HybridX Trail's two maps and navigation (with a route), then RunLVGL's.
    mFaceCount = 0;
    if (mIntervalsMode) {
        mFaces[mFaceCount++] = FaceId::ID_INTERVALS;
    }
    if (mHasRoute) {
        mFaces[mFaceCount++] = FaceId::ID_MAP_NEAR;
        mFaces[mFaceCount++] = FaceId::ID_MAP_FAR;
        mFaces[mFaceCount++] = FaceId::ID_NAV;
    }
    mFaces[mFaceCount++] = FaceId::ID_TRACK1;
    mFaces[mFaceCount++] = FaceId::ID_TRACK2;
    mFaces[mFaceCount++] = FaceId::ID_TRACK3;
    mIndicator->setCount(mFaceCount);

    // Coming back from a cool-down alert, show the intervals face regardless of
    // where the user had scrolled, so the cool-down phase is visible. Otherwise
    // the face last shown, if this run has it.
    const bool forceIntervals = mIntervalsMode &&
        mModel.getPendingAlertIntervals().phase == Track::IntervalsPhase::COOL_DOWN;
    uint16_t face = forceIntervals ? static_cast<uint16_t>(FaceId::ID_INTERVALS) : mModel.menu().track.get();
    if (mFaces[faceIndex(face)] != face) {
        face = firstFace();
    }
    showFace(face);
}

void TrackScreen::onHide()
{
    mModel.menu().track.set(mFaceId);
}

void TrackScreen::showFace(uint16_t id)
{
    mFaceId = id;
    const Kind kind = kindOf(id);
    const bool fresh = kind != mKind;
    if (fresh) {
        dropFace();
        buildFace(kind);
    }
    mIndicator->setActive(faceIndex(id));
    if (kind == Kind::Map) {
        lv_label_set_text(mMapScale, id == FaceId::ID_MAP_NEAR ? "250 m" : "1 km");
    }
    if (fresh) {
        fillFace();
    } else if (kind == Kind::Map) {
        updateMap(mModel.nav());   // the other zoom
    }
}

void TrackScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    const uint8_t at = faceIndex(mFaceId);
    switch (code) {
        case Btn::L1:
            showFace(mFaces[at == 0 ? mFaceCount - 1 : at - 1]);
            break;
        case Btn::L2:
            showFace(mFaces[at + 1 >= mFaceCount ? 0 : at + 1]);
            break;
        case Btn::R1:
            ScreenManager::instance().goTo(ScreenId::TrackAction);
            break;
        case Btn::R2:
            // In an intervals workout the lap button advances the phase, on any
            // face; laps are phase-driven. A free run records a manual lap.
            if (mIntervalsMode) {
                mModel.intervalsNextPhase();
            } else {
                mModel.saveLap();
                ScreenManager::instance().goTo(ScreenId::TrackLap);
            }
            break;
        default:
            break;
    }
}

// -- HybridX Trail: navigation ----------------------------------------------------

void TrackScreen::onNav(const Trail::Navigator::Status& s)
{
    if (!mHasRoute) {
        return;
    }
    if (mKind == Kind::Map) {
        updateMap(s);
    } else if (mKind == Kind::Nav) {
        updateNav(s);
    }

    // The off-course banner stays as long as the runner is off; the timed
    // ones (back on, finished) are left to their timer.
    const bool off = s.alert == Trail::OffCourse::State::Off;
    if (off) {
        char text[32];
        std::snprintf(text, sizeof(text), "OFF COURSE %lu m", static_cast<unsigned long>(s.pos.offRouteM + 0.5f));
        showBanner(text, Color::RED, 0);
        mOffBanner = true;
    } else if (mOffBanner) {
        mOffBanner = false;
        if (!mBannerTimer) {
            Theme::setHidden(mBanner, true);
        }
    }
}

void TrackScreen::updateMap(const Trail::Navigator::Status& s)
{
    if (!mHasRoute || mKind != Kind::Map) {
        return;
    }
    const uint16_t radius = mFaceId == FaceId::ID_MAP_FAR ? kFarRadiusM : kNearRadiusM;
    // With no fix yet, centre on the start so the route is still there to see.
    const Trail::GeoPoint centre = s.hasFix ? s.fix : mModel.routePoints()[0];
    mMap->follow(centre, radius, s.hasFix && s.headingValid, s.headingDeg);

    char buf[32];
    if (s.pos.everLocked) {
        char d[16];
        RouteFmt::distance(d, sizeof(d), s.pos.remainingM, mIsImperial);
        std::snprintf(buf, sizeof(buf), "%s to go", d);
    } else if (s.hasFix) {
        char d[16];
        RouteFmt::distance(d, sizeof(d), s.toStartM, mIsImperial);
        std::snprintf(buf, sizeof(buf), "start %s", d);
    } else {
        std::snprintf(buf, sizeof(buf), "no GPS");
    }
    lv_label_set_text(mMapToGo, buf);
}

void TrackScreen::updateNav(const Trail::Navigator::Status& s)
{
    if (mKind != Kind::Nav) {
        return;
    }
    char        buf[32];
    const char* unit      = "";
    const float remaining = s.pos.everLocked ? s.pos.remainingM : static_cast<float>(mModel.route().lengthM);
    RouteFmt::distanceParts(buf, sizeof(buf), unit, remaining, mIsImperial);
    lv_label_set_text(mNavToGo, buf);
    lv_label_set_text(mNavToGoUnit, unit);
    lv_obj_update_layout(mNavToGo);
    lv_obj_update_layout(mNavToGoUnit);
    const int32_t numW = lv_obj_get_width(mNavToGo);
    const int32_t left = (240 - (numW + kUnitGap + lv_obj_get_width(mNavToGoUnit))) / 2;
    lv_obj_set_pos(mNavToGo, left, kToGoY);
    lv_obj_set_pos(mNavToGoUnit, left + numW + kUnitGap, kToGoY + 22);
    Fmt::fixed(buf, sizeof(buf), Fmt::distUnits(s.pos.alongM, mIsImperial), 2);
    lv_label_set_text(mNavDone, buf);
    Fmt::fixed(buf, sizeof(buf), Fmt::distUnits(static_cast<float>(mModel.route().lengthM), mIsImperial), 2);
    lv_label_set_text(mNavTotal, buf);

    const char* status = "";
    uint32_t    colour = Color::WHITE;
    switch (s.alert) {
        case Trail::OffCourse::State::OnCourse: status = "On course";  colour = Color::LIME; break;
        case Trail::OffCourse::State::Off:      status = "Off course"; colour = Color::RED; break;
        case Trail::OffCourse::State::Finished: status = "Finished";   colour = Color::LIME; break;
        case Trail::OffCourse::State::NotStarted:
            status = s.pos.everLocked ? "On the route" : (s.hasFix ? "To the start" : "No GPS");
            colour = Color::GRAY;
            break;
    }
    lv_label_set_text(mNavStatus, status);
    lv_obj_set_style_text_color(mNavStatus, Theme::rgb(colour), LV_PART_MAIN);

    if (s.hasFix && !s.pos.everLocked) {
        char d[16];
        RouteFmt::distance(d, sizeof(d), s.toStartM, mIsImperial);
        std::snprintf(buf, sizeof(buf), "start %s away", d);
    } else if (s.hasFix) {
        std::snprintf(buf, sizeof(buf), "%lu m from line", static_cast<unsigned long>(s.pos.offRouteM + 0.5f));
    } else {
        buf[0] = '\0';
    }
    lv_label_set_text(mNavFoot, buf);
}

void TrackScreen::onNavAlert(Trail::OffCourse::Event e)
{
    if (!mHasRoute) {
        return;
    }
    switch (e) {
        case Trail::OffCourse::Event::WentOff:
        case Trail::OffCourse::Event::StillOff:
            // The map is what gets a runner back: show it, near.
            showFace(FaceId::ID_MAP_NEAR);
            break;
        case Trail::OffCourse::Event::BackOn:
            mOffBanner = false;
            showBanner("BACK ON COURSE", Color::GREEN, kBackOnMs);
            break;
        case Trail::OffCourse::Event::Finished:
            mOffBanner = false;
            showBanner("ROUTE COMPLETE", Color::GREEN, kFinishedMs);
            break;
        case Trail::OffCourse::Event::None:
            break;
    }
    onNav(mModel.nav());
}

void TrackScreen::showBanner(const char* text, uint32_t colour, uint32_t forMs)
{
    lv_label_set_text(mBannerText, text);
    lv_obj_set_style_bg_color(mBanner, Theme::rgb(colour), LV_PART_MAIN);
    Theme::setHidden(mBanner, false);
    lv_obj_move_foreground(mBanner);
    if (mBannerTimer) {
        lv_timer_delete(mBannerTimer);
        mBannerTimer = nullptr;
    }
    if (forMs > 0) {
        mBannerTimer = lv_timer_create(&TrackScreen::bannerTimerCb, forMs, this);
        lv_timer_set_repeat_count(mBannerTimer, 1);
    }
}

void TrackScreen::bannerTimerCb(lv_timer_t* t)
{
    auto* self         = static_cast<TrackScreen*>(lv_timer_get_user_data(t));
    self->mBannerTimer = nullptr;   // a one-shot timer deletes itself
    if (!self->mOffBanner) {
        Theme::setHidden(self->mBanner, true);
    }
}

void TrackScreen::onTrackData(const Track::Data& data)
{
    char buf[16];

    switch (mKind) {
        case Kind::Total:
            Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(data.pace, mIsImperial));
            lv_label_set_text(mPaceValue, buf);
            Fmt::distanceTotal(buf, sizeof(buf), Fmt::distUnits(data.distance, mIsImperial));
            lv_label_set_text(mDistanceValue, buf);
            lv_label_set_text(mDistanceUnits, Fmt::units(mIsImperial));
            Fmt::hms(buf, sizeof(buf), data.totalTime);
            lv_label_set_text(mTimerValue, buf);
            break;

        case Kind::Lap:
            Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(data.lapPace, mIsImperial));
            lv_label_set_text(mLapPaceValue, buf);
            Fmt::distanceLap(buf, sizeof(buf), Fmt::distUnits(data.lapDistance, mIsImperial));
            lv_label_set_text(mLapDistValue, buf);
            Fmt::hms(buf, sizeof(buf), data.lapTime);
            lv_label_set_text(mLapTimerValue, buf);
            Fmt::heartRate(buf, sizeof(buf), data.hr);
            lv_label_set_text(mHrValue, buf);
            mHrZone->setHR(data.hr < App::Display::kMinHR ? 0.0f : data.hr, mHrThresholds, mHrThresholdCount);
            break;

        case Kind::Intervals: {
            const Track::IntervalsData& iv = data.intervals;
            setIntervalsPhase(iv);
            if (iv.metric == Track::IntervalsMetric::DISTANCE) {
                mIntervalsTimer->setPhaseDistance(Fmt::distUnits(iv.distRemaining, mIsImperial), mIsImperial);
            } else {
                mIntervalsTimer->setPhaseTime(iv.phaseTimerSec, iv.metric);
            }
            Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(data.pace, mIsImperial));
            lv_label_set_text(mIvPace, buf);
            Fmt::heartRate(buf, sizeof(buf), data.hr);
            lv_label_set_text(mIvHr, buf);
        } break;

        default:
            break;
    }

    mHrSource = data.hrSource;
    updateHrIcon();
}

void TrackScreen::setIntervalsPhase(const Track::IntervalsData& iv)
{
    const bool run  = iv.phase == Track::IntervalsPhase::RUN;
    const bool rest = iv.phase == Track::IntervalsPhase::REST;

    mIntervalsTitle->setText(phaseTitle(iv.phase));
    mIntervalsTimer->setColor(run ? kIvRun : rest ? kIvRest : kIvNeutral);
    mIntervalsTimer->setLineVisible(run || rest);

    // Bottom row: pace while running, heart rate while resting, the runner otherwise.
    setHidden(mIvRunIcon,   run || rest);
    setHidden(mIvPaceIcon,  !run);
    setHidden(mIvPace,      !run);
    setHidden(mIvHeartIcon, !rest);
    setHidden(mIvHr,        !rest);

    // Repeat counter only during RUN / REST; "n" alone for open-ended repeats.
    setHidden(mIvRepeats, !(run || rest));
    if (iv.totalRepeats == 0) {
        lv_label_set_text_fmt(mIvRepeats, "%u", static_cast<unsigned>(iv.repeat));
    } else {
        lv_label_set_text_fmt(mIvRepeats, "%u/%u", static_cast<unsigned>(iv.repeat),
                              static_cast<unsigned>(iv.totalRepeats));
    }
}

void TrackScreen::onBatteryLevel(uint8_t level)
{
    if (mKind != Kind::Status) {
        return;
    }
    mBattery->setLevel(level);
    lv_label_set_text_fmt(mPercent, "%u%%", level);
}

void TrackScreen::onTime(uint8_t hour, uint8_t minute, uint8_t /*sec*/)
{
    setTime(hour, minute);
}

void TrackScreen::setTime(uint8_t h, uint8_t m)
{
    if (mKind != Kind::Status) {
        return;
    }
    const SDK::Clock::Hour12 civil = SDK::Clock::to12Hour(h);
    lv_label_set_text_fmt(mDayTime, "%u:%02u", mIs12Hour ? civil.hour : h, m);

    // Centre digits (+ suffix) as one group on the 240 px face, as the Run app does.
    lv_obj_update_layout(mDayTime);
    const int32_t timeW = lv_obj_get_width(mDayTime);
    int32_t groupW = timeW;
    int32_t merW   = 0;
    if (mIs12Hour) {
        lv_label_set_text(mMeridiem, civil.pm ? "PM" : "AM");
        lv_obj_remove_flag(mMeridiem, LV_OBJ_FLAG_HIDDEN);
        lv_obj_update_layout(mMeridiem);
        merW   = lv_obj_get_width(mMeridiem);
        groupW = timeW + kMeridiemGap + merW;
    } else {
        lv_obj_add_flag(mMeridiem, LV_OBJ_FLAG_HIDDEN);
    }
    const int32_t left = (240 - groupW) / 2;
    lv_obj_set_pos(mDayTime, left, kTimeY);
    if (mIs12Hour) {
        lv_obj_set_pos(mMeridiem, left + timeW + kMeridiemGap, kMeridiemY);
    }
}

void TrackScreen::onLapChanged(uint8_t /*lapEnd*/)
{
    ScreenManager::instance().goTo(ScreenId::TrackLap);
}

void TrackScreen::onIntervalsPhaseAlert()
{
    ScreenManager::instance().goTo(ScreenId::TrackIntervalsAlert);
}

void TrackScreen::onIntervalsWorkoutCompleted()
{
    ScreenManager::instance().goTo(ScreenId::TrackIntervalsCompleted);
}

void TrackScreen::onGpsFix(bool acquired)
{
    if (!mSensorRow) {
        return;
    }
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void TrackScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mAccessoryState = state;
    updateHrIcon();
}

void TrackScreen::updateHrIcon()
{
    if (!mSensorRow) {
        return;
    }
    mSensorRow->setHr(Widgets::SensorStatusRow::hrStateFromSource(mAccessoryState, mHrSource));
}
