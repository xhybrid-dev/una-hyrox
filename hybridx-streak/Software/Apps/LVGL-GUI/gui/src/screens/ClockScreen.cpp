/**
 ******************************************************************************
 * @file    ClockScreen.cpp
 * @brief   The watch has lost the time (see the header).
 *
 * After a flat battery, before the phone syncs, the clock can read 1970. The
 * service then judges nothing (PLAN 6.1): no week could be dated. This says
 * so plainly, and what to do.
 ******************************************************************************
 */

#include "gui/screens/ClockScreen.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/widgets/Widgets.hpp"

void ClockScreen::build()
{
    Theme::label(mRoot, Theme::Font::SemiBold25, "Set the time", 20, 70, 200, LV_TEXT_ALIGN_CENTER, Palette::kAtRisk);
    lv_obj_t* body = Theme::label(mRoot, Theme::Font::Regular16,
                                  "Your watch has lost the time. Sync it with the UNA app and your streak carries on.",
                                  36, 112, 168, LV_TEXT_ALIGN_CENTER, Palette::kTextSoft);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
}

void ClockScreen::onShow()
{
    mModel.resetIdleTimer();
}

void ClockScreen::onHide() {}

void ClockScreen::onHomeView()
{
    if (!mModel.clockUnset()) {
        ScreenManager::instance().goTo(ScreenId::Home);
    }
}

void ClockScreen::onKey(uint8_t code)
{
    if (code == SDK::GUI::Button::R2) {
        mModel.exitApp();
    }
}
