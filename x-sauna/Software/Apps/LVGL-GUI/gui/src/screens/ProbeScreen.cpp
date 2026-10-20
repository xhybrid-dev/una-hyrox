/**
 ******************************************************************************
 * @file    ProbeScreen.cpp
 * @brief   The Gate 0 heat probe's screen (see the header).
 ******************************************************************************
 */

#include "gui/screens/ProbeScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "FixedPoint.hpp"
#include "ProbeStats.hpp"

#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;

namespace
{
/// The colour for one sensor's state: green when samples are arriving, amber
/// while waiting for the first, red when the sensor is not offered.
uint32_t senseColor(uint8_t sense)
{
    switch (static_cast<Sauna::Sense>(sense)) {
    case Sauna::Sense::Ok:           return Color::GREEN;
    case Sauna::Sense::ConnectFailed: return Color::RED;
    default:                         return Color::YELLOW_DARK;
    }
}

/// The big temperature stays white while it is being read, so it is not
/// mistaken for a verdict; blue marks a made-up one (the simulator).
uint32_t tempColor(const Sauna::ProbeSnapshot& s)
{
    if (s.flags & Sauna::kFlagSimulated) {
        return Color::SKY;
    }
    return static_cast<Sauna::Sense>(s.tempSense) == Sauna::Sense::Ok ? Color::WHITE : senseColor(s.tempSense);
}
} // namespace

ProbeScreen::ProbeScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &ProbeScreen::keyEventCb, LV_EVENT_KEY, this);

    // Centred, clear of the bezel button hints at the corners.
    mTitle   = Draw::label(mRoot, &poppins_semibold_20, "HEAT PROBE", 40, 20, 160,
                           LV_TEXT_ALIGN_CENTER, Color::YELLOW_DARK);
    mTemp    = Draw::label(mRoot, &poppins_light_60, "--", 20, 44, 200);
    mCaption = Draw::label(mRoot, &poppins_regular_14, "temperature sensor, raw", 20, 108, 200,
                           LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mRange   = Draw::label(mRoot, &poppins_regular_18, "min -- max --", 20, 126, 200);
    mHr      = Draw::label(mRoot, &poppins_semibold_25, "HR --", 24, 150, 110,
                           LV_TEXT_ALIGN_LEFT);
    // Right edge at 196: the R2 bezel arc starts at about x = 205 on this row.
    mTrust   = Draw::label(mRoot, &poppins_regular_14, "trust --", 116, 160, 80,
                           LV_TEXT_ALIGN_RIGHT, Color::GRAY);
    mRate    = Draw::label(mRoot, &poppins_regular_14, "", 20, 182, 200,
                           LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mStatus  = Draw::label(mRoot, &poppins_regular_14, "", 30, 200, 180,
                           LV_TEXT_ALIGN_CENTER, Color::GRAY);

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::WHITE,    // clear the range
                  SDK::LVGL::Buttons::NONE,
                  SDK::LVGL::Buttons::AMBER,    // mark
                  SDK::LVGL::Buttons::WHITE);   // back

    bind(&mModel);
    mModel.bind(this);

    // The service sends its readings as soon as the GUI starts; until then
    // draw what the model has.
    show(mModel.snapshot());
}

ProbeScreen::~ProbeScreen()
{
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void ProbeScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<ProbeScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void ProbeScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;

    switch (code) {
        case Btn::R1:
            mModel.mark();
            break;
        case Btn::L1:
            mModel.clearRange();
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            break;
    }
}

void ProbeScreen::onProbeUpdate(const Sauna::ProbeSnapshot& snapshot)
{
    show(snapshot);
}

void ProbeScreen::show(const Sauna::ProbeSnapshot& s)
{
    // Integer formatting only; the buffers hold the longest text each line
    // can take (a 32-bit count is 10 digits).
    char text[48];
    char a[16];
    char b[16];

    // The big number, one decimal: a sensor's last digit means little.
    Sauna::formatX100(text, sizeof(text), s.tempX100, 1);
    lv_label_set_text(mTemp, text);
    lv_obj_set_style_text_color(mTemp, Draw::rgb(tempColor(s)), LV_PART_MAIN);

    if (s.flags & Sauna::kFlagSimulated) {
        lv_label_set_text(mCaption, "made up (simulator)");
    } else if (static_cast<Sauna::Sense>(s.tempSense) == Sauna::Sense::ConnectFailed) {
        lv_label_set_text(mCaption, "no temperature sensor");
    } else {
        lv_label_set_text(mCaption, "temperature sensor, raw");
    }

    Sauna::formatX100(a, sizeof(a), s.tempMinX100, 1);
    Sauna::formatX100(b, sizeof(b), s.tempMaxX100, 1);
    std::snprintf(text, sizeof(text), "min %s  max %s", a, b);
    lv_label_set_text(mRange, text);

    if (s.hrSamples == 0) {
        std::snprintf(text, sizeof(text), "HR --");
    } else {
        std::snprintf(text, sizeof(text), "HR %u", static_cast<unsigned>(s.hr));
    }
    lv_label_set_text(mHr, text);
    lv_obj_set_style_text_color(mHr, Draw::rgb(senseColor(s.hrSense)), LV_PART_MAIN);

    Sauna::formatX100(a, sizeof(a), s.trustX100, 1);
    std::snprintf(text, sizeof(text), "trust %s", a);
    lv_label_set_text(mTrust, text);

    if (static_cast<Sauna::Sense>(s.tempSense) == Sauna::Sense::Ok || (s.flags & Sauna::kFlagSimulated)) {
        if (s.tempSamples > 1) {
            std::snprintf(text, sizeof(text), "n %lu  every %lu ms", static_cast<unsigned long>(s.tempSamples),
                          static_cast<unsigned long>(s.tempGapMs));
        } else {
            std::snprintf(text, sizeof(text), "n %lu", static_cast<unsigned long>(s.tempSamples));
        }
    } else {
        std::snprintf(text, sizeof(text), "temp: %s", Sauna::senseName(static_cast<Sauna::Sense>(s.tempSense)));
    }
    lv_label_set_text(mRate, text);

    // Every variant is 21 characters or so, which the round face holds at this
    // row (about 186 px of chord for Regular 14).
    const char* log = "log --";
    switch (static_cast<Sauna::LogState>(s.log)) {
        case Sauna::LogState::Ok:     log = "log ok"; break;
        case Sauna::LogState::Failed: log = "NO LOG"; break;
        default:                      break;
    }
    std::snprintf(text, sizeof(text), "%02lu:%02lu  mark %u  %s",
                  static_cast<unsigned long>(s.elapsedS / 60u), static_cast<unsigned long>(s.elapsedS % 60u),
                  static_cast<unsigned>(s.marks), log);
    lv_label_set_text(mStatus, text);

    // The sensors stop after 45 minutes; the title says so.
    const bool finished = (s.flags & Sauna::kFlagFinished) != 0;
    lv_label_set_text(mTitle, finished ? "PROBE DONE" : "HEAT PROBE");
    lv_obj_set_style_text_color(mTitle, Draw::rgb(finished ? Color::GREEN : Color::YELLOW_DARK), LV_PART_MAIN);
    lv_obj_set_style_text_color(
        mStatus, Draw::rgb(static_cast<Sauna::LogState>(s.log) == Sauna::LogState::Failed ? Color::RED : Color::GRAY),
        LV_PART_MAIN);
}
