/**
 ******************************************************************************
 * @file    LiveScreen.cpp
 * @brief   The ride on the wrist (see LiveScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/LiveScreen.hpp"

#include <cstdio>

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "RideText.hpp"

using namespace SDK::GUI;

namespace
{
/// How long the lap toast stays.
constexpr uint32_t kToastMs = 2000;

constexpr int32_t kLapTimeY   = 54;    // top of the lap time's box, in the 60 px face
constexpr int32_t kZonesY     = 48;    // the zones face: first row, and the step between rows
constexpr int32_t kZonesStep  = 28;
constexpr uint8_t kTotals     = 4;     // rows on the totals face

constexpr int32_t kRingRadius = 113;   // under the bezel arcs, which share it
constexpr int32_t kRingWidth  = 6;

/// The big number's font, and its y so the digits keep one centre line whichever font is used
/// (the 40 px face is vertically centred against the 60 px one).
void setBigNumber(lv_obj_t* label, const lv_font_t* font, const lv_font_t* reference)
{
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_y(label, kLapTimeY + (lv_font_get_line_height(reference) - lv_font_get_line_height(font)) / 2);
}

const char* speedUnit(bool imperial) { return imperial ? Strings::kMph : Strings::kKmh; }
const char* distUnit(bool imperial) { return imperial ? Strings::kMiles : Strings::kKm; }
const char* climbUnit(bool imperial) { return imperial ? Strings::kFeet : Strings::kMetres; }

uint32_t ringColor(const Ride::SessionSnapshot& s)
{
    if (s.state == static_cast<uint8_t>(Ride::RunState::Paused)) {
        return Color::GRAY;
    }
    return Theme::zoneColor(s.zone);
}

} // namespace

LiveScreen::LiveScreen(Model& model)
    : Screen(model)
{
}

LiveScreen::~LiveScreen()
{
    if (mToastTimer) {
        lv_timer_delete(mToastTimer);
    }
}

void LiveScreen::build()
{
    // The ring is first, so the faces sit over it and the bezel hints (created
    // last, and kept above everything) over them.
    mRing = Theme::arc(mRoot, Widgets::kCx, Widgets::kCy, kRingRadius, kRingWidth, 0, 360, Color::GRAY_DARK);

    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::AMBER);

    uint8_t face = mModel.liveFace() < FaceCount ? mModel.liveFace() : static_cast<uint8_t>(FaceMain);
    if (face == FaceLap && mModel.snapshot().outdoor == 0) {
        face = FaceMain;   // the Lap face is outdoor only
    }
    showFace(face);
}

uint8_t LiveScreen::stepFace(int dir) const
{
    uint8_t face = mFace;
    do {
        face = static_cast<uint8_t>((face + FaceCount + dir) % FaceCount);
    } while (face == FaceLap && mModel.snapshot().outdoor == 0);
    return face;
}

void LiveScreen::onShow()
{
    redraw();
}

void LiveScreen::onHide()
{
    mModel.setLiveFace(mFace);
}

void LiveScreen::showFace(uint8_t face)
{
    destroyFace();
    mFace = face;
    switch (face) {
        case FaceMain:   buildMain(); break;
        case FaceLap:    buildMain(); break;   // the same five lines
        case FaceTotals: buildTotals(); break;
        case FaceZones:  buildZones(); break;
        default:         buildClock(); break;
    }
    // Just above the ring, so the bezel hints stay on top.
    lv_obj_move_to_index(mFaceRoot, 1);
}

void LiveScreen::destroyFace()
{
    mFaceTitle.reset();
    mBattery.reset();
    if (mFaceRoot) {
        lv_obj_delete(mFaceRoot);
        mFaceRoot = nullptr;
    }
    mLapNo = mLapTime = mInfo = mHr = mZone = nullptr;
    mClockLap = mClock = mBatteryPct = nullptr;
    for (uint8_t i = 0; i < kTotalRows; ++i) {
        mRowName[i] = mRowValue[i] = nullptr;
    }
}

void LiveScreen::buildMain()
{
    using F = Theme::Font;
    mFaceRoot = Theme::container(mRoot, 0, 0, 240, 240);
    mLapNo    = Theme::label(mFaceRoot, F::SemiBold25, Strings::kNoValue, 0, 24, 240);
    mLapTime  = Theme::label(mFaceRoot, F::Light60, "00:00", 0, kLapTimeY, 240);
    mInfo     = Theme::label(mFaceRoot, F::Regular14, "", 0, 138, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mHr       = Theme::label(mFaceRoot, F::Medium25, "-- bpm", 0, 154, 240);
    mZone     = Theme::label(mFaceRoot, F::Regular14, "", 0, 192, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void LiveScreen::buildTotals()
{
    using F = Theme::Font;
    mFaceRoot = Theme::container(mRoot, 0, 0, 240, 240);
    for (uint8_t i = 0; i < kTotals; ++i) {
        const int32_t y = 52 + static_cast<int32_t>(i) * 34;
        mRowName[i]  = Theme::label(mFaceRoot, F::Medium18, "", 44, y, 70, LV_TEXT_ALIGN_LEFT);
        // Right-aligned to x = 196; the box is wide enough for "999.99 km" (the name is short beside it).
        mRowValue[i] = Theme::label(mFaceRoot, F::Medium18, "", 84, y, 112, LV_TEXT_ALIGN_RIGHT);
    }
    mFaceTitle = std::make_unique<Widgets::Title>(mFaceRoot, Strings::kTotals);
}

void LiveScreen::buildZones()
{
    using F = Theme::Font;
    mFaceRoot = Theme::container(mRoot, 0, 0, 240, 240);
    // Zone 5 at the top, as the example's summary lists them hardest first.
    for (uint8_t i = 0; i < kTotalRows; ++i) {
        const int32_t y = kZonesY + static_cast<int32_t>(i) * kZonesStep;
        mRowName[i]  = Theme::label(mFaceRoot, F::Medium18, "", 50, y, 50, LV_TEXT_ALIGN_LEFT);
        mRowValue[i] = Theme::label(mFaceRoot, F::Medium18, "", 100, y, 90, LV_TEXT_ALIGN_RIGHT);
    }
    mFaceTitle = std::make_unique<Widgets::Title>(mFaceRoot, Strings::kZones);
}

void LiveScreen::buildClock()
{
    using F = Theme::Font;
    mFaceRoot   = Theme::container(mRoot, 0, 0, 240, 240);
    mClockLap   = Theme::label(mFaceRoot, F::Regular14, "", 0, 44, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mClock      = Theme::label(mFaceRoot, F::SemiBold40, "--:--", 0, 70, 240);
    mBatteryPct = Theme::label(mFaceRoot, F::Medium18, "--%", 0, 124, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mBattery    = std::make_unique<Widgets::Battery>(mFaceRoot, 75, 156);
}

void LiveScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;

    const bool click = code == Btn::L1 || code == Btn::L2 || code == Btn::R1 || code == Btn::R2;
    if (click) {
        hideToast();
    }

    switch (code) {
        case Btn::L1:
            showFace(stepFace(-1));
            redraw();
            break;

        case Btn::L2:
            showFace(stepFace(+1));
            redraw();
            break;

        case Btn::R1:
            ScreenManager::instance().goTo(ScreenId::Action);
            break;

        case Btn::R2:
            // A paused ride has no lap to end; the model drops a double tap.
            if (!mModel.paused()) {
                mModel.lap();
            }
            break;

        default:
            break;
    }
}

void LiveScreen::onSessionUpdate(const Ride::SessionSnapshot&)
{
    redraw();
}

void LiveScreen::onLapEvent(const Ride::LapEventData& event)
{
    showToast(event);
}

void LiveScreen::redraw()
{
    const Ride::SessionSnapshot& s = mModel.snapshot();

    Theme::setArcColor(mRing, ringColor(s));
    mButtons->setR2(mModel.paused() ? Widgets::Buttons::NONE : Widgets::Buttons::AMBER);

    switch (mFace) {
        case FaceMain:   redrawMain(s); break;
        case FaceLap:    redrawLap(s); break;
        case FaceTotals: redrawTotals(s); break;
        case FaceZones:  redrawZones(s); break;
        default:         redrawClock(s); break;
    }
}

void LiveScreen::redrawMain(const Ride::SessionSnapshot& s)
{
    char buf[64];   // "km/h  999.99 km  99:59:59" and its worst-case bounds
    const bool paused = s.state == static_cast<uint8_t>(Ride::RunState::Paused);

    std::snprintf(buf, sizeof(buf), "Lap %u", static_cast<unsigned>(s.laps));
    Theme::assign(mLapNo, buf);

    const bool imperial = mModel.imperial();
    const lv_font_t* big = Theme::font(Theme::Font::Light60);
    if (s.outdoor) {
        // Outdoors the large number is the speed (60 px holds "999.9" with room to spare).
        Ride::formatSpeed(buf, sizeof(buf), s.speedMmS, imperial);
        setBigNumber(mLapTime, big, big);
        Theme::setText(mLapTime, buf, paused ? Color::GRAY : Color::WHITE);
    } else {
        // An hour into one lap the digits no longer fit the face at 60 px.
        Ride::formatClock(buf, sizeof(buf), s.lapMs);
        const lv_font_t* lapFont = s.lapMs >= 3600000u ? Theme::font(Theme::Font::SemiBold40) : big;
        setBigNumber(mLapTime, lapFont, big);
        Theme::setText(mLapTime, buf, paused ? Color::GRAY : Color::WHITE);
    }

    if (!s.logOk) {
        Theme::setText(mInfo, Strings::kFileTrouble, Color::RED);
    } else if (paused) {
        Theme::setText(mInfo, Strings::kPaused, Color::YELLOW_DARK);
    } else if (s.outdoor) {
        char dist[16];
        char total[16];
        Ride::formatDistance(dist, sizeof(dist), s.distCm, imperial);
        Ride::formatClock(total, sizeof(total), s.activeMs);
        std::snprintf(buf, sizeof(buf), "%s  %s %s  %s", speedUnit(imperial), dist, distUnit(imperial), total);
        Theme::setText(mInfo, buf, Color::GRAY);
    } else {
        char total[16];
        Ride::formatClock(total, sizeof(total), s.activeMs);
        std::snprintf(buf, sizeof(buf), "Total %s", total);
        Theme::setText(mInfo, buf, Color::GRAY);
    }

    char bpm[8];
    Ride::formatBpm(bpm, sizeof(bpm), s.hr);
    std::snprintf(buf, sizeof(buf), "%s bpm", bpm);
    Theme::assign(mHr, buf);

    // The zone, in its colour; nothing below zone 1.
    if (s.zone >= 1) {
        std::snprintf(buf, sizeof(buf), "Zone %u", static_cast<unsigned>(s.zone > 5 ? 5 : s.zone));
        Theme::setText(mZone, buf, Theme::zoneColor(s.zone));
    } else {
        Theme::assign(mZone, "");
    }
}

void LiveScreen::redrawLap(const Ride::SessionSnapshot& s)
{
    char buf[40];
    char part[24];
    const bool imperial = mModel.imperial();
    const bool paused   = s.state == static_cast<uint8_t>(Ride::RunState::Paused);
    const lv_font_t* big = Theme::font(Theme::Font::Light60);

    std::snprintf(buf, sizeof(buf), "Lap %u", static_cast<unsigned>(s.laps));
    Theme::assign(mLapNo, buf);

    Ride::formatClock(buf, sizeof(buf), s.lapMs);
    const lv_font_t* lapFont = s.lapMs >= 3600000u ? Theme::font(Theme::Font::SemiBold40) : big;
    setBigNumber(mLapTime, lapFont, big);
    Theme::setText(mLapTime, buf, paused ? Color::GRAY : Color::WHITE);

    Ride::formatDistance(part, sizeof(part), s.lapDistCm, imperial);
    std::snprintf(buf, sizeof(buf), "%s %s", part, distUnit(imperial));
    Theme::setText(mInfo, buf, Color::GRAY);

    Ride::formatSpeed(part, sizeof(part), s.lapAvgSpeedMmS, imperial);
    std::snprintf(buf, sizeof(buf), "%s %s", part, speedUnit(imperial));
    Theme::assign(mHr, buf);
    Theme::setText(mZone, "lap average", Color::GRAY);
}

void LiveScreen::redrawTotals(const Ride::SessionSnapshot& s)
{
    char value[kTotals][32];
    const char* names[kTotals];
    Ride::formatClock(value[0], sizeof(value[0]), s.activeMs);
    if (s.outdoor) {
        const bool imperial = mModel.imperial();
        char part[16];
        Ride::formatDistance(part, sizeof(part), s.distCm, imperial);
        std::snprintf(value[1], sizeof(value[1]), "%s %s", part, distUnit(imperial));
        Ride::formatClimb(part, sizeof(part), s.ascentCm, imperial);
        std::snprintf(value[2], sizeof(value[2]), "%s %s", part, climbUnit(imperial));
        Ride::formatKcal(value[3], sizeof(value[3]), s.kcalX10);
        names[0] = Strings::kTime;
        names[1] = Strings::kDistance;
        names[2] = Strings::kClimb;
        names[3] = Strings::kCalories;
    } else {
        Ride::formatKcal(value[1], sizeof(value[1]), s.kcalX10);
        std::snprintf(value[2], sizeof(value[2]), "%u", static_cast<unsigned>(s.laps));
        Ride::formatAutoLap(value[3], sizeof(value[3]), s.autoLapMin);
        names[0] = Strings::kTotal;
        names[1] = Strings::kCalories;
        names[2] = Strings::kLapsRow;
        names[3] = Strings::kAutoLapRow;
    }
    for (uint8_t i = 0; i < kTotals; ++i) {
        Theme::assign(mRowName[i], names[i]);
        Theme::assign(mRowValue[i], value[i]);
    }
}

void LiveScreen::redrawZones(const Ride::SessionSnapshot& s)
{
    for (uint8_t i = 0; i < kTotalRows; ++i) {
        const uint8_t zone = static_cast<uint8_t>(5u - i);
        char name[8];
        char time[16];
        Ride::formatZone(name, sizeof(name), zone);
        Ride::formatClock(time, sizeof(time), s.zoneMs[zone]);
        // The zone on now is bright; the rest are grey once they have nothing.
        const uint32_t color = s.zoneMs[zone] > 0 || s.zone == zone ? Theme::zoneColor(zone) : Color::GRAY;
        Theme::setText(mRowName[i], name, color);
        Theme::setText(mRowValue[i], time, color);
    }
}

void LiveScreen::redrawClock(const Ride::SessionSnapshot& s)
{
    char buf[24];

    // What the ride is doing, small, so the clock face is not a blind spot.
    char lap[16];
    Ride::formatClock(lap, sizeof(lap), s.lapMs);
    std::snprintf(buf, sizeof(buf), "Lap %u %s", static_cast<unsigned>(s.laps), lap);
    Theme::setText(mClockLap, buf, ringColor(s) == Color::GRAY ? Color::GRAY : ringColor(s));

    Ride::formatHourMinute(buf, sizeof(buf), s.hour, s.minute);
    Theme::assign(mClock, buf);

    Ride::formatPercent(buf, sizeof(buf), s.battery, Ride::kBatteryUnknown);
    Theme::assign(mBatteryPct, buf);
    mBattery->setLevel(s.battery <= 100 ? s.battery : 0);
}

void LiveScreen::showToast(const Ride::LapEventData& e)
{
    using F = Theme::Font;
    char buf[64];
    char clock[16];

    const uint32_t color = Color::YELLOW_DARK;   // the lap button's amber

    if (mToast) {
        lv_obj_delete(mToast);
    }
    mToast = Theme::box(mRoot, 0, 0, 240, 240, Color::BLACK);
    // Over the faces and the ring, under the bezel hints.
    lv_obj_move_to_index(mToast, static_cast<int32_t>(lv_obj_get_child_count(mRoot)) - 1 - 4);

    Theme::arc(mToast, Widgets::kCx, Widgets::kCy, kRingRadius, kRingWidth, 0, 360, color);
    // The lap now begun ("Auto lap" when the auto lap ended the last), then the
    // one just finished: how long, and its heart rate.
    if (e.autoEnded) {
        std::snprintf(buf, sizeof(buf), "Auto lap");
    } else {
        std::snprintf(buf, sizeof(buf), "Lap %u", static_cast<unsigned>(e.finishedLap + 1u));
    }
    Theme::label(mToast, F::SemiBold30, buf, 0, 52, 240, LV_TEXT_ALIGN_CENTER, color);

    Ride::formatClock(clock, sizeof(clock), e.durationMs);
    std::snprintf(buf, sizeof(buf), "Lap %u %s", static_cast<unsigned>(e.finishedLap), clock);
    Theme::label(mToast, F::Medium18, buf, 0, 106, 240);

    if (e.hrAvg == 0) {
        std::snprintf(buf, sizeof(buf), "no heart rate");
    } else {
        std::snprintf(buf, sizeof(buf), "average %u bpm", static_cast<unsigned>(e.hrAvg));
    }
    Theme::label(mToast, F::Regular14, buf, 0, 138, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);

    if (mModel.snapshot().outdoor) {
        // Outdoors the lap's distance and average speed too.
        const bool imperial = mModel.imperial();
        char dist[16];
        char speed[16];
        Ride::formatDistance(dist, sizeof(dist), e.distCm, imperial);
        Ride::formatSpeed(speed, sizeof(speed), e.avgSpeedMmS, imperial);
        std::snprintf(buf, sizeof(buf), "%s %s  %s %s", dist, distUnit(imperial), speed, speedUnit(imperial));
        Theme::label(mToast, F::Regular14, buf, 0, 160, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    }

    if (mToastTimer) {
        lv_timer_reset(mToastTimer);
    } else {
        mToastTimer = lv_timer_create(&LiveScreen::toastCb, kToastMs, this);
        lv_timer_set_repeat_count(mToastTimer, 1);
    }
}

void LiveScreen::hideToast()
{
    if (mToastTimer) {
        lv_timer_delete(mToastTimer);
        mToastTimer = nullptr;
    }
    if (mToast) {
        lv_obj_delete(mToast);
        mToast = nullptr;
    }
}

void LiveScreen::toastCb(lv_timer_t* t)
{
    auto* self = static_cast<LiveScreen*>(lv_timer_get_user_data(t));
    self->mToastTimer = nullptr;   // one-shot: LVGL deletes the timer after this call
    if (self->mToast) {
        lv_obj_delete(self->mToast);
        self->mToast = nullptr;
    }
}
