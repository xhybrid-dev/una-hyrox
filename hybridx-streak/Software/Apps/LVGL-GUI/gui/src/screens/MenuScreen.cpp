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

#include "LoadFormText.hpp"
#include "gui/copy/DashWords.hpp"

using Style = Widgets::Wheel::Item::Style;

void MenuScreen::build()
{
    mItems[kWeek]     = { Style::Tip, "This week" };
    mItems[kSport]    = { Style::Tip, "By sport" };
    mItems[kSport].tip      = mSportTip;
    mItems[kSport].tipColor = Palette::kTextSoft;
    mItems[kNumbers]  = { Style::Tip, "Your numbers" };
    mItems[kNumbers].tip      = mNumbersTip;
    mItems[kNumbers].tipColor = Palette::kTextSoft;
    mItems[kLog]      = { Style::Simple, "Log a session" };
    mItems[kTrophy]   = { Style::Simple, "Trophy case" };
    mItems[kLoad]     = { Style::Tip, "Training load" };
    mItems[kLoad].tip      = mLoadTip;
    mItems[kLoad].tipColor = Palette::kTextSoft;
    mItems[kSettings] = { Style::Simple, "Settings" };
    mItems[kWeek].tip      = mWeekTip;
    mItems[kWeek].tipColor = Palette::kWin;
    onHomeView();
    onDash();

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
    Streak::formatFormLine(mLoadTip, sizeof(mLoadTip), v.form, static_cast<Streak::FormWord>(v.formWord), v.loadDays);
    if (mMenu) {
        mMenu->refresh();
    }
}

void MenuScreen::onDash()
{
    const CustomMessage::DashData& d = mModel.dash();
    DashWords::menuTip(d.week, d.manual, mSportTip, sizeof(mSportTip));
    DashWords::numbersMenuTip(d.family, mNumbersTip, sizeof(mNumbersTip));
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
                case kSport:
                    mModel.sportAt = 0;
                    ScreenManager::instance().goTo(ScreenId::Sport);
                    break;
                case kNumbers:
                    mModel.numbersAt = 0;
                    ScreenManager::instance().goTo(ScreenId::Numbers);
                    break;
                case kLog:
                    mModel.logWhen = false;
                    ScreenManager::instance().goTo(ScreenId::Log);
                    break;
                case kTrophy:
                    mModel.trophyAt = 0;
                    ScreenManager::instance().goTo(ScreenId::Trophy);
                    break;
                case kLoad:
                    mModel.loadAt = 0;
                    ScreenManager::instance().goTo(ScreenId::Load);
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
