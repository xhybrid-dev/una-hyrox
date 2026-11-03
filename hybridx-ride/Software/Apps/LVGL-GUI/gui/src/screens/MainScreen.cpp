/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Before a ride: Start, the ride type and its settings (see MainScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "RidePlanText.hpp"
#include "RideText.hpp"

#include "SDK/Messages/AccessoryMessages.hpp"

using namespace SDK::GUI;

namespace
{
using Style = WheelMenu::Item::Style;

// Mutable: the hint lines are rewritten as the choices change, then the wheel
// is refreshed (the SDK widget reads the items, it has no setters).
WheelMenu::Item kItems[5] = {
    { Style::Tip, Strings::kStart },
    { Style::Tip, Strings::kRideType },
    { Style::Tip, Strings::kSensor },
    { Style::Tip, Strings::kAutoLap },
    { Style::Tip, Strings::kAutoPause },
};
char sFourthTip[24];
char sFifthTip[40];

/// What Start will do about position: the GPS outdoors, nothing indoors.
const char* startTip(const Model& model, bool warned, uint32_t& color)
{
    color = Color::GRAY;
    if (!model.outdoor()) {
        return Strings::kIndoorHint;
    }
    if (model.gps() == static_cast<uint8_t>(Ride::GpsState::Fix)) {
        color = Color::GREEN;
        return Strings::kGpsReady;
    }
    if (warned) {
        color = Color::YELLOW_DARK;
        return Strings::kNoGpsAgain;
    }
    return Strings::kGpsFinding;
}

/// Where the heart rate will come from, as the kernel last said.
const char* strapTip(uint8_t state, uint32_t& color)
{
    using State = SDK::Accessory::State;
    color = Color::GRAY;
    switch (state) {
        case static_cast<uint8_t>(State::CONNECTED):
            color = Color::GREEN;
            return Strings::kStrapConnected;
        case static_cast<uint8_t>(State::SEARCHING):
        case static_cast<uint8_t>(State::CONNECTING):
            return Strings::kStrapFinding;
        case static_cast<uint8_t>(State::LOST):
            color = Color::YELLOW_DARK;
            return Strings::kStrapLost;
        default:
            // Unavailable (external HR off, or no strap remembered), idle, or not said.
            return Strings::kWristHr;
    }
}
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
    mWarnedNoGps = false;
    // Back on the menu (the first time, or after a ride): an outdoor ride's GPS looks again.
    if (mModel.settingsKnown()) {
        mModel.setOutdoor(mModel.outdoor());
    }
    mMenu->select(mModel.mainPos());
    refresh();
}

void MainScreen::onHide()
{
    mModel.setMainPos(mMenu->selected());
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code != Btn::R1) {
        mWarnedNoGps = false;
    }
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
            if (mModel.outdoor() && mModel.gps() != static_cast<uint8_t>(Ride::GpsState::Fix) && !mWarnedNoGps) {
                mWarnedNoGps = true;   // warn once; the second press starts anyway
                refresh();
                break;
            }
            mModel.startSession();
            ScreenManager::instance().goTo(ScreenId::Live);
            break;

        case RowRide:
            // The service saves it and answers with the new value (onSettings).
            mWarnedNoGps = false;
            mModel.setOutdoor(!mModel.outdoor());
            break;

        case RowSensor:
            break;   // only shows where the heart rate comes from

        case RowFourth:
            // The service saves it and answers with the new value (onSettings).
            if (mModel.outdoor()) {
                mModel.nextAutoLap();
            } else {
                mModel.nextKind();
            }
            break;

        case RowFifth: {
            const Ride::SessionType t = mModel.plan().type;
            Ride::PlanField fields[3];
            if (mModel.outdoor()) {
                mModel.toggleAutoPause();
            } else if (t == Ride::SessionType::Plain) {
                mModel.nextAutoLap();
            } else if (Ride::planFields(t, fields) > 0) {
                ScreenManager::instance().goTo(ScreenId::PlanSettings);
            }
            // Tabata: nothing to set.
        } break;

        default:
            break;
    }
}

void MainScreen::refresh()
{
    uint32_t color = Color::GRAY;
    kItems[RowStart].tip      = startTip(mModel, mWarnedNoGps, color);
    kItems[RowStart].tipColor = color;

    kItems[RowRide].tip      = mModel.outdoor() ? Strings::kOutdoor : Strings::kIndoor;
    kItems[RowRide].tipColor = Color::WHITE;

    kItems[RowSensor].tip      = strapTip(mModel.strapState(), color);
    kItems[RowSensor].tipColor = color;

    if (mModel.outdoor()) {
        kItems[RowFourth].text = Strings::kAutoLap;
        Ride::formatAutoLapChoice(sFourthTip, sizeof(sFourthTip), mModel.autoLapMin(), mModel.autoLapKm(), true);
        kItems[RowFifth].text = Strings::kAutoPause;
        Ride::plantext::copyOut(sFifthTip, sizeof(sFifthTip), mModel.autoPause() ? Strings::kOn : Strings::kOff);
    } else {
        const Ride::SessionType t = mModel.plan().type;
        kItems[RowFourth].text = Strings::kType;
        Ride::plantext::copyOut(sFourthTip, sizeof(sFourthTip), Ride::kindName(t));
        if (t == Ride::SessionType::Plain) {
            kItems[RowFifth].text = Strings::kAutoLap;
            Ride::formatAutoLapChoice(sFifthTip, sizeof(sFifthTip), mModel.autoLapMin(), 0, false);
        } else {
            // Tabata has nothing to set: its hint says what the fixed plan is.
            kItems[RowFifth].text = Strings::kSettings;
            Ride::formatPlanSummary(sFifthTip, sizeof(sFifthTip), mModel.plan());
        }
    }
    kItems[RowFourth].tip      = sFourthTip;
    kItems[RowFourth].tipColor = Color::WHITE;
    kItems[RowFifth].tip       = sFifthTip;
    kItems[RowFifth].tipColor  = Color::WHITE;

    mMenu->setBackground(Color::TEAL_DARK);
    mMenu->refresh();
}

void MainScreen::onSettings()
{
    refresh();
}

void MainScreen::onStrap()
{
    refresh();
}

void MainScreen::onGps()
{
    if (mModel.gps() == static_cast<uint8_t>(Ride::GpsState::Fix)) {
        mWarnedNoGps = false;
    }
    refresh();
}

void MainScreen::onIdleTimeout()
{
    // Every menu screen exits on idle.
    mModel.exitApp();
}

void MainScreen::onSessionUpdate(const Ride::SessionSnapshot&)
{
    if (mModel.sessionActive()) {
        ScreenManager::instance().goTo(ScreenId::Live);
    }
}
