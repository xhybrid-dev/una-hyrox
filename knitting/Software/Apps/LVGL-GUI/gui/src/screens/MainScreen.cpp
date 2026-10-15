/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Knitting' one screen: rows in this set, the set, the total.
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    // Centred rows, clear of the bezel button hints at the corners.
    mRows  = Draw::label(mRoot, &poppins_light_60, "0", 20, 52, 200);
    mSet   = Draw::label(mRoot, &poppins_semibold_25, "Set 1", 20, 128, 200);
    mTotal = Draw::label(mRoot, &poppins_regular_18, "Total 0", 20, 170, 200,
                         LV_TEXT_ALIGN_CENTER, Color::GRAY);

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::WHITE,    // next set
                  SDK::LVGL::Buttons::AMBER,    // reset
                  SDK::LVGL::Buttons::WHITE,    // count a row
                  SDK::LVGL::Buttons::WHITE);   // back

    bind(&mModel);
    mModel.bind(this);

    // The service sends the counter as soon as the GUI starts; until then
    // draw what the model has.
    show(mModel.counter());
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
    // Only clicks act: a press or release code must not count a row twice.
    switch (code) {
        case Btn::R1:
            mModel.countRow();
            break;
        case Btn::L1:
            mModel.nextSet();
            break;
        case Btn::L2:
            // K0: an immediate reset. K2 replaces it with hold-to-confirm.
            mModel.resetAll();
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            break;
    }
}

void MainScreen::onCounterUpdate(const Knitting::CounterState& state)
{
    show(state);
}

void MainScreen::show(const Knitting::CounterState& state)
{
    // Integer formatting only; the buffers are sized for the limits in
    // RowCounter.hpp (3 digits, 3 digits, 5 digits).
    char text[24];

    std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(state.rows));
    lv_label_set_text(mRows, text);

    std::snprintf(text, sizeof(text), "Set %lu", static_cast<unsigned long>(state.set));
    lv_label_set_text(mSet, text);

    std::snprintf(text, sizeof(text), "Total %lu", static_cast<unsigned long>(state.total));
    lv_label_set_text(mTotal, text);
}
