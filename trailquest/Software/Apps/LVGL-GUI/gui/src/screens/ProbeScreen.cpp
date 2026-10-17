/**
 ******************************************************************************
 * @file    ProbeScreen.cpp
 * @brief   The Q0 feasibility screen (see ProbeScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/ProbeScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "Feedback.hpp"
#include "gui/Assets.hpp"
#include "gui/Sprites.hpp"

#define LOG_MODULE_PRX      "Probe"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace Draw  = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;
using namespace TrailQuest;

namespace
{
// The three live sprites' buffers, outside LVGL's pool (see Sprite.hpp).
alignas(4) uint8_t sSpriteBuf[3][Sprite::kBytes];

// The beat track.
constexpr int32_t kTrackX = 45;
constexpr int32_t kTrackW = 150;
constexpr int32_t kTrackY = 164;
constexpr int32_t kTrackH = 12;
constexpr int32_t kNeedleW = 3;

/// How often the sprites flip between their two frames, ms.
constexpr uint32_t kIdleMs = 500;

/// A time in the sweep as an x on screen (Beat.hpp's beatToTrack, so the
/// drawing and the judging share one mapping).
int32_t trackX(const BeatConfig& c, uint32_t ms)
{
    return kTrackX + static_cast<int32_t>(beatToTrack(c, ms, static_cast<uint32_t>(kTrackW)));
}

/// A zone of @p widthMs centred on the target, drawn as one box. clampBeat()
/// keeps the good zone (and so the perfect one) inside the sweep.
lv_obj_t* zoneBox(lv_obj_t* parent, const BeatConfig& c, uint32_t widthMs, uint32_t color)
{
    const uint32_t half = (widthMs + 1u) / 2u;
    const int32_t  x0   = trackX(c, c.targetMs - half);
    const int32_t  x1   = trackX(c, c.targetMs + half);
    return Draw::box(parent, x0, kTrackY, x1 - x0, kTrackH, color);
}
} // namespace

ProbeScreen::ProbeScreen(Model& model)
    : mModel(model)
    , mBeat(clampBeat(BeatConfig{}))
{
}

ProbeScreen::~ProbeScreen()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
    lv_display_remove_event_cb_with_user_data(lv_display_get_default(), renderCb, this);
    mButtons.reset();
    if (mRoot) {
        lv_obj_delete(mRoot);
    }
}

void ProbeScreen::show()
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &ProbeScreen::keyEventCb, LV_EVENT_KEY, this);

    // Scene: sky, far hills, near hill, path. Flat shapes in the SDK palette.
    Draw::box(mRoot, 0, 40, 240, 70, Color::AZURE);
    Draw::box(mRoot, 0, 96, 240, 24, Color::GREEN_MID);
    Draw::box(mRoot, 0, 112, 240, 24, Color::GREEN);
    Draw::box(mRoot, 0, 134, 240, 20, Color::SIENNA_DARK);
    Draw::dot(mRoot, 190, 58, 9, Color::YELLOW);   // sun

    Draw::label(mRoot, &pressstart2p_16, "Q0 PROBE", 40, 21, 160, LV_TEXT_ALIGN_CENTER, Color::LIME);

    mRunner.create(mRoot, 30, 92, TRAILQUEST_SPRITE_RUNNER, sSpriteBuf[0]);
    mStitch.create(mRoot, 100, 92, TRAILQUEST_SPRITE_STITCH, sSpriteBuf[1]);
    mBonk.create(mRoot, 160, 92, TRAILQUEST_SPRITE_BONK, sSpriteBuf[2]);

    // The beat: a track, the good zone, the perfect zone, and the needle.
    Draw::box(mRoot, kTrackX, kTrackY, kTrackW, kTrackH, Color::GRAY_DARK);
    zoneBox(mRoot, mBeat, mBeat.goodMs, Color::YELLOW_DARK);
    zoneBox(mRoot, mBeat, mBeat.perfectMs, Color::GREEN);
    mNeedle = Draw::box(mRoot, kTrackX, kTrackY - 3, kNeedleW, kTrackH + 6, Color::WHITE);

    mResult = Draw::label(mRoot, &pressstart2p_16, "", 20, 180, 200, LV_TEXT_ALIGN_CENTER, Color::WHITE);
    mStats  = Draw::label(mRoot, &poppins_regular_14, "", 35, 200, 170, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mKeys   = Draw::label(mRoot, &poppins_regular_14, "", 62, 217, 116, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mFxName = Draw::label(mRoot, &poppins_regular_14, "", 60, 44, 120, LV_TEXT_ALIGN_CENTER, Color::WHITE);
    std::snprintf(mResultText, sizeof(mResultText), "R1: BEAT");
    lv_label_set_text_static(mResult, mResultText);
    lv_label_set_text_static(mStats, mStatsText);
    lv_label_set_text_static(mKeys, mKeysText);
    lv_label_set_text_static(mFxName, fxPattern(static_cast<FxId>(mFx)).name);   // a string literal

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::AMBER,    // L1: play the pattern
                  SDK::LVGL::Buttons::WHITE,    // L2: next pattern
                  SDK::LVGL::Buttons::GREEN,    // R1: the beat
                  SDK::LVGL::Buttons::RED);     // R2: leave

    // Time-driven, so a period of 1 ms: one run per kernel tick (see lv_conf.h).
    mTimer = lv_timer_create(&ProbeScreen::tickCb, 1, this);
    lv_display_add_event_cb(lv_display_get_default(), &ProbeScreen::renderCb, LV_EVENT_RENDER_START, this);
    lv_display_add_event_cb(lv_display_get_default(), &ProbeScreen::renderCb, LV_EVENT_RENDER_READY, this);

    mBeatStart = lv_tick_get();
    mNeedleMs  = mBeatStart;
    mStatsAt   = mBeatStart;
    mLastFlip  = mBeatStart;

    lv_screen_load(mRoot);
    mModel.bind(this);
}

void ProbeScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<ProbeScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void ProbeScreen::tickCb(lv_timer_t* t)
{
    static_cast<ProbeScreen*>(lv_timer_get_user_data(t))->onTick();
}

void ProbeScreen::renderCb(lv_event_t* e)
{
    auto* self = static_cast<ProbeScreen*>(lv_event_get_user_data(e));
    if (lv_event_get_code(e) == LV_EVENT_RENDER_START) {
        self->mRenderStart = lv_tick_get();
    } else {
        const uint32_t ms = lv_tick_get() - self->mRenderStart;
        self->mRenderLast = ms;
        if (ms > self->mRenderMax) {
            self->mRenderMax = ms;
        }
        ++self->mFrames;
    }
}

void ProbeScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;

    // Keep the last three codes, in arrival order, so the order of press,
    // click and release on the watch is visible.
    if (mCodeCount < sizeof(mCodes)) {
        mCodes[mCodeCount++] = static_cast<char>(code);
    } else {
        mCodes[0] = mCodes[1];
        mCodes[1] = mCodes[2];
        mCodes[2] = static_cast<char>(code);
    }
    updateKeys();
    LOG_INFO("key '%c' at %lu\n", static_cast<char>(code), static_cast<unsigned long>(lv_tick_get()));

    switch (code) {
        case Btn::R1_PRESS: {          // the earliest code for R1
            // The port hands out buttons before LVGL redraws (LvglPort.cpp,
            // Port::run), so the frame on screen still shows the needle drawn
            // last tick. Judge the press there, at what the player saw, not at
            // "now", which is a frame (100 ms) later.
            ++mSeq;
            mPressTick[mSeq % kPressSlots] = lv_tick_get();
            mModel.sendBeatPress(mSeq, mBeatStart, mNeedleMs, mBeat);
            LOG_INFO("beat press %lu judged at the needle drawn at %lu (code at %lu)\n",
                     static_cast<unsigned long>(mSeq), static_cast<unsigned long>(mNeedleMs),
                     static_cast<unsigned long>(mPressTick[mSeq % kPressSlots]));
        } break;
        case Btn::L1:
            mModel.playFx(static_cast<FxId>(mFx));
            break;
        case Btn::L2:
            mFx = static_cast<uint8_t>((mFx + 1u) % static_cast<uint8_t>(FxId::Count));
            lv_label_set_text_static(mFxName, fxPattern(static_cast<FxId>(mFx)).name);
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            break;
    }
}

void ProbeScreen::onBeatResult(const CustomMessage::BeatResult& r)
{
    // Time the answer against its own press. One older than the slots hold
    // cannot be timed, so it leaves the last round trip as it was.
    if (mSeq - r.seq < kPressSlots) {
        mRoundTrip = lv_tick_get() - mPressTick[r.seq % kPressSlots];
        updateKeys();
    }
    static const char* const kNames[] = { "MISS", "GOOD", "PERFECT" };
    std::snprintf(mResultText, sizeof(mResultText), "%s %+ld",
                  kNames[r.result <= 2 ? r.result : 0], static_cast<long>(r.offsetMs));
    lv_label_set_text_static(mResult, mResultText);
    lv_obj_set_style_text_color(mResult,
        Draw::rgb(r.result == static_cast<uint8_t>(BeatResult::Perfect) ? Color::GREEN
                  : r.result == static_cast<uint8_t>(BeatResult::Good)  ? Color::YELLOW
                                                                       : Color::RED), 0);
    LOG_INFO("beat %lu: result %u offset %ld ms, round trip %lu ms\n",
             static_cast<unsigned long>(r.seq), static_cast<unsigned>(r.result),
             static_cast<long>(r.offsetMs), static_cast<unsigned long>(mRoundTrip));
}

void ProbeScreen::onTick()
{
    const uint32_t now = lv_tick_get();

    // The needle, and the time it is drawn for (a press is judged there).
    mNeedleMs = now;
    lv_obj_set_x(mNeedle, trackX(mBeat, beatPhaseMs(mBeat, mBeatStart, now)) - kNeedleW / 2);

    // Idle animation: all three sprites swap frames.
    if (now - mLastFlip >= kIdleMs) {
        mLastFlip = now;
        mFrame ^= 1u;
        mRunner.setFrame(mFrame);
        mStitch.setFrame(mFrame);
        mBonk.setFrame(mFrame);
    }

    if (now - mStatsAt >= 1000) {
        mStatsAt = now;
        updateStats();
    }
}

void ProbeScreen::updateStats()
{
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    const uint32_t used = mon.total_size - mon.free_size;
    std::snprintf(mStatsText, sizeof(mStatsText), "r %lu/%lums  f%lu  p%lu%%",
                  static_cast<unsigned long>(mRenderLast % 1000u), static_cast<unsigned long>(mRenderMax % 1000u),
                  static_cast<unsigned long>(mFrames % 100u),
                  static_cast<unsigned long>(used * 100u / mon.total_size));
    lv_label_set_text_static(mStats, mStatsText);
    LOG_INFO("render last %lu max %lu ms, %lu frames/s, pool %lu/%lu B (peak %u%%), round trip %lu ms\n",
             static_cast<unsigned long>(mRenderLast), static_cast<unsigned long>(mRenderMax),
             static_cast<unsigned long>(mFrames),
             static_cast<unsigned long>(used), static_cast<unsigned long>(mon.total_size),
             static_cast<unsigned>(mon.max_used * 100u / mon.total_size),
             static_cast<unsigned long>(mRoundTrip));
    mRenderMax = 0;
    mFrames    = 0;
}

void ProbeScreen::updateKeys()
{
    char codes[8] = {};
    for (uint8_t i = 0; i < mCodeCount && i < sizeof(mCodes); ++i) {
        codes[i * 2]     = mCodes[i];
        codes[i * 2 + 1] = ' ';
    }
    std::snprintf(mKeysText, sizeof(mKeysText), "%srt%lu", codes,
                  static_cast<unsigned long>(mRoundTrip % 10000u));
    lv_label_set_text_static(mKeys, mKeysText);
}
