/**
 ******************************************************************************
 * @file    MenuSettingsScreen.cpp
 * @brief   Settings menu (see MenuSettingsScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuSettingsScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Assets.hpp"

using Style = WheelMenu::Item::Style;

MenuSettingsScreen::MenuSettingsScreen(Model& model)
    : Screen(model)
{
}

void MenuSettingsScreen::build()
{
    mItems[Menu::ID_ALERTS]      = { Style::Simple, "Lap Alerts" };
    // X Training (TP3): the plan's settings.
    mItems[Menu::ID_REPS_DISTANCE] = { Style::Toggle, "Reps by\ndistance", "Reps by distance", &poppins_semibold_25 };
    mItems[Menu::ID_EASY_HR]       = { Style::Toggle, "Easy by\nheart rate", "Easy by HR", &poppins_semibold_25 };
    mItems[Menu::ID_RACE_ALERTS]   = { Style::Tip, "Race alerts", nullptr, &poppins_semibold_25 };
    mItems[Menu::ID_RACE_ALERTS].tip     = "Off";
    mItems[Menu::ID_RACE_ALERTS].itemTip = "";
    mItems[Menu::ID_PHONE_NOTIF] = { Style::Toggle, "Phone\nNotif.", "Phone Notif.", &poppins_semibold_25 };

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, "SETTINGS");
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuSettingsScreen::onShow()
{
    mMenu->select(mModel.menu().settings.get());
    mModel.menu().settings.resetChildren();
    mModel.resetIdleTimer();
    onSettings(mModel.getSettings());
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuSettingsScreen::onHide()
{
    mModel.menu().settings.set(mMenu->selected());
}

void MenuSettingsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: {
            if (mMenu->selected() == Menu::ID_ALERTS) {
                ScreenManager::instance().goTo(ScreenId::MenuAlerts);
                break;
            }
            Settings sett = mModel.getSettings();
            switch (mMenu->selected()) {
                case Menu::ID_REPS_DISTANCE: sett.repsByDistance = !sett.repsByDistance; break;
                case Menu::ID_EASY_HR:       sett.easyByHr = !sett.easyByHr; break;
                case Menu::ID_RACE_ALERTS:
                    sett.raceAlerts = static_cast<uint8_t>((sett.raceAlerts + 1) % Settings::RACE_ALERTS_COUNT);
                    break;
                default:                     sett.phoneNotifEn = !sett.phoneNotifEn; break;
            }
            mModel.saveSettings(sett);
            onSettings(sett);
        } break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Main);
            break;
        default:
            break;
    }
}

void MenuSettingsScreen::onSettings(const Settings& settings)
{
    mItems[Menu::ID_PHONE_NOTIF].toggleState = settings.phoneNotifEn;
    mItems[Menu::ID_REPS_DISTANCE].toggleState = settings.repsByDistance;
    mItems[Menu::ID_EASY_HR].toggleState = settings.easyByHr;
    static const char* const kRace[Settings::RACE_ALERTS_COUNT] = { "Off", "First 2 km, too fast", "Both ways" };
    mItems[Menu::ID_RACE_ALERTS].tip = kRace[settings.raceAlerts < Settings::RACE_ALERTS_COUNT ? settings.raceAlerts : 0];
    mMenu->refresh();
}

void MenuSettingsScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuSettingsScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
