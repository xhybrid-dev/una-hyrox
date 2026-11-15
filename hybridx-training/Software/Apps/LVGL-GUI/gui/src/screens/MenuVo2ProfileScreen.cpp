/**
 ******************************************************************************
 * @file    MenuVo2ProfileScreen.cpp
 * @brief   HybridX Run: the VO2max profile menu (see the header).
 ******************************************************************************
 */

#include "gui/screens/MenuVo2ProfileScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"

#include <cstdio>

#include "Vo2Text.hpp"

using namespace SDK::GUI;
using Style = WheelMenu::Item::Style;

MenuVo2ProfileScreen::MenuVo2ProfileScreen(Model& model)
    : Screen(model)
{
}

void MenuVo2ProfileScreen::build()
{
    mItems[Menu::ID_BIRTH]       = { Style::Tip, "Birth date", nullptr, &poppins_semibold_30, mBirthTip };
    mItems[Menu::ID_MAX_HR]      = { Style::Tip, "Max HR",     nullptr, &poppins_semibold_30, mMaxTip };
    mItems[Menu::ID_REST_HR]     = { Style::Tip, "Resting HR", nullptr, &poppins_semibold_30, mRestTip };
    // Two lines when selected, as RunLVGL's "Phone\nNotif." toggle: one line
    // does not fit beside the switch.
    mItems[Menu::ID_TUNING_LOG]  = { Style::Toggle, "Tuning\nlog", "Tuning log", &poppins_semibold_25 };
    for (auto& item : mItems) {
        item.tipColor = Color::TEAL;
    }

    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "VO2 PROFILE");
}

void MenuVo2ProfileScreen::onShow()
{
    mMenu->select(mModel.menu().vo2Profile.get());
    mModel.resetIdleTimer();
    refreshTips();
}

void MenuVo2ProfileScreen::onHide()
{
    mModel.menu().vo2Profile.set(mMenu->selected());
}

void MenuVo2ProfileScreen::onVo2Info(const CustomMessage::Vo2Info& /*info*/)
{
    refreshTips();
}

void MenuVo2ProfileScreen::refreshTips()
{
    namespace Text = RunVo2::Text;
    const CustomMessage::Vo2Info& v = mModel.getVo2Info();

    Text::monthYear(v.birthMonth, v.birthYear, mBirthTip, sizeof(mBirthTip));

    if (v.setMaxHr != 0) {
        std::snprintf(mMaxTip, sizeof(mMaxTip), "%u bpm", v.setMaxHr);
    } else if (v.nowMaxHr != 0) {
        std::snprintf(mMaxTip, sizeof(mMaxTip), "Auto: %u (%s)", v.nowMaxHr,
                      Text::maxSourceLabel(static_cast<RunVo2::MaxHrSource>(v.nowMaxSource)));
    } else {
        std::snprintf(mMaxTip, sizeof(mMaxTip), "Auto: needs age");
    }

    if (v.setRestHr != 0) {
        std::snprintf(mRestTip, sizeof(mRestTip), "%u bpm", v.setRestHr);
    } else if (v.nowRestHr != 0) {
        std::snprintf(mRestTip, sizeof(mRestTip), "Watch: %u", v.nowRestHr);
    } else {
        std::snprintf(mRestTip, sizeof(mRestTip), "Watch: none yet");
    }

    mItems[Menu::ID_TUNING_LOG].toggleState = v.tuningLog;
    mMenu->refresh();
}

void MenuVo2ProfileScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            switch (mMenu->selected()) {
                case Menu::ID_BIRTH:   ScreenManager::instance().goTo(ScreenId::Vo2PickBirth); break;
                case Menu::ID_MAX_HR:  ScreenManager::instance().goTo(ScreenId::Vo2PickMaxHr); break;
                case Menu::ID_REST_HR: ScreenManager::instance().goTo(ScreenId::Vo2PickRestHr); break;
                case Menu::ID_TUNING_LOG: {
                    const CustomMessage::Vo2Info& v = mModel.getVo2Info();
                    mModel.saveVo2Profile(v.birthYear, v.birthMonth, v.setMaxHr, v.setRestHr, !v.tuningLog);
                    refreshTips();
                } break;
                default: break;
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Vo2);
            break;
        default:
            break;
    }
    mModel.resetIdleTimer();
}
