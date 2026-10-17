/**
 ******************************************************************************
 * @file    ProbeScreen.cpp
 * @brief   The Q0 feasibility screen (see ProbeScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/ProbeScreen.hpp"

#include <cstdio>
#include <cstring>

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
using namespace XTrailQuest;

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

int32_t trackX(uint32_t ms, uint32_t sweepMs)
{
    return kTrackX + static_cast<int32_t>(ms * static_cast<uint32_t>(kTrackW) / sweepMs);
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
    const int32_t goodX = trackX(mBeat.targetMs - mBeat.goodMs / 2, mBeat.sweepMs);
    const int32_t goodW = static_cast<int32_t>(mBeat.goodMs * static_cast<uint32_t>(kTrackW) / mBeat.sweepMs);
    const int32_t perfX = trackX(mBeat.targetMs - mBeat.perfectMs / 2, mBeat.sweepMs);
    const int32_t perfW = static_cast<int32_t>(mBeat.perfectMs * static_cast<uint32_t>(kTrackW) / mBeat.sweepMs);
    Draw::box(mRoot, goodX, kTrackY, goodW, kTrackH, Color::YELLOW_DARK);
    Draw::box(mRoot, perfX, kTrackY, perfW, kTrackH, Color::GREEN);
    mNeedle = Draw::box(mRoot, kTrackX, kTrackY - 3, kNeedleW, kTrackH + 6, Color::WHITE);

    mResult = Draw::label(mRoot, &pressstart2p_16, "R1: BEAT", 20, 180, 200, LV_TEXT_ALIGN_CENTER, Color::WHITE);
    mStats  = Draw::label(mRoot, &poppins_regular_14, "", 35, 200, 170, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mKeys   = Draw::label(mRoot, &poppins_regular_14, "", 60, 217, 120, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mFxName = Draw::label(mRoot, &poppins_regular_14, fxPattern(static_cast<FxId>(mFx)).name, 60, 44, 120, LV_TEXT_ALIGN_CENTER, Color::WHITE);

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

    // Show the last three codes, in arrival order, with the time since the
    // beat press so the click/press/release lag is visible.
    const size_t len = std::strlen(mKeyText);
    if (len >= 5) {
        std::memmove(mKeyText, mKeyText + 2, len - 1);   // drop the oldest "x "
    }
    std::snprintf(mKeyText + std::strlen(mKeyText), sizeof(mKeyText) - std::strlen(mKeyText), "%c ", static_cast<char>(code));
    updateKeys();
    LOG_INFO("key '%c' at %lu\n", static_cast<char>(code), static_cast<unsigned long>(lv_tick_get()));

    switch (code) {
        case Btn::R1_PRESS: {          // the earliest code for R1
            const uint32_t now = lv_tick_get();
            mPressTick = now;
            mModel.sendBeatPress(++mSeq, mBeatStart, now);
        } break;
        case Btn::L1:
            mModel.playFx(static_cast<FxId>(mFx));
            break;
        case Btn::L2:
            mFx = static_cast<uint8_t>((mFx + 1u) % static_cast<uint8_t>(FxId::Count));
            lv_label_set_text(mFxName, fxPattern(static_cast<FxId>(mFx)).name);
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
    mRoundTrip = lv_tick_get() - mPressTick;
    updateKeys();
    static const char* const kNames[] = { "MISS", "GOOD", "PERFECT" };
    char text[40];
    std::snprintf(text, sizeof(text), "%s %+ld",
                  kNames[r.result <= 2 ? r.result : 0], static_cast<long>(r.offsetMs));
    lv_label_set_text(mResult, text);
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

    // The needle.
    const uint32_t ms = beatPhaseMs(mBeat, mBeatStart, now);
    lv_obj_set_x(mNeedle, trackX(ms, mBeat.sweepMs) - kNeedleW / 2);

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
    char text[64];
    std::snprintf(text, sizeof(text), "r %lu/%lums  f%lu  p%lu%%",
                  static_cast<unsigned long>(mRenderLast), static_cast<unsigned long>(mRenderMax),
                  static_cast<unsigned long>(mFrames),
                  static_cast<unsigned long>(used * 100u / mon.total_size));
    lv_label_set_text(mStats, text);
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
    char text[40];
    std::snprintf(text, sizeof(text), "%s rt%lu", mKeyText, static_cast<unsigned long>(mRoundTrip));
    lv_label_set_text(mKeys, text);
}
