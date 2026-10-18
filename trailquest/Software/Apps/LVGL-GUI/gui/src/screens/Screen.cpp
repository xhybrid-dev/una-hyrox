/**
 ******************************************************************************
 * @file    Screen.cpp
 * @brief   Base class of every Trail Quest screen (see Screen.hpp).
 ******************************************************************************
 */

#include "gui/screens/Screen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;
namespace Btn  = SDK::GUI::Button;

namespace
{
// R2's bezel arc runs 112-128 degrees (SDK Buttons.cpp); the leave arc grows
// over and past it, clockwise, as R2 is held.
constexpr int32_t kLeaveFrom   = 100;
constexpr int32_t kLeaveSweep  = 50;
constexpr int32_t kLeaveRadius = 110;
} // namespace

Screen::Screen(Model& model)
    : mModel(model)
{
}

Screen::~Screen()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
    lv_display_remove_event_cb_with_user_data(lv_display_get_default(), &Screen::renderCb, this);
    if (mRoot) {
        lv_obj_delete(mRoot);
    }
}

void Screen::createRoot()
{
    if (mRoot) {
        return;
    }
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &Screen::keyEventCb, LV_EVENT_KEY, this);
}

void Screen::create()
{
    createRoot();
    build();
    mButtons.emplace(mRoot);
    mLeaveArc = Draw::arc(mRoot, SDK::LVGL::kCx, SDK::LVGL::kCy, kLeaveRadius, 8, kLeaveFrom, kLeaveFrom + 1, 0xC00000);
    Draw::setHidden(mLeaveArc, true);
    mDev = Draw::label(mRoot, &poppins_regular_14, "", 40, 214, 160, LV_TEXT_ALIGN_CENTER, 0x808080);
    lv_label_set_text_static(mDev, mDevText);
    Draw::setHidden(mDev, true);
    // Time-driven, so a period of 1 ms: one run per kernel tick (lv_conf.h).
    mTimer = lv_timer_create(&Screen::tickCb, 1, this);
    lv_display_add_event_cb(lv_display_get_default(), &Screen::renderCb, LV_EVENT_RENDER_START, this);
    lv_display_add_event_cb(lv_display_get_default(), &Screen::renderCb, LV_EVENT_RENDER_READY, this);
}

void Screen::setArcs(SDK::LVGL::Buttons::Color l1, SDK::LVGL::Buttons::Color l2, SDK::LVGL::Buttons::Color r1,
                     SDK::LVGL::Buttons::Color r2)
{
    if (mButtons) {
        mButtons->set(l1, l2, r1, r2);
    }
}

void Screen::onSuspend()
{
    // The release will never come: drop the hold rather than leave on return.
    mLeave.cancel();
    if (mLeaveArc) {
        Draw::setHidden(mLeaveArc, true);
    }
}

void Screen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<Screen*>(lv_event_get_user_data(e));
    self->handleKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void Screen::tickCb(lv_timer_t* t)
{
    static_cast<Screen*>(lv_timer_get_user_data(t))->frame();
}

void Screen::renderCb(lv_event_t* e)
{
    auto* self = static_cast<Screen*>(lv_event_get_user_data(e));
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

namespace
{
const uint8_t kPress[4]   = { Btn::L1_PRESS, Btn::L2_PRESS, Btn::R1_PRESS, Btn::R2_PRESS };
const uint8_t kClick[4]   = { Btn::L1, Btn::L2, Btn::R1, Btn::R2 };
const uint8_t kRelease[4] = { Btn::L1_RELEASE, Btn::L2_RELEASE, Btn::R1_RELEASE, Btn::R2_RELEASE };

/// 0-3 for L1, L2, R1, R2 if @p code is in @p codes, else -1.
int buttonOf(uint8_t code, const uint8_t (&codes)[4])
{
    for (int i = 0; i < 4; ++i) {
        if (code == codes[i]) {
            return i;
        }
    }
    return -1;
}
} // namespace

void Screen::handleKey(uint8_t code)
{
    const uint32_t now = lv_tick_get();
    if (code == Btn::L1R2) {
        Draw::setHidden(mDev, !lv_obj_has_flag(mDev, LV_OBJ_FLAG_HIDDEN));
        updateDev(now);
        return;
    }
    if (rawKeys()) {
        // The beat: every press now. Its clicks, which follow, are not taps.
        for (bool& a : mArmed) {
            a = false;
        }
        mLeave.cancel();
        Draw::setHidden(mLeaveArc, true);
        onKey(code);
        return;
    }
    const int pressed = buttonOf(code, kPress);
    if (pressed >= 0) {
        mArmed[pressed] = true;
    }
    const int clicked = buttonOf(code, kClick);
    if (clicked >= 0) {
        if (!mArmed[clicked]) {
            return;   // the tail of a press this screen did not see
        }
        mArmed[clicked] = false;
    }
    // A slow press may send no click (the simulator sends none at 500 ms or
    // more): its release is the tap then. R2's is the hold timer's.
    const int released = buttonOf(code, kRelease);
    if (released >= 0 && released != 3) {
        const bool tap = mArmed[released];
        mArmed[released] = false;
        onKey(code);
        if (tap) {
            onKey(kClick[released]);
        }
        return;
    }
    switch (code) {
        case Btn::R2_PRESS:
            mLeave.press(now);
            return;
        case Btn::R2_RELEASE:
            Draw::setHidden(mLeaveArc, true);
            mArmed[3] = false;
            if (mLeave.release(now)) {
                onKey(Btn::R2);
            }
            return;
        case Btn::R2:
            if (mLeave.takeClick(now)) {
                onKey(Btn::R2);
            }
            return;
        default:
            onKey(code);
            return;
    }
}

void Screen::frame()
{
    const uint32_t now = lv_tick_get();
    mModel.keepAlive(now);
    if (mLeave.pending()) {
        const int32_t sweep = static_cast<int32_t>(mLeave.progressPermille(now) * kLeaveSweep / 1000u);
        Draw::setArc(mLeaveArc, kLeaveFrom, kLeaveFrom + (sweep > 1 ? sweep : 1));
        Draw::setHidden(mLeaveArc, false);
    }
    if (mLeave.poll(now)) {
        // Everything was saved when the last step ended; a step in progress
        // (a duel, an obstacle) is walked again next time.
        mModel.exitApp();
        return;
    }
    if (!lv_obj_has_flag(mDev, LV_OBJ_FLAG_HIDDEN) && now - mDevAt >= 1000) {
        updateDev(now);
    }
    onFrame(now);
}

void Screen::updateDev(uint32_t nowMs)
{
    mDevAt = nowMs;
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    const uint32_t used = mon.total_size - mon.free_size;
    std::snprintf(mDevText, sizeof(mDevText), "r %lu/%lums f%lu p%lu%%",
                  static_cast<unsigned long>(mRenderLast % 1000u), static_cast<unsigned long>(mRenderMax % 1000u),
                  static_cast<unsigned long>(mFrames % 100u),
                  static_cast<unsigned long>(mon.total_size ? used * 100u / mon.total_size : 0u));
    lv_label_set_text_static(mDev, mDevText);
    lv_obj_move_foreground(mDev);
    mFrames    = 0;
    mRenderMax = 0;
}
