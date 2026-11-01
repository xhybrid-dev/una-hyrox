/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Before a workout: Start and Auto lap (see MainScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "WorkoutText.hpp"

#include "SDK/Messages/AccessoryMessages.hpp"

using namespace SDK::GUI;

namespace
{
using Style = WheelMenu::Item::Style;

// Mutable: the hint lines are rewritten as the choices change, then the wheel
// is refreshed (the SDK widget reads the items, it has no setters).
WheelMenu::Item kItems[3] = {
    { Style::Tip, Strings::kStart },
    { Style::Tip, Strings::kType },
    { Style::Tip, Strings::kAutoLap },
};
char sThirdTip[40];

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

        case RowType:
            // The service saves it and answers with the new value (onSettings).
            mModel.nextKind();
            break;

        case RowThird: {
            const Workout::SessionType t = mModel.plan().type;
            Workout::PlanField fields[3];
            if (t == Workout::SessionType::Plain) {
                mModel.nextAutoLap();
            } else if (Workout::planFields(t, fields) > 0) {
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
    kItems[RowStart].tip      = strapTip(mModel.strapState(), color);
    kItems[RowStart].tipColor = color;

    const Workout::SessionType t = mModel.plan().type;
    kItems[RowType].tip      = Workout::kindName(t);
    kItems[RowType].tipColor = Color::WHITE;

    if (t == Workout::SessionType::Plain) {
        kItems[RowThird].text = Strings::kAutoLap;
        Workout::formatAutoLap(sThirdTip, sizeof(sThirdTip), mModel.autoLapMin());
    } else {
        // Tabata has nothing to set: its hint says what the fixed plan is.
        kItems[RowThird].text = Strings::kSettings;
        Workout::formatPlanSummary(sThirdTip, sizeof(sThirdTip), mModel.plan());
    }
    kItems[RowThird].tip      = sThirdTip;
    kItems[RowThird].tipColor = Color::WHITE;

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

void MainScreen::onIdleTimeout()
{
    // Every menu screen exits on idle.
    mModel.exitApp();
}

void MainScreen::onSessionUpdate(const Workout::SessionSnapshot&)
{
    if (mModel.sessionActive()) {
        ScreenManager::instance().goTo(ScreenId::Live);
    }
}
