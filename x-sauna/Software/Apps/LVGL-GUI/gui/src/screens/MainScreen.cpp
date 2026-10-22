/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Before a session: Start, Mode and Rest (see MainScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;

namespace
{
using Style = WheelMenu::Item::Style;

// Mutable: the hint lines are rewritten as the choices change, then the wheel
// is refreshed (the SDK widget reads the items, it has no setters).
WheelMenu::Item kItems[3] = {
    { Style::Tip, Strings::kStart },
    { Style::Tip, Strings::kMode },
    { Style::Tip, Strings::kRest },
};
} // namespace

MainScreen::MainScreen(Model& model)
    : Screen(model)
{
}

void MainScreen::build()
{
    mMenu    = std::make_unique<WheelMenu>(mRoot, kItems, RowCount);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, Strings::kAppTitle);

    mMenu->setSlideMidCallback([](void* ctx, uint16_t) { static_cast<MainScreen*>(ctx)->refresh(); }, this);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::AMBER,
                  Widgets::Buttons::WHITE);
}

void MainScreen::onShow()
{
    mModel.setIdleTimeout();
    if (mModel.sessionActive()) {
        ScreenManager::instance().goTo(ScreenId::Live);
        return;
    }
    mMenu->select(mModel.mainPos());
    refresh();
}

void MainScreen::onHide()
{
    mModel.setMainPos(mMenu->selected());
}

bool MainScreen::restLocked() const
{
    // Just sauna alternates Sauna and Rest, always.
    return mModel.mode() == static_cast<uint8_t>(Sauna::Mode::SaunaOnly);
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: confirm(); break;
        case Btn::R2: mModel.exitApp(); break;
        default: break;
    }
}

void MainScreen::confirm()
{
    switch (mMenu->selected()) {
        case RowStart:
            mModel.startSession();
            ScreenManager::instance().goTo(ScreenId::Live);
            break;

        case RowMode:
            mModel.setMode(restLocked() ? static_cast<uint8_t>(Sauna::Mode::SaunaPlunge)
                                        : static_cast<uint8_t>(Sauna::Mode::SaunaOnly));
            refresh();
            break;

        case RowRest:
            if (!restLocked()) {
                mModel.setRest(!mModel.rest());
                refresh();
            }
            break;

        default:
            break;
    }
}

void MainScreen::refresh()
{
    const bool locked = restLocked();

    kItems[RowMode].tip = locked ? Strings::kModeJustSauna : Strings::kModeSaunaPlunge;
    if (locked) {
        kItems[RowRest].tip      = Strings::kAlwaysOn;
        kItems[RowRest].tipColor = Color::GRAY;
        kItems[RowStart].tip     = Strings::kStartJustSauna;
    } else {
        kItems[RowRest].tip      = mModel.rest() ? Strings::kOn : Strings::kOff;
        kItems[RowRest].tipColor = Color::WHITE;
        kItems[RowStart].tip     = mModel.rest() ? Strings::kStartSaunaPlungeRest : Strings::kStartSaunaPlunge;
    }

    // A Rest row that cannot change is greyed, and drops its select hint.
    const bool blocked = (mMenu->selected() == RowRest) && locked;
    mMenu->setBackground(blocked ? Color::GRAY_DARK : Color::SIENNA_DARK);
    mButtons->setR1(blocked ? Widgets::Buttons::NONE : Widgets::Buttons::AMBER);
    mMenu->refresh();
}

void MainScreen::onIdleTimeout()
{
    // Every menu screen exits on idle.
    mModel.exitApp();
}

void MainScreen::onSessionUpdate(const Sauna::SessionSnapshot&)
{
    if (mModel.sessionActive()) {
        ScreenManager::instance().goTo(ScreenId::Live);
    }
}
