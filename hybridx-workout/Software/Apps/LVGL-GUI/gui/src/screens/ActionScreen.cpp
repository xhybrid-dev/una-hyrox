/**
 ******************************************************************************
 * @file    ActionScreen.cpp
 * @brief   The action menu (see ActionScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/ActionScreen.hpp"

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;

namespace
{
using Style = WheelMenu::Item::Style;

// Mutable: the pause row flips with the session's state.
WheelMenu::Item kItems[4] = {
    { Style::Simple, Strings::kResume },
    { Style::Simple, Strings::kPause },
    { Style::Simple, Strings::kEnd },
    { Style::Simple, Strings::kDiscard },
};

// A finished plan: End, Discard, and Back to the live screen.
const WheelMenu::Item kDoneItems[3] = {
    { Style::Simple, Strings::kEnd },
    { Style::Simple, Strings::kDiscard },
    { Style::Simple, Strings::kBack },
};
} // namespace

ActionScreen::ActionScreen(Model& model)
    : Screen(model)
{
}

void ActionScreen::build()
{
    const Workout::SessionSnapshot& s = mModel.snapshot();
    mDone = s.plan.steps > 0 && s.plan.done;
    if (mDone) {
        mMenu = std::make_unique<WheelMenu>(mRoot, kDoneItems, 3);
    } else {
        mMenu = std::make_unique<WheelMenu>(mRoot, kItems, RowCount);
    }
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, Strings::kActionTitle);

    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::AMBER,
                  Widgets::Buttons::WHITE);
}

void ActionScreen::onShow()
{
    // Deliberately no pause on entry: the clock runs while the menu is open.
    mModel.setIdleTimeout(Model::kActionTimeoutSteps);
    // Always opens on Resume, so what R1 then does is the same every time; a
    // finished plan opens on End.
    mMenu->select(0);
    mMenu->setBackground(Color::TEAL_DARK);
    refreshItems();
}

ActionScreen::Row ActionScreen::selectedRow() const
{
    const uint16_t i = mMenu->selected();
    if (mDone) {
        static constexpr Row kDoneRows[3] = {RowEnd, RowDiscard, RowResume};
        return kDoneRows[i < 3 ? i : 2];
    }
    return i < RowCount ? static_cast<Row>(i) : RowResume;
}

void ActionScreen::refreshItems()
{
    if (mDone) {
        return;   // nothing in the done menu changes
    }
    kItems[RowPause].text = mModel.paused() ? Strings::kResumeTimer : Strings::kPause;
    mMenu->refresh();
}

void ActionScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;

        // End and Discard are hold-to-confirm: a single press is exactly what
        // a wet hand produces by accident. The press opens the hold screen; a
        // release before it completes comes back here.
        case Btn::R1_PRESS:
            if (selectedRow() == RowEnd) {
                mModel.setHoldMode(Model::HoldMode::End);
                ScreenManager::instance().goTo(ScreenId::HoldConfirm);
            } else if (selectedRow() == RowDiscard) {
                mModel.setHoldMode(Model::HoldMode::Discard);
                ScreenManager::instance().goTo(ScreenId::HoldConfirm);
            }
            break;

        case Btn::R1: confirm(); break;

        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Live);
            break;

        default:
            break;
    }
}

void ActionScreen::confirm()
{
    switch (selectedRow()) {
        case RowResume:
            ScreenManager::instance().goTo(ScreenId::Live);
            break;

        case RowPause:
            if (mModel.paused()) {
                mModel.resume();
            } else {
                mModel.pause();
            }
            // The row flips when the service's next update says so.
            break;

        default:
            // End and Discard are handled on R1_PRESS, not on the click.
            break;
    }
}

void ActionScreen::onSessionUpdate(const Workout::SessionSnapshot&)
{
    refreshItems();
}

void ActionScreen::onIdleTimeout()
{
    // Back to the session after ten seconds, and never out of the app.
    ScreenManager::instance().goTo(ScreenId::Live);
}
