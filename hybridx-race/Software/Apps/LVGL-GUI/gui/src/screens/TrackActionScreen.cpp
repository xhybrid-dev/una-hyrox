/**
 ******************************************************************************
 * @file    TrackActionScreen.cpp
 * @brief   In-race action menu (see TrackActionScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackActionScreen.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;

namespace
{
using Style = WheelMenu::Item::Style;

// Mutable: the pause row's label flips with the race state.
WheelMenu::Item kItems[App::MenuNav::RaceView::Action::ID_COUNT] = {
    { Style::Simple, "Resume" },
    { Style::Simple, "Undo last\nsplit" },
    { Style::Simple, "Pause" },
    { Style::Simple, "End race" },
    { Style::Simple, "Discard" },
};
}  // namespace

TrackActionScreen::TrackActionScreen(Model& model)
    : Screen(model)
{
}

void TrackActionScreen::build()
{
    mMenu    = std::make_unique<WheelMenu>(mRoot, kItems, Menu::ID_COUNT);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "Race");

    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
}

void TrackActionScreen::onShow()
{
    // Deliberately no trackPause() here -- see the header.
    mMenu->select(mModel.menu().race.action.get());
    mModel.resetIdleTimer();
    refreshItems();
}

void TrackActionScreen::onHide()
{
    mModel.menu().race.action.set(mMenu->selected());
}

void TrackActionScreen::refreshItems()
{
    kItems[Menu::ID_PAUSE].text = mModel.isRacePaused() ? "Resume\ntimer" : "Pause";
    mMenu->refresh();
}

void TrackActionScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
    case Btn::L1: mMenu->prev(); break;
    case Btn::L2: mMenu->next(); break;

    // End race and Discard are hold-to-confirm: both are unrecoverable, and a
    // single press is exactly what a sweaty hand produces by accident.
    case Btn::R1_PRESS:
        if (mMenu->selected() == Menu::ID_END) {
            mModel.setHoldConfirmMode(Model::HoldConfirmMode::Finish);
            ScreenManager::instance().goTo(ScreenId::RaceHoldConfirm);
        } else if (mMenu->selected() == Menu::ID_DISCARD) {
            mModel.setHoldConfirmMode(Model::HoldConfirmMode::Discard);
            ScreenManager::instance().goTo(ScreenId::RaceHoldConfirm);
        }
        break;

    case Btn::R1: confirm(); break;

    case Btn::R2:
        ScreenManager::instance().goTo(ScreenId::Race);
        break;

    default:
        break;
    }
}

void TrackActionScreen::confirm()
{
    switch (mMenu->selected()) {
    case Menu::ID_RESUME:
        ScreenManager::instance().goTo(ScreenId::Race);
        break;

    case Menu::ID_UNDO_SPLIT:
        mModel.raceUndoSplit();
        ScreenManager::instance().goTo(ScreenId::Race);
        break;

    case Menu::ID_PAUSE:
        if (mModel.isRacePaused()) {
            mModel.raceResume();
        } else {
            mModel.racePause();
        }
        refreshItems();
        break;

    default:
        // End and Discard are handled on R1_PRESS, not on the click.
        break;
    }
}

void TrackActionScreen::onIdleTimeout()
{
    // Brief 8.2 item 5: back to the race after ten seconds, and NEVER out of
    // the app -- a race must not be abandoned because nobody pressed anything.
    ScreenManager::instance().goTo(ScreenId::Race);
}
