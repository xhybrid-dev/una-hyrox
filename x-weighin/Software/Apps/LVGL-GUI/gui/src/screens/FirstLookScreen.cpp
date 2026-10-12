/**
 ******************************************************************************
 * @file    FirstLookScreen.cpp
 * @brief   W0's one screen: what the watch knows about the athlete.
 *
 * Laid out like the SDK's Waypoint screen (centred rows inside the round
 * display), with its fonts. Everything is formatted from integers.
 ******************************************************************************
 */

#include "gui/screens/FirstLookScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "WeighText.hpp"
#include "gui/Assets.hpp"

namespace Draw  = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;

FirstLookScreen::FirstLookScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &FirstLookScreen::keyEventCb, LV_EVENT_KEY, this);

    // Rows narrow towards the top and bottom, where the round display does,
    // and the status line ends left of the R2 marker (right edge, y 160-192).
    mTitle  = Draw::label(mRoot, &poppins_semibold_20, "X Weigh-in", 40, 28, 160,
                          LV_TEXT_ALIGN_CENTER, Color::LIME);
    mHeight = Draw::label(mRoot, &poppins_regular_18, "", 20, 62, 200);
    mWeight = Draw::label(mRoot, &poppins_regular_18, "", 20, 90, 200);
    mRestHr = Draw::label(mRoot, &poppins_regular_18, "", 20, 118, 200);
    mAge    = Draw::label(mRoot, &poppins_regular_18, "", 20, 146, 200);
    mStatus = Draw::label(mRoot, &poppins_regular_18, "", 30, 180, 180,
                          LV_TEXT_ALIGN_CENTER, Color::GRAY);

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::NONE, SDK::LVGL::Buttons::NONE,
                  SDK::LVGL::Buttons::NONE,
                  SDK::LVGL::Buttons::WHITE);   // back

    bind(&mModel);
    mModel.bind(this);

    // The service sends a first look as soon as the GUI starts; until then
    // draw what the model has (all "not set").
    show(mModel.look());
}

FirstLookScreen::~FirstLookScreen()
{
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void FirstLookScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<FirstLookScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void FirstLookScreen::onKey(uint8_t code)
{
    if (code == SDK::GUI::Button::R2) {
        mModel.exitApp();
    }
}

void FirstLookScreen::onFirstLook(const CustomMessage::FirstLookState& look)
{
    show(look);
}

void FirstLookScreen::show(const CustomMessage::FirstLookState& look)
{
    char text[40];
    char kg[16];

    if (look.heightCm != 0) {
        std::snprintf(text, sizeof(text), "Height %lu cm",
                      static_cast<unsigned long>(look.heightCm));
        lv_label_set_text(mHeight, text);
    } else {
        lv_label_set_text(mHeight, "Height not set");
    }

    // Two decimals, so the profile's resolution shows (W0's question).
    if (look.weightHundredths > 0) {
        XWeighIn::formatKg(look.weightHundredths, 2, kg, sizeof(kg));
        std::snprintf(text, sizeof(text), "Weight %s kg", kg);
        lv_label_set_text(mWeight, text);
    } else {
        lv_label_set_text(mWeight, "Weight not set");
    }

    if (look.restHr != 0) {
        std::snprintf(text, sizeof(text), "Resting HR %u", static_cast<unsigned>(look.restHr));
        lv_label_set_text(mRestHr, text);
    } else {
        lv_label_set_text(mRestHr, "Resting HR --");
    }

    const char* sex = XWeighIn::sexLabel(look.sex);
    if (look.ageYears != 0) {
        if (sex != nullptr) {
            std::snprintf(text, sizeof(text), "Age %u, %s", static_cast<unsigned>(look.ageYears), sex);
        } else {
            std::snprintf(text, sizeof(text), "Age %u", static_cast<unsigned>(look.ageYears));
        }
        lv_label_set_text(mAge, text);
    } else if (!look.clockSet && look.settingsRead) {
        lv_label_set_text(mAge, "Age: clock not set");
    } else {
        // The status line says where to set it.
        lv_label_set_text(mAge, "Age not set");
    }

    // The status line names the first thing missing.
    if (!look.settingsRead) {
        lv_label_set_text(mStatus, "No profile yet");
    } else if (!look.configLoaded) {
        lv_label_set_text(mStatus, "No phone settings");
    } else {
        lv_label_set_text(mStatus, "First look");
    }
}
