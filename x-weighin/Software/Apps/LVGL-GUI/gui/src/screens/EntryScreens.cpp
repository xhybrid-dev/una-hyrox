/**
 ******************************************************************************
 * @file    EntryScreens.cpp
 * @brief   Weighing in: the entry screen and the saved confirmation.
 *
 * The entry opens on the last logged weight (else the UNA profile's, else
 * 70.0). UP and DOWN move it 0.1 kg a tap; held, they repeat and speed up
 * (XWeighIn::HoldRepeat, after the SDK Timer example's editor). SELECT saves,
 * BACK cancels. The service files the weigh-in under the local date and time
 * and answers with SAVE_RESULT; the Saved screen shows it for three seconds.
 ******************************************************************************
 */

#include "gui/screens/Screens.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "DateText.hpp"
#include "WeighText.hpp"
#include "gui/Assets.hpp"

namespace Draw  = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;
using namespace XWeighIn;

namespace
{
/// How often a held button is checked, ms.
constexpr uint32_t kTickMs = 50;
/// How long the Saved screen stays up, ms.
constexpr uint32_t kSavedMs = 3000;
}  // namespace

// -- Weigh in ------------------------------------------------------------------------

WeighInScreen::~WeighInScreen()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
}

void WeighInScreen::build()
{
    mTitle = Draw::label(mRoot, &poppins_semibold_20, "Weigh in", 40, 26, 160, LV_TEXT_ALIGN_CENTER, Color::LIME);
    mValue = Draw::label(mRoot, &poppins_light_60, "", 20, 64, 200);
    mUnit  = Draw::label(mRoot, &poppins_regular_18, "kg", 40, 130, 160, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mHint  = Draw::label(mRoot, &poppins_regular_18, "", 20, 160, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::WHITE,    // UP: +0.1 kg
                  SDK::LVGL::Buttons::WHITE,    // DOWN: -0.1 kg
                  SDK::LVGL::Buttons::GREEN,    // SELECT: save
                  SDK::LVGL::Buttons::WHITE);   // BACK: cancel
}

void WeighInScreen::onShow()
{
    const bool have = mModel.hasHome();
    const CustomMessage::HomeData& h = mModel.home();
    mValueH = have ? clampEntry(h.entryStart) : kEntryFallback;

    char text[48];   // "Last " + 15 + ", " + 23, terminated
    switch (have ? static_cast<CustomMessage::EntryFrom>(h.entryFrom) : CustomMessage::EntryFrom::Default) {
        case CustomMessage::EntryFrom::Log: {
            char kg[16];
            char when[24];
            formatKg(h.latest, 1, kg, sizeof(kg));
            if (h.today != 0) {
                agoText(h.today, h.lastDay, when, sizeof(when));
            } else {
                formatShortDate(h.lastDay, when, sizeof(when));
            }
            std::snprintf(text, sizeof(text), "Last %s, %s", kg, when);
        } break;
        case CustomMessage::EntryFrom::Profile:
            std::snprintf(text, sizeof(text), "From your UNA profile");
            break;
        default:
            std::snprintf(text, sizeof(text), "UP and DOWN to set");
            break;
    }
    lv_label_set_text(mHint, text);
    show();

    mTimer = lv_timer_create(&WeighInScreen::tickCb, kTickMs, this);
}

void WeighInScreen::show()
{
    char text[16];
    formatKg(mValueH, 1, text, sizeof(text));
    lv_label_set_text(mValue, text);
}

void WeighInScreen::step(int32_t steps)
{
    const int32_t next = stepEntry(mValueH, steps);
    if (next != mValueH) {
        mValueH = next;
        show();
    }
}

void WeighInScreen::tickCb(lv_timer_t* t)
{
    auto* self = static_cast<WeighInScreen*>(lv_timer_get_user_data(t));
    if (self->mSaving || self->mHeldDir == 0) {
        return;
    }
    const uint32_t steps = self->mHold.tick(lv_tick_get());
    if (steps > 0) {
        self->step(self->mHeldDir * static_cast<int32_t>(steps));
    }
}

void WeighInScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (mSaving) {
        return;   // waiting for the service's answer
    }
    const uint32_t now = lv_tick_get();
    switch (code) {
        case Btn::L1_PRESS:
            mHeldDir = 1;
            mHold.press(now);
            break;
        case Btn::L2_PRESS:
            mHeldDir = -1;
            mHold.press(now);
            break;
        case Btn::L1_RELEASE:
        case Btn::L2_RELEASE:
            mHold.release(now);
            break;
        case Btn::L1:
            if (mHold.takeClick(now)) {
                step(1);
            }
            break;
        case Btn::L2:
            if (mHold.takeClick(now)) {
                step(-1);
            }
            break;
        case Btn::R1:
            mSaving = true;
            mHeldDir = 0;
            lv_label_set_text(mTitle, "Saving...");
            mModel.saveWeighIn(mValueH);
            break;
        case Btn::R2:
            mModel.setPage(0);
            ScreenManager::instance().goTo(ScreenId::Home);
            break;
        default:
            break;
    }
}

void WeighInScreen::onSaveResult()
{
    ScreenManager::instance().goTo(ScreenId::Saved);
}

// -- Saved ---------------------------------------------------------------------------

SavedScreen::~SavedScreen()
{
    if (mDone) {
        lv_timer_delete(mDone);
    }
}

void SavedScreen::build()
{
    mHeading = Draw::label(mRoot, &poppins_semibold_25, "", 20, 44, 200);
    mValue   = Draw::label(mRoot, &poppins_semibold_40, "", 20, 80, 200);
    mLine1   = Draw::label(mRoot, &poppins_regular_18, "", 20, 136, 200);
    mLine2   = Draw::label(mRoot, &poppins_regular_18, "", 20, 164, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void SavedScreen::onShow()
{
    refresh();
    mDone = lv_timer_create(&SavedScreen::doneCb, kSavedMs, this);
    lv_timer_set_repeat_count(mDone, 1);
}

void SavedScreen::refresh()
{
    using CustomMessage::SaveOutcome;
    char text[40];
    switch (mModel.lastOutcome()) {
        case SaveOutcome::Saved: {
            lv_label_set_text(mHeading, "Saved");
            lv_obj_set_style_text_color(mHeading, Draw::rgb(Color::GREEN), LV_PART_MAIN);
            char kg[16];
            formatKg(mModel.lastSaved(), 1, kg, sizeof(kg));
            std::snprintf(text, sizeof(text), "%s kg", kg);
            lv_label_set_text(mValue, text);
            if (mModel.hasHome() && mModel.home().days > 0) {
                const CustomMessage::HomeData& h = mModel.home();
                formatKg(h.trend, 1, kg, sizeof(kg));
                std::snprintf(text, sizeof(text), "Trend %s kg", kg);
                lv_label_set_text(mLine1, text);
                char change[16];
                if (h.hasWeekChange) {
                    formatChange(h.weekChange, 1, change, sizeof(change));
                    std::snprintf(text, sizeof(text), "%s kg this week", change);
                } else {
                    text[0] = '\0';
                }
                lv_label_set_text(mLine2, text);
            }
        } break;
        case SaveOutcome::ClockNotSet:
            lv_label_set_text(mHeading, "Not saved");
            lv_obj_set_style_text_color(mHeading, Draw::rgb(Color::YELLOW), LV_PART_MAIN);
            lv_label_set_text(mValue, "");
            lv_label_set_text(mLine1, "The clock is not set.");
            lv_label_set_text(mLine2, "Sync with the UNA app.");
            break;
        case SaveOutcome::Invalid:
            lv_label_set_text(mHeading, "Not saved");
            lv_obj_set_style_text_color(mHeading, Draw::rgb(Color::YELLOW), LV_PART_MAIN);
            lv_label_set_text(mValue, "");
            lv_label_set_text(mLine1, "That weight is");
            lv_label_set_text(mLine2, "out of range.");
            break;
        case SaveOutcome::WriteFailed:
        default:
            lv_label_set_text(mHeading, "Not saved");
            lv_obj_set_style_text_color(mHeading, Draw::rgb(Color::YELLOW), LV_PART_MAIN);
            lv_label_set_text(mValue, "");
            lv_label_set_text(mLine1, "Could not write the log.");
            lv_label_set_text(mLine2, "Try again.");
            break;
    }
}

void SavedScreen::doneCb(lv_timer_t* t)
{
    auto* self = static_cast<SavedScreen*>(lv_timer_get_user_data(t));
    self->mDone = nullptr;   // a one-shot timer deletes itself after this call
    self->mModel.setPage(0);
    ScreenManager::instance().goTo(ScreenId::Home);
}

void SavedScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code == Btn::L1 || code == Btn::L2 || code == Btn::R1 || code == Btn::R2) {
        mModel.setPage(0);
        ScreenManager::instance().goTo(ScreenId::Home);
    }
}
