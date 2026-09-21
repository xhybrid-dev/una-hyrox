/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Pre-race menu (see MainScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include "gui/Assets.hpp"
#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#define LOG_MODULE_PRX   "MainScreen"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

using namespace SDK::GUI;

namespace
{
using Style = WheelMenu::Item::Style;

// Brief 8.2 item 1. No GPS gating anywhere: HYROX is indoors.
// Mutable: the Format row's tip line is rewritten as the format changes, then
// the wheel is refreshed (the SDK widget reads the item, it has no setter).
WheelMenu::Item kItems[App::MenuNav::Root::ID_COUNT] = {
    { Style::Simple, "Start race", nullptr, &poppins_semibold_30 },
    { Style::Tip,    "Format" },
    { Style::Simple, "Last race" },
    { Style::Simple, "Settings" },
};

const char* formatName(Race::Format f)
{
    switch (f) {
    case Race::Format::HalfA: return "Half: 1 to 4";
    case Race::Format::HalfB: return "Half: 5 to 8";
    case Race::Format::Full:
    default:                  return "Full";
    }
}
}  // namespace

MainScreen::MainScreen(Model& model)
    : Screen(model)
{
}

void MainScreen::build()
{
    mMenu = std::make_unique<WheelMenu>(mRoot, kItems, Menu::ID_COUNT);
    mMenu->setSlideMidCallback(
        [](void* ctx, uint16_t) { static_cast<MainScreen*>(ctx)->updateBackground(); }, this);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, Strings::kAppNameUc);
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);

    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
}

void MainScreen::onShow()
{
    mMenu->select(mModel.menu().get());
    mModel.menu().resetChildren();
    mModel.resetIdleTimer();
    updateBackground();
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MainScreen::onHide()
{
    mModel.menu().set(mMenu->selected());
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
    case Menu::ID_START:
        ScreenManager::instance().goTo(ScreenId::RaceStartConfirm);
        break;

    case Menu::ID_FORMAT:
        // Cycling in place rather than opening a screen: there are only three
        // formats and the hint line shows the current one.
        cycleFormat();
        break;

    case Menu::ID_LAST_RACE:
        if (mModel.isSummaryAvailable()) {
            ScreenManager::instance().goTo(ScreenId::RaceSummary);
        }
        break;

    case Menu::ID_SETTINGS:
        ScreenManager::instance().goTo(ScreenId::MenuSettings);
        break;

    default:
        break;
    }
}

void MainScreen::cycleFormat()
{
    Race::Format next = Race::Format::Full;
    switch (mModel.getFormat()) {
    case Race::Format::Full:  next = Race::Format::HalfA; break;
    case Race::Format::HalfA: next = Race::Format::HalfB; break;
    case Race::Format::HalfB:
    default:                  next = Race::Format::Full;  break;
    }
    mModel.setFormat(next);
    updateBackground();
}

void MainScreen::updateBackground()
{
    const uint16_t selected = mMenu->selected();

    // "Last race" is dead until a race has been saved, so grey it and drop the
    // select hint rather than offering an empty screen.
    const bool blocked = (selected == Menu::ID_LAST_RACE) && !mModel.isSummaryAvailable();
    mMenu->setBackground(blocked ? Color::GRAY_DARK : Color::TEAL_DARK);
    mButtons->setR1(blocked ? Widgets::Buttons::NONE : Widgets::Buttons::AMBER);

    // The Format row carries the current format, so the athlete can see what
    // they are about to start without opening anything.
    kItems[Menu::ID_FORMAT].tip = formatName(mModel.getFormat());
    mMenu->refresh();
}

void MainScreen::onIdleTimeout()
{
    // Every menu screen exits on idle (brief 8.4). The RunLVGL review found ten
    // screens missing this; Start is exempt so a hesitating athlete is not
    // thrown out of the app.
    if (mMenu->selected() != Menu::ID_START) {
        mModel.exitApp();
    }
}

void MainScreen::onSettings(const Settings& /*settings*/)
{
    updateBackground();
}

void MainScreen::onSummary(const ActivitySummary& /*summary*/)
{
    updateBackground();
}

void MainScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
