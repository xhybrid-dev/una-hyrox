/**
 ******************************************************************************
 * @file    LiveScreen.cpp
 * @brief   The workout on the wrist (see LiveScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/LiveScreen.hpp"

#include <cstdio>

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "PlanText.hpp"
#include "WorkoutText.hpp"

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

bool planDone(const Workout::SessionSnapshot& s)
{
    return s.plan.steps > 0 && s.plan.done;
}

/// R2 does something now: not paused, not done, and not a timer kind.
bool lapButtonLive(const Workout::SessionSnapshot& s)
{
    return s.state == static_cast<uint8_t>(Workout::RunState::Running) && !planDone(s) &&
           Workout::buttonCounts(s.plan.type);
}

uint32_t ringColor(const Workout::SessionSnapshot& s)
{
    if (s.state == static_cast<uint8_t>(Workout::RunState::Paused)) {
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

    showFace(mModel.liveFace() < FaceCount ? mModel.liveFace() : static_cast<uint8_t>(FaceMain));
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
        mRowValue[i] = Theme::label(mFaceRoot, F::Medium18, "", 104, y, 92, LV_TEXT_ALIGN_RIGHT);
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
            showFace(static_cast<uint8_t>((mFace + FaceCount - 1) % FaceCount));
            redraw();
            break;

        case Btn::L2:
            showFace(static_cast<uint8_t>((mFace + 1) % FaceCount));
            redraw();
            break;

        case Btn::R1:
            ScreenManager::instance().goTo(ScreenId::Action);
            break;

        case Btn::R2:
            // A paused or finished workout has no lap to end, and a timer takes
            // no button; the model drops a double tap.
            if (lapButtonLive(mModel.snapshot())) {
                mModel.lap();
            }
            break;

        default:
            break;
    }
}

void LiveScreen::onSessionUpdate(const Workout::SessionSnapshot&)
{
    redraw();
}

void LiveScreen::onLapEvent(const Workout::LapEventData& event)
{
    showToast(event);
}

void LiveScreen::redraw()
{
    const Workout::SessionSnapshot& s = mModel.snapshot();

    Theme::setArcColor(mRing, ringColor(s));
    mButtons->setR2(lapButtonLive(s) ? Widgets::Buttons::AMBER : Widgets::Buttons::NONE);

    switch (mFace) {
        case FaceMain:   redrawMain(s); break;
        case FaceTotals: redrawTotals(s); break;
        case FaceZones:  redrawZones(s); break;
        default:         redrawClock(s); break;
    }
}

void LiveScreen::redrawMain(const Workout::SessionSnapshot& s)
{
    if (s.plan.steps > 0) {
        redrawPlanMain(s);
        return;
    }
    char buf[40];
    lv_obj_set_style_text_font(mLapNo, Theme::font(Theme::Font::SemiBold25), LV_PART_MAIN);
    lv_obj_set_style_text_color(mLapNo, Theme::rgb(Color::WHITE), LV_PART_MAIN);
    const bool paused = s.state == static_cast<uint8_t>(Workout::RunState::Paused);

    std::snprintf(buf, sizeof(buf), "Lap %u", static_cast<unsigned>(s.laps));
    Theme::assign(mLapNo, buf);

    // An hour into one lap the digits no longer fit the face at 60 px.
    Workout::formatClock(buf, sizeof(buf), s.lapMs);
    const lv_font_t* big   = Theme::font(Theme::Font::Light60);
    const lv_font_t* lapFont = s.lapMs >= 3600000u ? Theme::font(Theme::Font::SemiBold40) : big;
    lv_obj_set_style_text_font(mLapTime, lapFont, LV_PART_MAIN);
    // Keep the digits on the same centre line whichever face is used.
    lv_obj_set_y(mLapTime, kLapTimeY + (lv_font_get_line_height(big) - lv_font_get_line_height(lapFont)) / 2);
    Theme::setText(mLapTime, buf, paused ? Color::GRAY : Color::WHITE);

    if (!s.logOk) {
        Theme::setText(mInfo, Strings::kFileTrouble, Color::RED);
    } else if (paused) {
        Theme::setText(mInfo, Strings::kPaused, Color::YELLOW_DARK);
    } else {
        char total[16];
        Workout::formatClock(total, sizeof(total), s.activeMs);
        std::snprintf(buf, sizeof(buf), "Total %s", total);
        Theme::setText(mInfo, buf, Color::GRAY);
    }

    char bpm[8];
    Workout::formatBpm(bpm, sizeof(bpm), s.hr);
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

void LiveScreen::redrawPlanMain(const Workout::SessionSnapshot& s)
{
    const Workout::PlanView& v = s.plan;
    const bool paused = s.state == static_cast<uint8_t>(Workout::RunState::Paused) && !v.done;
    char buf[40];

    // The step, smaller than a Free workout's lap number: the face is narrow up there.
    Workout::formatStepHeading(buf, sizeof(buf), v);
    lv_obj_set_style_text_font(mLapNo, Theme::font(Theme::Font::Medium18), LV_PART_MAIN);
    Theme::setText(mLapNo, buf, v.done ? Color::GREEN : Theme::stepColor(v.kind));

    // The step's time: down for a timed step, up for an open one; the total once done.
    const bool down = Workout::countsDown(v);
    uint32_t ms = v.done ? s.activeMs : v.stepMs;
    if (down) {
        Workout::formatCountdown(buf, sizeof(buf), v.remainingMs);
    } else {
        Workout::formatClock(buf, sizeof(buf), ms);
    }
    if (down) {
        ms = v.remainingMs;
    }
    const lv_font_t* big  = Theme::font(Theme::Font::Light60);
    const lv_font_t* font = ms >= 3600000u ? Theme::font(Theme::Font::SemiBold40) : big;
    lv_obj_set_style_text_font(mLapTime, font, LV_PART_MAIN);
    lv_obj_set_y(mLapTime, kLapTimeY + (lv_font_get_line_height(big) - lv_font_get_line_height(font)) / 2);
    uint32_t color = Color::WHITE;
    if (paused) {
        color = Color::GRAY;
    } else if (down && v.remainingMs <= Workout::kWarnMs) {
        color = Color::YELLOW_DARK;   // the countdown buzz's three seconds
    } else if (v.kind == static_cast<uint8_t>(Workout::StepKind::Rest) && !v.done) {
        color = Theme::stepColor(v.kind);
    }
    Theme::setText(mLapTime, buf, color);

    if (!s.logOk) {
        Theme::setText(mInfo, Strings::kFileTrouble, Color::RED);
    } else if (v.done) {
        Theme::setText(mInfo, Strings::kPlanDone, Color::YELLOW_DARK);
    } else if (paused) {
        Theme::setText(mInfo, Strings::kPaused, Color::YELLOW_DARK);
    } else if (v.kind == static_cast<uint8_t>(Workout::StepKind::Station)) {
        Theme::setText(mInfo, Workout::circuitStation(v.station).work, Color::YELLOW_DARK);
    } else if (v.kind == static_cast<uint8_t>(Workout::StepKind::Run)) {
        Workout::formatRunM(buf, sizeof(buf), v.distanceM);
        Theme::setText(mInfo, buf, Color::GRAY);
    } else {
        char total[16];
        Workout::formatClock(total, sizeof(total), s.activeMs);
        std::snprintf(buf, sizeof(buf), "Total %s", total);
        Theme::setText(mInfo, buf, Color::GRAY);
    }

    char bpm[8];
    Workout::formatBpm(bpm, sizeof(bpm), s.hr);
    std::snprintf(buf, sizeof(buf), "%s bpm", bpm);
    Theme::assign(mHr, buf);
    if (s.zone >= 1) {
        std::snprintf(buf, sizeof(buf), "Zone %u", static_cast<unsigned>(s.zone > 5 ? 5 : s.zone));
        Theme::setText(mZone, buf, Theme::zoneColor(s.zone));
    } else {
        Theme::assign(mZone, "");
    }
}

void LiveScreen::redrawTotals(const Workout::SessionSnapshot& s)
{
    char value[kTotals][16];
    Workout::formatClock(value[0], sizeof(value[0]), s.activeMs);
    Workout::formatKcal(value[1], sizeof(value[1]), s.kcalX10);
    std::snprintf(value[2], sizeof(value[2]), "%u", static_cast<unsigned>(s.laps));
    const bool plan = s.plan.steps > 0;
    if (plan) {
        std::snprintf(value[3], sizeof(value[3]), "%s",
                      Workout::kindName(static_cast<Workout::SessionType>(s.plan.type)));
    } else {
        Workout::formatAutoLap(value[3], sizeof(value[3]), s.autoLapMin);
    }
    const char* names[kTotals] = {Strings::kTotal, Strings::kCalories, Strings::kLapsRow,
                                  plan ? Strings::kTypeRow : Strings::kAutoLapRow};
    for (uint8_t i = 0; i < kTotals; ++i) {
        Theme::assign(mRowName[i], names[i]);
        Theme::assign(mRowValue[i], value[i]);
    }
}

void LiveScreen::redrawZones(const Workout::SessionSnapshot& s)
{
    for (uint8_t i = 0; i < kTotalRows; ++i) {
        const uint8_t zone = static_cast<uint8_t>(5u - i);
        char name[8];
        char time[16];
        Workout::formatZone(name, sizeof(name), zone);
        Workout::formatClock(time, sizeof(time), s.zoneMs[zone]);
        // The zone on now is bright; the rest are grey once they have nothing.
        const uint32_t color = s.zoneMs[zone] > 0 || s.zone == zone ? Theme::zoneColor(zone) : Color::GRAY;
        Theme::setText(mRowName[i], name, color);
        Theme::setText(mRowValue[i], time, color);
    }
}

void LiveScreen::redrawClock(const Workout::SessionSnapshot& s)
{
    char buf[24];

    // What the workout is doing, small, so the clock face is not a blind spot.
    char lap[16];
    Workout::formatClock(lap, sizeof(lap), s.lapMs);
    std::snprintf(buf, sizeof(buf), "Lap %u %s", static_cast<unsigned>(s.laps), lap);
    Theme::setText(mClockLap, buf, ringColor(s) == Color::GRAY ? Color::GRAY : ringColor(s));

    Workout::formatHourMinute(buf, sizeof(buf), s.hour, s.minute);
    Theme::assign(mClock, buf);

    Workout::formatPercent(buf, sizeof(buf), s.battery, Workout::kBatteryUnknown);
    Theme::assign(mBatteryPct, buf);
    mBattery->setLevel(s.battery <= 100 ? s.battery : 0);
}

void LiveScreen::showToast(const Workout::LapEventData& e)
{
    using F = Theme::Font;
    char buf[40];
    char clock[16];

    // The running workout's kind (a Free workout's is Plain: its laps read as before).
    const uint8_t type = mModel.snapshot().plan.type;
    uint32_t color = Color::YELLOW_DARK;   // the lap button's amber
    switch (static_cast<Workout::Cue>(e.cue)) {
        case Workout::Cue::Done:    color = Color::GREEN; break;
        case Workout::Cue::Rest:    color = Theme::stepColor(static_cast<uint8_t>(Workout::StepKind::Rest)); break;
        case Workout::Cue::Work:    color = Theme::stepColor(static_cast<uint8_t>(Workout::StepKind::Work)); break;
        case Workout::Cue::Run:     color = Theme::stepColor(static_cast<uint8_t>(Workout::StepKind::Run)); break;
        default: break;
    }

    if (mToast) {
        lv_obj_delete(mToast);
    }
    mToast = Theme::box(mRoot, 0, 0, 240, 240, Color::BLACK);
    // Over the faces and the ring, under the bezel hints.
    lv_obj_move_to_index(mToast, static_cast<int32_t>(lv_obj_get_child_count(mRoot)) - 1 - 4);

    Theme::arc(mToast, Widgets::kCx, Widgets::kCy, kRingRadius, kRingWidth, 0, 360, color);
    // What begins now ("Rest", "Set 3", "Auto lap"...), then the lap just
    // finished: what it was, how long, and its heart rate.
    Workout::formatCueHeading(buf, sizeof(buf), type, e);
    Theme::label(mToast, F::SemiBold30, buf, 0, 52, 240, LV_TEXT_ALIGN_CENTER, color);

    char name[24];
    Workout::formatClock(clock, sizeof(clock), e.durationMs);
    Workout::formatStepName(name, sizeof(name), type, e.kind, e.round, e.station, e.finishedLap, e.autoEnded != 0);
    std::snprintf(buf, sizeof(buf), "%s %s", name, clock);
    Theme::label(mToast, F::Medium18, buf, 0, 106, 240);

    if (e.hrAvg == 0) {
        std::snprintf(buf, sizeof(buf), "no heart rate");
    } else {
        std::snprintf(buf, sizeof(buf), "average %u bpm", static_cast<unsigned>(e.hrAvg));
    }
    Theme::label(mToast, F::Regular14, buf, 0, 138, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);

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
