/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Knitting's one screen: rows in this set, the set, the total, and
 *          the two holds (see the header).
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;

namespace
{
constexpr int32_t kBarX = 60;
constexpr int32_t kBarY = 193;
constexpr int32_t kBarW = 120;
constexpr int32_t kBarH = 6;
constexpr uint32_t kTickMs = 40;

// The button labels' boxes (x, y, width), in 240x240 display coordinates.
constexpr int32_t kLabelLeftX   = 44;
constexpr int32_t kLabelRightX  = 106;
constexpr int32_t kLabelW       = 90;
constexpr int32_t kLabelTopY    = 30;
constexpr int32_t kLabelStep    = 16;
constexpr int32_t kLabelBottomY = 203;
constexpr int32_t kLabelBottomX = 60;      // the display is round: further left is cut off
} // namespace

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    // What each button does, in the corner beside its bezel button: white for a
    // tap, amber for a hold (the colour of the hold's own prompt and bar).
    // L1 and R1 are the top corners, L2 the bottom left.
    Draw::label(mRoot, &poppins_regular_14, "New set", kLabelLeftX, kLabelTopY, kLabelW,
                LV_TEXT_ALIGN_LEFT, Color::WHITE);
    Draw::label(mRoot, &poppins_regular_14, "Knit row", kLabelRightX, kLabelTopY, kLabelW,
                LV_TEXT_ALIGN_RIGHT, Color::WHITE);
    Draw::label(mRoot, &poppins_regular_14, "Hold: undo", kLabelRightX, kLabelTopY + kLabelStep, kLabelW,
                LV_TEXT_ALIGN_RIGHT, Color::YELLOW_DARK);
    Draw::label(mRoot, &poppins_regular_14, "Hold: reset", kLabelBottomX, kLabelBottomY, kLabelW,
                LV_TEXT_ALIGN_LEFT, Color::YELLOW_DARK);

    // The numbers, centred and clear of the labels.
    mRows   = Draw::label(mRoot, &poppins_light_60, "0", 20, 62, 200);
    mSet    = Draw::label(mRoot, &poppins_semibold_25, "Set 1", 20, 130, 200);
    mStatus = Draw::label(mRoot, &poppins_regular_18, "Total 0", 20, 167, 200,
                          LV_TEXT_ALIGN_CENTER, Color::GRAY);

    // The progress of a hold, under the bottom line. Hidden until one starts.
    mBarTrack = Draw::box(mRoot, kBarX, kBarY, kBarW, kBarH, Color::GRAY_DARK, LV_RADIUS_CIRCLE);
    mBarFill  = Draw::box(mRoot, kBarX, kBarY, 1, kBarH, Color::YELLOW_DARK, LV_RADIUS_CIRCLE);
    Draw::setHidden(mBarTrack, true);
    Draw::setHidden(mBarFill, true);

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::WHITE,    // next set
                  SDK::LVGL::Buttons::AMBER,    // hold to reset
                  SDK::LVGL::Buttons::WHITE,    // count a row; hold to undo
                  SDK::LVGL::Buttons::WHITE);   // back

    bind(&mModel);
    mModel.bind(this);

    // The service sends the counter as soon as the GUI starts; until then
    // draw what the model has.
    show(mModel.counter());
}

MainScreen::~MainScreen()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void MainScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<MainScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void MainScreen::tickCb(lv_timer_t* t)
{
    static_cast<MainScreen*>(lv_timer_get_user_data(t))->tick();
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    const uint32_t now = lv_tick_get();

    switch (code) {
        // R1: a tap counts a row; a hold takes one back. A tap is the click or,
        // for a slow press that sends none, the release; HoldTimer counts
        // whichever comes first, once, and never after a hold that fired.
        case Btn::R1_PRESS:
            mHasNotice = false;
            mUndo.press(now);
            break;
        case Btn::R1_RELEASE:
            if (mUndo.release(now)) {
                mModel.countRow();
            }
            break;
        case Btn::R1:
            if (mUndo.takeClick(now)) {
                mModel.countRow();
            }
            break;

        case Btn::L1:
            mHasNotice = false;
            mModel.nextSet();
            break;

        // L2: only a hold resets. A tap says how.
        case Btn::L2_PRESS:
            mHasNotice = false;
            mReset.press(now);
            break;
        case Btn::L2_RELEASE:
            if (mReset.release(now)) {
                showNotice("Hold to reset", now, Color::YELLOW_DARK);
            }
            break;
        case Btn::L2:
            if (mReset.takeClick(now)) {
                showNotice("Hold to reset", now, Color::YELLOW_DARK);
            }
            break;

        case Btn::R2:
            mModel.exitApp();
            return;

        default:
            return;
    }

    refreshStatus(now);
    updateTimer(now);
}

void MainScreen::tick()
{
    const uint32_t now = lv_tick_get();

    if (mReset.poll(now)) {
        mModel.resetAll();
        showNotice("Reset", now, Color::CHARTREUSE);
    }
    if (mUndo.poll(now)) {
        mModel.undoRow();
        showNotice("Row undone", now, Color::CHARTREUSE);
    }
    if (mHasNotice && now - mNoticeAt >= kNoticeMs) {
        mHasNotice = false;
    }

    refreshStatus(now);
    updateTimer(now);
}

void MainScreen::showNotice(const char* text, uint32_t nowMs, uint32_t color)
{
    std::strncpy(mNotice, text, sizeof(mNotice) - 1);
    mNotice[sizeof(mNotice) - 1] = '\0';
    mHasNotice   = true;
    mNoticeAt    = nowMs;
    mNoticeColor = color;
}

void MainScreen::updateTimer(uint32_t nowMs)
{
    (void)nowMs;
    const bool needed = mReset.pending() || mUndo.pending() || mHasNotice;
    if (needed && !mTimer) {
        mTimer = lv_timer_create(&MainScreen::tickCb, kTickMs, this);
    } else if (!needed && mTimer) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
}

void MainScreen::onSuspend()
{
    // The release cannot reach this screen while the GUI is suspended, so a
    // hold left running could complete by itself on resume. Drop it.
    mReset.cancel();
    mUndo.cancel();
    mHasNotice = false;
    const uint32_t now = lv_tick_get();
    refreshStatus(now);
    updateTimer(now);
}

void MainScreen::onCounterUpdate(const Knitting::CounterState& state)
{
    show(state);
}

void MainScreen::show(const Knitting::CounterState& state)
{
    mCounter = state;

    // Integer formatting only; the buffers are sized for the limits in
    // RowCounter.hpp (3 digits, 3 digits).
    char text[24];

    std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(state.rows));
    lv_label_set_text(mRows, text);

    std::snprintf(text, sizeof(text), "Set %lu", static_cast<unsigned long>(state.set));
    lv_label_set_text(mSet, text);

    refreshStatus(lv_tick_get());
}

void MainScreen::refreshStatus(uint32_t nowMs)
{
    char     text[32];
    uint32_t color = Color::GRAY;
    uint32_t progress = 0;       // 0..1000, 0 = no bar

    if (mHasNotice) {
        std::snprintf(text, sizeof(text), "%s", mNotice);
        color = mNoticeColor;
    } else if (mReset.pending()) {
        std::snprintf(text, sizeof(text), "Hold to reset");
        color    = Color::YELLOW_DARK;
        progress = mReset.progressPermille(nowMs);
    } else if (mUndo.pending()) {
        std::snprintf(text, sizeof(text), "Hold to undo");
        color    = Color::YELLOW_DARK;
        progress = mUndo.progressPermille(nowMs);
    } else {
        std::snprintf(text, sizeof(text), "Total %lu", static_cast<unsigned long>(mCounter.total));
    }

    lv_label_set_text(mStatus, text);
    lv_obj_set_style_text_color(mStatus, Draw::rgb(color), LV_PART_MAIN);

    const bool showBar = progress > 0;
    Draw::setHidden(mBarTrack, !showBar);
    Draw::setHidden(mBarFill, !showBar);
    if (showBar) {
        int32_t w = static_cast<int32_t>(progress) * kBarW / 1000;
        if (w < kBarH) {
            w = kBarH;     // a pill narrower than it is tall draws badly
        }
        lv_obj_set_width(mBarFill, w);
    }
}
