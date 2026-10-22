/**
 ******************************************************************************
 * @file    LiveScreen.cpp
 * @brief   The session on the wrist (see LiveScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/LiveScreen.hpp"

#include <cstdio>

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "SessionText.hpp"

using namespace SDK::GUI;

namespace
{
/// How long the lap toast stays.
constexpr uint32_t kToastMs = 2000;

constexpr int32_t kLapTimeY   = 54;    // top of the lap time's box, in the 60 px face

constexpr int32_t kRingRadius = 113;   // under the bezel arcs, which share it
constexpr int32_t kRingWidth  = 6;

uint32_t ringColor(const Sauna::SessionSnapshot& s)
{
    if (s.state == static_cast<uint8_t>(Sauna::RunState::Paused)) {
        return Color::GRAY;
    }
    return Theme::phaseColor(static_cast<Sauna::Phase>(s.phase));
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
        default:         buildClock(); break;
    }
    // Just above the ring, so the bezel hints stay on top.
    lv_obj_move_to_index(mFaceRoot, 1);
}

void LiveScreen::destroyFace()
{
    mTotalsTitle.reset();
    mBattery.reset();
    if (mFaceRoot) {
        lv_obj_delete(mFaceRoot);
        mFaceRoot = nullptr;
    }
    mPhase = mLapTime = mInfo = mHr = mRecovery = nullptr;
    mClockPhase = mClock = mBatteryPct = nullptr;
    for (uint8_t i = 0; i < kTotalRows; ++i) {
        mRowName[i] = mRowValue[i] = nullptr;
    }
}

void LiveScreen::buildMain()
{
    using F = Theme::Font;
    mFaceRoot = Theme::container(mRoot, 0, 0, 240, 240);
    mPhase    = Theme::label(mFaceRoot, F::SemiBold25, Strings::kNoValue, 0, 24, 240);
    mLapTime  = Theme::label(mFaceRoot, F::Light60, "00:00", 0, kLapTimeY, 240);
    mInfo     = Theme::label(mFaceRoot, F::Regular14, "", 0, 138, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mHr       = Theme::label(mFaceRoot, F::Medium25, "-- bpm", 0, 154, 240);
    mRecovery = Theme::label(mFaceRoot, F::Regular14, "", 0, 192, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void LiveScreen::buildTotals()
{
    using F = Theme::Font;
    mFaceRoot = Theme::container(mRoot, 0, 0, 240, 240);
    for (uint8_t i = 0; i < kTotalRows; ++i) {
        const int32_t y = 52 + static_cast<int32_t>(i) * 34;
        mRowName[i]  = Theme::label(mFaceRoot, F::Medium18, "", 44, y, 70, LV_TEXT_ALIGN_LEFT);
        mRowValue[i] = Theme::label(mFaceRoot, F::Medium18, "", 104, y, 92, LV_TEXT_ALIGN_RIGHT);
    }
    mTotalsTitle = std::make_unique<Widgets::Title>(mFaceRoot, Strings::kTotals);
}

void LiveScreen::buildClock()
{
    using F = Theme::Font;
    mFaceRoot   = Theme::container(mRoot, 0, 0, 240, 240);
    mClockPhase = Theme::label(mFaceRoot, F::Regular14, "", 0, 44, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
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
            // A paused session has no lap to end; the model drops a double tap.
            if (!mModel.paused()) {
                mModel.lap();
            }
            break;

        default:
            break;
    }
}

void LiveScreen::onSessionUpdate(const Sauna::SessionSnapshot&)
{
    redraw();
}

void LiveScreen::onLapEvent(const Sauna::LapEventData& event)
{
    showToast(event);
}

void LiveScreen::redraw()
{
    const Sauna::SessionSnapshot& s = mModel.snapshot();

    Theme::setArcColor(mRing, ringColor(s));
    mButtons->setR2(mModel.paused() ? Widgets::Buttons::NONE : Widgets::Buttons::AMBER);

    switch (mFace) {
        case FaceMain:   redrawMain(s); break;
        case FaceTotals: redrawTotals(s); break;
        default:         redrawClock(s); break;
    }
}

void LiveScreen::redrawMain(const Sauna::SessionSnapshot& s)
{
    char buf[40];
    const bool paused = s.state == static_cast<uint8_t>(Sauna::RunState::Paused);
    const uint32_t color = ringColor(s);

    Theme::setText(mPhase, Strings::phase(s.phase), color);

    // An hour into one lap the digits no longer fit the face at 60 px.
    Sauna::formatClock(buf, sizeof(buf), s.lapMs);
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
        Sauna::formatClock(total, sizeof(total), s.activeMs);
        std::snprintf(buf, sizeof(buf), "Round %u, total %s", static_cast<unsigned>(s.round), total);
        Theme::setText(mInfo, buf, Color::GRAY);
    }

    char bpm[8];
    Sauna::formatBpm(bpm, sizeof(bpm), s.hr);
    std::snprintf(buf, sizeof(buf), "%s bpm", bpm);
    Theme::assign(mHr, buf);

    Sauna::formatRecoveryLine(buf, sizeof(buf), static_cast<Sauna::Recovery::State>(s.recoveryState),
                              s.recoverySecLeft, s.recoveryDrop);
    Theme::assign(mRecovery, buf);
}

void LiveScreen::redrawTotals(const Sauna::SessionSnapshot& s)
{
    // Rows: each phase the mode uses, then the total. Rest is shown when the
    // session has it (Just sauna always, otherwise when chosen).
    const bool justSauna = s.mode == static_cast<uint8_t>(Sauna::Mode::SaunaOnly);
    const bool plunge    = !justSauna;
    const bool rest      = justSauna || s.rest != 0;

    struct Row { const char* name; uint32_t ms; uint32_t color; };
    Row rows[kTotalRows];
    uint8_t n = 0;
    rows[n++] = { Strings::phase(0), s.phaseMs[0], Theme::phaseColor(Sauna::Phase::Sauna) };
    if (plunge) {
        rows[n++] = { Strings::phase(1), s.phaseMs[1], Theme::phaseColor(Sauna::Phase::Plunge) };
    }
    if (rest) {
        rows[n++] = { Strings::phase(2), s.phaseMs[2], Theme::phaseColor(Sauna::Phase::Rest) };
    }
    rows[n++] = { Strings::kTotal, s.activeMs, Color::WHITE };

    // Centre the rows in the room under the title.
    const int32_t y0 = 52 + static_cast<int32_t>(kTotalRows - n) * 17;
    for (uint8_t i = 0; i < kTotalRows; ++i) {
        if (i >= n) {
            Theme::setHidden(mRowName[i], true);
            Theme::setHidden(mRowValue[i], true);
            continue;
        }
        char buf[16];
        Sauna::formatClock(buf, sizeof(buf), rows[i].ms);
        Theme::setHidden(mRowName[i], false);
        Theme::setHidden(mRowValue[i], false);
        lv_obj_set_y(mRowName[i], y0 + static_cast<int32_t>(i) * 34);
        lv_obj_set_y(mRowValue[i], y0 + static_cast<int32_t>(i) * 34);
        Theme::setText(mRowName[i], rows[i].name, rows[i].color);
        Theme::setText(mRowValue[i], buf, rows[i].color);
    }
}

void LiveScreen::redrawClock(const Sauna::SessionSnapshot& s)
{
    char buf[24];

    // What the session is doing, small, so the clock face is not a blind spot.
    char lap[16];
    Sauna::formatClock(lap, sizeof(lap), s.lapMs);
    std::snprintf(buf, sizeof(buf), "%s %s", Strings::phase(s.phase), lap);
    Theme::setText(mClockPhase, buf, ringColor(s));

    Sauna::formatHourMinute(buf, sizeof(buf), s.hour, s.minute);
    Theme::assign(mClock, buf);

    Sauna::formatPercent(buf, sizeof(buf), s.battery, Sauna::kBatteryUnknown);
    Theme::assign(mBatteryPct, buf);
    mBattery->setLevel(s.battery <= 100 ? s.battery : 0);
}

void LiveScreen::showToast(const Sauna::LapEventData& e)
{
    using F = Theme::Font;
    char buf[40];
    char clock[16];

    const uint32_t color = Theme::phaseColor(static_cast<Sauna::Phase>(e.nextPhase));

    if (mToast) {
        lv_obj_delete(mToast);
    }
    mToast = Theme::box(mRoot, 0, 0, 240, 240, Color::BLACK);
    // Over the faces and the ring, under the bezel hints.
    lv_obj_move_to_index(mToast, static_cast<int32_t>(lv_obj_get_child_count(mRoot)) - 1 - 4);

    Theme::arc(mToast, Widgets::kCx, Widgets::kCy, kRingRadius, kRingWidth, 0, 360, color);
    Theme::label(mToast, F::SemiBold30, Strings::phase(e.nextPhase), 0, 52, 240, LV_TEXT_ALIGN_CENTER, color);

    // The lap just finished: what it was and how long.
    Sauna::formatClock(clock, sizeof(clock), e.durationMs);
    std::snprintf(buf, sizeof(buf), "%s %s", Strings::phase(e.finishedPhase), clock);
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
