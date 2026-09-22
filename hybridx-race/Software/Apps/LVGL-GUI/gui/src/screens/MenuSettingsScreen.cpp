/**
 ******************************************************************************
 * @file    MenuSettingsScreen.cpp
 * @brief   Settings wheel (see MenuSettingsScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuSettingsScreen.hpp"

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;

namespace
{
using Style = WheelMenu::Item::Style;

// Mutable: toggle states and the lockout tip are rewritten from the settings,
// then the wheel is refreshed.
WheelMenu::Item kItems[App::MenuNav::Root::Settings::ID_COUNT] = {
    { Style::Toggle, "Roxzone\nsplits" },
    // One line, and no longer than "Split lock": a Tip row draws its title over
    // its value, so a wrapped title lands on top of the number.
    { Style::Tip,    "Run length" },
    { Style::Tip,    "Split lock" },
    { Style::Toggle, "Vibrate\non split" },
};

char kLockoutTip[8] = "3 s";
char kRunDistanceTip[8] = "1 km";
}  // namespace

MenuSettingsScreen::MenuSettingsScreen(Model& model)
    : Screen(model)
{
}

void MenuSettingsScreen::build()
{
    mMenu    = std::make_unique<WheelMenu>(mRoot, kItems, Menu::ID_COUNT);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "Settings");

    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
}

void MenuSettingsScreen::onShow()
{
    mSettings = mModel.getSettings();
    mMenu->select(mModel.menu().settings.get());
    mModel.resetIdleTimer();
    refreshItems();
}

void MenuSettingsScreen::onHide()
{
    mModel.menu().settings.set(mMenu->selected());
}

void MenuSettingsScreen::refreshItems()
{
    kItems[Menu::ID_ROXZONE].toggleState = mSettings.roxzoneSplits;
    kItems[Menu::ID_VIBRATE].toggleState = mSettings.vibrateOnSplit;

    snprintf(kLockoutTip, sizeof(kLockoutTip), "%u s",
             static_cast<unsigned>(mSettings.splitLockoutSec));
    kItems[Menu::ID_LOCKOUT].tip = kLockoutTip;

    // The table already words 1000 m as "1 km", which is how the format writes
    // the race distance.
    snprintf(kRunDistanceTip, sizeof(kRunDistanceTip), "%s",
             Race::runWork(mSettings.runDistanceM));
    kItems[Menu::ID_RUN_DISTANCE].tip = kRunDistanceTip;

    mMenu->refresh();
}

void MenuSettingsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
    case Btn::L1: mMenu->prev(); break;
    case Btn::L2: mMenu->next(); break;
    case Btn::R1: confirm(); break;
    case Btn::R2:
        // Save on the way out so a change is never silently dropped.
        mModel.saveSettings(mSettings);
        ScreenManager::instance().goTo(ScreenId::Main);
        break;
    default: break;
    }
}

void MenuSettingsScreen::confirm()
{
    switch (mMenu->selected()) {
    case Menu::ID_ROXZONE:
        mSettings.roxzoneSplits = !mSettings.roxzoneSplits;
        break;

    case Menu::ID_RUN_DISTANCE:
        // Cycles 100 m to 1 km in 100 m steps, in place, like the lockout.
        // A sim is often run short -- 500 m or 800 m (NOTES.md 5.13).
        mSettings.runDistanceM = static_cast<uint16_t>(
                mSettings.runDistanceM + Race::kRunDistanceStepM);
        if (mSettings.runDistanceM > Race::kRunDistanceMaxM) {
            mSettings.runDistanceM = Race::kRunDistanceMinM;
        }
        break;

    case Menu::ID_LOCKOUT:
        // Cycles 1..10 s in place. A picker screen would be a lot of ceremony
        // for ten integers.
        if (++mSettings.splitLockoutSec > Settings::kLockoutMaxSec) {
            mSettings.splitLockoutSec = Settings::kLockoutMinSec;
        }
        break;

    case Menu::ID_VIBRATE:
        mSettings.vibrateOnSplit = !mSettings.vibrateOnSplit;
        break;

    default:
        break;
    }
    refreshItems();
}

void MenuSettingsScreen::onIdleTimeout()
{
    // Brief 8.4: every menu screen exits on idle. Save first.
    mModel.saveSettings(mSettings);
    mModel.exitApp();
}
