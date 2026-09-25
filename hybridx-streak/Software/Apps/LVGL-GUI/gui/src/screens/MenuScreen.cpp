/**
 ******************************************************************************
 * @file    MenuScreen.cpp
 * @brief   The menu (see the header).
 ******************************************************************************
 */

#include "gui/screens/MenuScreen.hpp"

#include <cstdio>

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Style = Widgets::Wheel::Item::Style;

void MenuScreen::build()
{
    mItems[kWeek]     = { Style::Tip, "This week" };
    mItems[kLog]      = { Style::Simple, "Log a session" };
    mItems[kTrophy]   = { Style::Simple, "Trophy case" };
    mItems[kSettings] = { Style::Simple, "Settings" };
    mItems[kWeek].tip      = mWeekTip;
    mItems[kWeek].tipColor = Palette::kWin;
    onHomeView();

    mTitle   = std::make_unique<Widgets::Title>(mRoot, "Streak");
    mMenu    = std::make_unique<Widgets::Wheel>(mRoot, mItems, kCount);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::AMBER,
                  Widgets::Buttons::WHITE);
}

void MenuScreen::onShow()
{
    mMenu->select(mModel.menuAt < kCount ? mModel.menuAt : 0);
    mModel.resetIdleTimer();
}

void MenuScreen::onHide()
{
    mModel.menuAt = mMenu->selected();
}

void MenuScreen::onWeek()
{
    onHomeView();
}

void MenuScreen::onHomeView()
{
    const Streak::HomeView& v = mModel.home();
    snprintf(mWeekTip, sizeof(mWeekTip), "%u of %u counted", static_cast<unsigned>(v.sessions),
             static_cast<unsigned>(v.target));
    if (mMenu) {
        mMenu->refresh();
    }
}

void MenuScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            switch (mMenu->selected()) {
                case kWeek:
                    mModel.weekAt = 0;
                    ScreenManager::instance().goTo(ScreenId::Week);
                    break;
                case kLog:
                    mModel.logWhen = false;
                    ScreenManager::instance().goTo(ScreenId::Log);
                    break;
                case kTrophy:
                    mModel.trophyAt = 0;
                    ScreenManager::instance().goTo(ScreenId::Trophy);
                    break;
                default:
                    mModel.settingsAt = 0;
                    ScreenManager::instance().goTo(ScreenId::Settings);
                    break;
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Home);
            break;
        default:
            break;
    }
}
