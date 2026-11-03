/**
 ******************************************************************************
 * @file    PlanSettingsScreen.cpp
 * @brief   The chosen kind's numbers (see PlanSettingsScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/PlanSettingsScreen.hpp"

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;

namespace
{
using Style = WheelMenu::Item::Style;

// Mutable: the names and hints are set for the kind on show (the SDK widget
// reads the items, it has no setters).
WheelMenu::Item kItems[3] = {
    { Style::Tip, "" },
    { Style::Tip, "" },
    { Style::Tip, "" },
};
char sTips[3][16];
} // namespace

PlanSettingsScreen::PlanSettingsScreen(Model& model)
    : Screen(model)
{
}

void PlanSettingsScreen::build()
{
    mCount = Ride::planFields(mModel.plan().type, mFields);
    for (uint8_t i = 0; i < mCount; ++i) {
        kItems[i].text = Ride::fieldName(mFields[i]);
    }
    // The wheel needs at least one row; a kind with none goes back on show.
    mMenu    = std::make_unique<WheelMenu>(mRoot, kItems, mCount > 0 ? mCount : 1);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, Ride::kindName(mModel.plan().type));
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::AMBER,
                  Widgets::Buttons::WHITE);
}

void PlanSettingsScreen::onShow()
{
    mModel.setIdleTimeout();
    if (mCount == 0) {
        ScreenManager::instance().goTo(ScreenId::Main);
        return;
    }
    mMenu->select(0);
    refresh();
}

void PlanSettingsScreen::refresh()
{
    for (uint8_t i = 0; i < mCount; ++i) {
        Ride::formatField(sTips[i], sizeof(sTips[i]), mModel.plan(), mFields[i]);
        kItems[i].tip      = sTips[i];
        kItems[i].tipColor = Color::WHITE;
    }
    mMenu->setBackground(Color::TEAL_DARK);
    mMenu->refresh();
}

void PlanSettingsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            if (mMenu->selected() < mCount) {
                // The service saves it and answers with the new value (onSettings).
                mModel.stepPlanField(mFields[mMenu->selected()]);
            }
            break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::Main); break;
        default: break;
    }
}

void PlanSettingsScreen::onSettings()
{
    refresh();
}

void PlanSettingsScreen::onIdleTimeout()
{
    // Every menu screen exits on idle.
    mModel.exitApp();
}
