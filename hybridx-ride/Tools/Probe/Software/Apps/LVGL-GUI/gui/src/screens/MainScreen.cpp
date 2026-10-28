/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   The bike probe's one screen (see the header).
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "ProbeText.hpp"
#include "gui/Assets.hpp"

namespace Draw  = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;

namespace
{
// The log's lines: narrower towards the top and bottom of the round face.
constexpr int32_t kLogY    = 78;
constexpr int32_t kLogStep = 20;
constexpr int32_t kLogX[MainScreen::kShown] = {30, 22, 20, 20, 22, 30};
} // namespace

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    Draw::label(mRoot, &poppins_semibold_20, "BIKE PROBE", 20, 16, 200);
    mNext = Draw::label(mRoot, &poppins_regular_14, "", 30, 40, 180, LV_TEXT_ALIGN_CENTER, Color::YELLOW_DARK);
    mHr   = Draw::label(mRoot, &poppins_regular_14, "", 30, 58, 180, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    for (uint8_t i = 0; i < kShown; ++i) {
        mLog[i] = Draw::label(mRoot, &poppins_regular_14, "", kLogX[i], kLogY + i * kLogStep,
                              240 - 2 * kLogX[i], LV_TEXT_ALIGN_LEFT, Color::WHITE);
    }
    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::WHITE, SDK::LVGL::Buttons::WHITE,
                  SDK::LVGL::Buttons::AMBER, SDK::LVGL::Buttons::WHITE);

    mModel.bind(this);
    onState();
    drawLog();
}

MainScreen::~MainScreen()
{
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void MainScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<MainScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::R1:
            mScroll = 0;
            mModel.nextRequest();
            break;
        case Btn::L1:   // newer
            if (mScroll > 0) {
                --mScroll;
                drawLog();
            }
            break;
        case Btn::L2:   // older
            if (mScroll + kShown < mModel.lineCount()) {
                ++mScroll;
                drawLog();
            }
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            break;
    }
}

void MainScreen::onLog()
{
    mScroll = 0;   // a new line: back to the newest
    drawLog();
}

void MainScreen::onState()
{
    const CustomMessage::ProbeState& s = mModel.state();
    char t[48];
    std::snprintf(t, sizeof(t), "R1: %s", RideProbe::kSteps[s.next % RideProbe::kStepCount].label);
    lv_label_set_text(mNext, t);
    // HeartRateEx's source: 0 none, 1 the wrist, 2 an external strap.
    static const char* const kSrc[] = {"none", "wrist", "strap"};
    const char* src = s.hrSource < 3 ? kSrc[s.hrSource] : "?";
    if (s.extBpm) {
        std::snprintf(t, sizeof(t), "HR %s, strap %u bpm", src, static_cast<unsigned>(s.extBpm));
    } else {
        std::snprintf(t, sizeof(t), "HR %s", src);
    }
    lv_label_set_text(mHr, s.logging ? t : "CSV not written");
}

void MainScreen::drawLog()
{
    // Oldest at the top, newest at the bottom.
    for (uint8_t i = 0; i < kShown; ++i) {
        const uint8_t back = static_cast<uint8_t>(mScroll + (kShown - 1 - i));
        lv_label_set_text(mLog[i], mModel.line(back));
    }
}
