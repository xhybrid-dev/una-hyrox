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
#include "gui/MapZoom.hpp"

#include <cmath>
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

// HybridX Trail
constexpr uint32_t kBackOnMs    = 4000;
constexpr uint32_t kTurnMs      = 4000;
constexpr int32_t  kToGoY       = 30;   ///< navigation face: the big "to go" row
constexpr int32_t  kUnitGap     = 6;
constexpr int32_t  kBannerX     = 14;   ///< the banner: a band across the middle, as in the promo
constexpr int32_t  kBannerY     = 84;
constexpr int32_t  kBannerW     = 212;
constexpr int32_t  kBannerH     = 68;
constexpr int16_t  kWholeMarginPx = 34;   ///< the whole-route map, clear of the edge and its markers
constexpr uint32_t kAlertYellow = 0xFFE000;   ///< off course
constexpr uint32_t kAlertGreen  = 0x38D060;   ///< back on course, route complete
constexpr uint32_t kAlertInk    = 0x2A1E00;   ///< text on the yellow
constexpr uint32_t kRouteMagenta = 0xE040FF;
constexpr uint32_t kFinishedMs  = 8000;

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
        case FaceId::ID_MAP:       return Kind::Map;
        case FaceId::ID_NAV:       return Kind::Nav;
        case FaceId::ID_PROFILE:   return Kind::Profile;
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
    mRunTitle.reset();
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
    mPaceValue = mPaceUnit = mDistanceValue = mDistanceUnits = mTimerValue = mTotalHr = mTotalLap = nullptr;
    mHrValue = mLapPaceValue = mLapDistValue = mLapTimerValue = nullptr;
    mDayTime = mMeridiem = mPercent = nullptr;
    mMapToGo = mMapTurn = nullptr;
    mChart.reset();
    mProfMax = mProfAscent = mProfNext = mProfNow = nullptr;
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
        case Kind::Profile:   buildFaceProfile(); break;
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
    mMap->showFurniture(true, true, mModel.isUnitsImperial());
    mMapToGo = Theme::label(f, F::SemiBold20, "", 20, 42, 200);
    mMapTurn = Theme::label(f, F::Medium18, "", 20, 68, 200, LV_TEXT_ALIGN_CENTER, kRouteMagenta);
}

void TrackScreen::buildFaceProfile()
{
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    mRunTitle  = std::make_unique<Widgets::Title>(f, "Elevation");
    mChart     = std::make_unique<Widgets::ElevationChart>(f, 24, 48, 192, 96);
    mProfMax   = Theme::label(f, F::Regular14, "", 30, 46, 90, LV_TEXT_ALIGN_LEFT, Color::GRAY);
    mProfAscent = Theme::label(f, F::SemiBold20, "", 20, 148, 200);
    mProfNext   = Theme::label(f, F::Medium18, "", 20, 176, 200, LV_TEXT_ALIGN_CENTER, kRouteMagenta);
    mProfNow    = Theme::label(f, F::Regular16, "", 20, 204, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
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
    mBanner = Theme::container(mRoot, kBannerX, kBannerY, kBannerW, kBannerH);
    lv_obj_set_style_bg_opa(mBanner, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(mBanner, Theme::rgb(kAlertYellow), LV_PART_MAIN);
    lv_obj_set_style_radius(mBanner, 12, LV_PART_MAIN);
    mBannerText = Theme::label(mBanner, F::SemiBold25, "", 0, 4, kBannerW, LV_TEXT_ALIGN_CENTER, kAlertInk);
    mBannerSub  = Theme::label(mBanner, F::Medium18, "", 0, 36, kBannerW, LV_TEXT_ALIGN_CENTER, kAlertInk);
    mBannerArrow = lv_line_create(mBanner);
    lv_obj_set_pos(mBannerArrow, 0, 0);
    lv_obj_set_style_line_width(mBannerArrow, 4, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(mBannerArrow, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(mBannerArrow, Theme::rgb(kAlertInk), LV_PART_MAIN);
    lv_obj_add_flag(mBannerArrow, LV_OBJ_FLAG_HIDDEN);
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
    // The promo's "Run" face: distance big, pace and time, heart rate, lap.
    lv_obj_t* f = mFace = Theme::container(mRoot, 0, 0, 240, 240);
    mRunTitle      = std::make_unique<Widgets::Title>(f, "Run");
    mDistanceValue = Theme::label(f, F::SemiBold40, Strings::kNoValue, 0, 40, 240);
    mDistanceUnits = Theme::label(f, F::Regular18, Strings::kKm, 0, 86, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mPaceValue     = Theme::label(f, F::SemiBold25, Strings::kNoValue, 14, 116, 106);
    mPaceUnit      = Theme::label(f, F::Regular16, "/km", 14, 144, 106, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mTimerValue    = Theme::label(f, F::SemiBold25, "0:00:00", 120, 116, 108);
    Theme::label(f, F::Regular16, "time", 120, 144, 108, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mTotalHr       = Theme::label(f, F::SemiBold25, "--- bpm", 30, 172, 180);
    mTotalLap      = Theme::label(f, F::Medium18, "Lap 1", 60, 205, 120, LV_TEXT_ALIGN_CENTER, kRouteMagenta);
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
        mFaces[mFaceCount++] = FaceId::ID_MAP;
        mFaces[mFaceCount++] = FaceId::ID_NAV;
        if (mModel.profile().valid()) {
            mFaces[mFaceCount++] = FaceId::ID_PROFILE;   // a route with no elevation has no profile to show
        }
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
    if (fresh) {
        fillFace();
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
            // On the map, R2 zooms: 300 m, 750 m, 1.5 km, 3 km, the whole route.
            if (mKind == Kind::Map) {
                mModel.nextMapZoom();
                updateMap(mModel.nav());
                break;
            }
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
    } else if (mKind == Kind::Profile) {
        updateProfile(s);
    }

    // The off-course banner stays as long as the runner is off; the timed
    // ones (back on, finished) are left to their timer.
    const bool off = s.alert == Trail::OffCourse::State::Off;
    if (off) {
        char text[32];
        std::snprintf(text, sizeof(text), "%lu m from line", static_cast<unsigned long>(s.pos.offRouteM + 0.5f));
        showBanner("Off course", text, kAlertYellow, 0);
        // The way back, as the runner sees it: turned by the direction they face,
        // or by nothing on a north-up map.
        const bool northUpMap = mKind == Kind::Map && mModel.getSettings().mapNorthUp;
        const float faces     = s.headingValid && !northUpMap ? s.headingDeg : 0.0f;
        setBannerArrow(s.guideValid, s.guideBearingDeg - faces);
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
    const uint8_t zoom = mModel.mapZoom();
    if (zoom == MapZoom::kWhole) {
        mMap->fitWhole(s.hasFix ? &s.fix : nullptr, kWholeMarginPx);
    } else {
        // With no fix yet, centre on the start so the route is still there to see.
        const Trail::GeoPoint centre = s.hasFix ? s.fix : mModel.routePoints()[0];
        const bool turn = s.hasFix && s.headingValid && !mModel.getSettings().mapNorthUp;
        mMap->follow(centre, MapZoom::kRadiiM[zoom], turn, s.headingDeg);
    }

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

    // The next turn, under it: "Right 120 m". Not while lost: the way back is the news then.
    if (s.turnValid && s.alert != Trail::OffCourse::State::Off) {
        char d[16];
        RouteFmt::distance(d, sizeof(d), static_cast<float>(s.turnDistM), mIsImperial);
        std::snprintf(buf, sizeof(buf), "%s %s", Trail::TurnFinder::name(s.turnAngleDeg), d);
        lv_label_set_text(mMapTurn, buf);
    } else {
        lv_label_set_text(mMapTurn, "");
    }
}

void TrackScreen::updateProfile(const Trail::Navigator::Status& s)
{
    if (mKind != Kind::Profile) {
        return;
    }
    const Trail::ElevationProfile& p = mModel.profile();
    const float along = s.pos.everLocked ? s.pos.alongM : 0.0f;
    mChart->set(p, along, s.pos.everLocked);

    char buf[64];
    char h[16];
    RouteFmt::height(h, sizeof(h), p.maxM(), mIsImperial);
    std::snprintf(buf, sizeof(buf), "%s", h);
    lv_label_set_text(mProfMax, buf);

    RouteFmt::height(h, sizeof(h), p.ascentLeftM(along), mIsImperial);
    std::snprintf(buf, sizeof(buf), "%s to climb", h);
    lv_label_set_text(mProfAscent, buf);

    const Trail::ElevationProfile::Climb c = p.nextClimb(along);
    if (c.found) {
        char up[16];
        RouteFmt::height(up, sizeof(up), c.riseM, mIsImperial);
        if (c.startAheadM < 50.0f) {
            std::snprintf(buf, sizeof(buf), "Climbing +%s", up);
        } else {
            char d[16];
            RouteFmt::distance(d, sizeof(d), c.startAheadM, mIsImperial);
            std::snprintf(buf, sizeof(buf), "+%s in %s", up, d);
        }
    } else {
        std::snprintf(buf, sizeof(buf), "No more climbs");
    }
    lv_label_set_text(mProfNext, buf);

    RouteFmt::height(h, sizeof(h), p.elevationM(along), mIsImperial);
    std::snprintf(buf, sizeof(buf), "Now %s", h);
    lv_label_set_text(mProfNow, buf);
}

void TrackScreen::setBannerArrow(bool show, float screenDeg)
{
    // The text moves over to make room for the arrow.
    lv_obj_set_x(mBannerText, show ? 48 : 0);
    lv_obj_set_x(mBannerSub, show ? 48 : 0);
    lv_obj_set_width(mBannerText, show ? kBannerW - 54 : kBannerW);
    lv_obj_set_width(mBannerSub, show ? kBannerW - 54 : kBannerW);
    if (!show) {
        lv_obj_add_flag(mBannerArrow, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    static const float kShape[5][2] = { { 0, -18 }, { 13, 14 }, { 0, 6 }, { -13, 14 }, { 0, -18 } };
    const float r = screenDeg * 3.14159265f / 180.0f;
    const float c = std::cos(r);
    const float s = std::sin(r);
    for (int i = 0; i < 5; ++i) {
        mArrowPts[i].x = static_cast<lv_value_precise_t>(std::lround(28.0f + kShape[i][0] * c - kShape[i][1] * s));
        mArrowPts[i].y = static_cast<lv_value_precise_t>(std::lround(static_cast<float>(kBannerH) / 2.0f +
                                                                     kShape[i][0] * s + kShape[i][1] * c));
    }
    lv_line_set_points(mBannerArrow, mArrowPts, 5);
    lv_obj_remove_flag(mBannerArrow, LV_OBJ_FLAG_HIDDEN);
}

void TrackScreen::onTurnCue(int16_t angleDeg)
{
    if (!mHasRoute || mOffBanner) {
        return;
    }
    const uint16_t dist = mModel.nav().turnDistM;
    char           sub[24];
    if (dist > 0) {
        std::snprintf(sub, sizeof(sub), "in %lu m", static_cast<unsigned long>((dist + 5u) / 10u * 10u));
    } else {
        sub[0] = '\0';
    }
    const bool sharp = Trail::TurnFinder::isSharp(angleDeg);
    showBanner(sharp ? Trail::TurnFinder::name(angleDeg) : (angleDeg < 0 ? "Turn left" : "Turn right"), sub,
               kRouteMagenta, kTurnMs);
    setBannerArrow(false, 0.0f);
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
            // The map is what gets a runner back: show it.
            showFace(FaceId::ID_MAP);
            break;
        case Trail::OffCourse::Event::BackOn:
            mOffBanner = false;
            showBanner("Back on course", "", kAlertGreen, kBackOnMs);
            break;
        case Trail::OffCourse::Event::Finished:
            mOffBanner = false;
            {
                char d[16];
                RouteFmt::distance(d, sizeof(d), static_cast<float>(mModel.route().lengthM), mIsImperial);
                showBanner("Route complete", d, kAlertGreen, kFinishedMs);
            }
            break;
        case Trail::OffCourse::Event::None:
            break;
    }
    onNav(mModel.nav());
}

void TrackScreen::showBanner(const char* title, const char* sub, uint32_t colour, uint32_t forMs)
{
    // One line sits in the middle of the band; two, the title above the detail.
    const bool two = sub[0] != '\0';
    lv_label_set_text(mBannerText, title);
    lv_label_set_text(mBannerSub, sub);
    setBannerArrow(false, 0.0f);   // a caller with a direction to show puts it back
    lv_obj_set_y(mBannerText, two ? 5 : 18);
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
        case Kind::Total: {
            Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(data.pace, mIsImperial));
            lv_label_set_text(mPaceValue, buf);
            Fmt::distanceTotal(buf, sizeof(buf), Fmt::distUnits(data.distance, mIsImperial));
            lv_label_set_text(mDistanceValue, buf);
            lv_label_set_text(mDistanceUnits, Fmt::units(mIsImperial));
            Fmt::hms(buf, sizeof(buf), data.totalTime);
            lv_label_set_text(mTimerValue, buf);
            char line[24];
            std::snprintf(line, sizeof(line), "/%s", Fmt::units(mIsImperial));
            lv_label_set_text(mPaceUnit, line);
            Fmt::heartRate(buf, sizeof(buf), data.hr);
            std::snprintf(line, sizeof(line), "%s bpm", buf);
            lv_label_set_text(mTotalHr, line);
            std::snprintf(line, sizeof(line), "Lap %lu", static_cast<unsigned long>(data.lapNum + 1u));
            lv_label_set_text(mTotalLap, line);
        } break;

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
