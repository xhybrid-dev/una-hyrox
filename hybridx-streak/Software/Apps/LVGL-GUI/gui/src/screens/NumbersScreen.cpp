/**
 ******************************************************************************
 * @file    NumbersScreen.cpp
 * @brief   Your numbers (see the header).
 ******************************************************************************
 */

#include "gui/screens/NumbersScreen.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Style = Widgets::Wheel::Item::Style;

void NumbersScreen::fill()
{
    mCount = DashWords::numbersRows(mModel.dash().family, mRows);
    for (uint8_t i = 0; i < mCount; ++i) {
        mItems[i]          = { Style::Tip, mRows[i].label };
        mItems[i].tip      = mRows[i].tip;
        mItems[i].tipColor = mRows[i].tone == 1   ? Palette::kWin
                             : mRows[i].tone == 2 ? Palette::kAtRisk
                                                  : Palette::kTextSoft;
    }
}

void NumbersScreen::build()
{
    fill();
    mTitle   = std::make_unique<Widgets::Title>(mRoot, DashWords::kNumbersTitle);
    mMenu    = std::make_unique<Widgets::Wheel>(mRoot, mItems, mCount);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
}

void NumbersScreen::onShow()
{
    mMenu->select(mModel.numbersAt < mCount ? mModel.numbersAt : 0);
    mModel.resetIdleTimer();
}

void NumbersScreen::onHide()
{
    mModel.numbersAt = mMenu->selected();
}

void NumbersScreen::onDash()
{
    // The rows can change in number: rebuild, as This week does. onHide keeps the place.
    ScreenManager::instance().goTo(ScreenId::Numbers);
}

void NumbersScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::Menu); break;
        default: break;
    }
}
