/**
 ******************************************************************************
 * @file    LogScreen.cpp
 * @brief   Log a session (see the header).
 *
 * Two questions, one screen each (the screen is rebuilt between them): what
 * kind, then today or yesterday. Yesterday is offered only while it is still
 * this week (S7). The session then plays on the home screen like any other.
 ******************************************************************************
 */

#include "gui/screens/LogScreen.hpp"

#include "gui/copy/Coach.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Style = Widgets::Wheel::Item::Style;

namespace
{
/// The order the wheel offers them: the likeliest first.
constexpr Streak::Kind kOrder[] = {
    Streak::Kind::Run,     Streak::Kind::Strength, Streak::Kind::Ride, Streak::Kind::Walk,
    Streak::Kind::Row,     Streak::Kind::Hybrid,   Streak::Kind::Workout, Streak::Kind::Other,
};
} // namespace

void LogScreen::build()
{
    if (!mModel.logWhen) {
        for (uint8_t i = 0; i < Streak::kKindCount; ++i) {
            mItems[i] = { Style::Simple, Coach::sportName(kOrder[i]) };
        }
        mCount = Streak::kKindCount;
        mTitle = std::make_unique<Widgets::Title>(mRoot, "Log a session");
    } else {
        mItems[0] = { Style::Simple, "Today" };
        mItems[1] = { Style::Simple, "Yesterday" };
        // Yesterday was last week when today is the week's first day.
        mCount = mModel.home().daysLeft >= 7 ? 1 : 2;
        mTitle = std::make_unique<Widgets::Title>(mRoot, "When?");
    }
    mMenu    = std::make_unique<Widgets::Wheel>(mRoot, mItems, mCount);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::GREEN, Widgets::Buttons::WHITE);
}

void LogScreen::onShow()
{
    if (!mModel.logWhen) {
        for (uint8_t i = 0; i < mCount; ++i) {
            if (static_cast<uint8_t>(kOrder[i]) == mModel.logKind) {
                mMenu->select(i);
            }
        }
    }
    mModel.resetIdleTimer();
}

void LogScreen::onHide() {}

void LogScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            if (!mModel.logWhen) {
                mModel.logKind = static_cast<uint8_t>(kOrder[mMenu->selected()]);
                mModel.logWhen = true;
                ScreenManager::instance().goTo(ScreenId::Log);
            } else {
                mModel.logManual(static_cast<Streak::Kind>(mModel.logKind), mMenu->selected() == 1);
                mModel.logWhen = false;
                ScreenManager::instance().goTo(ScreenId::Home);   // it plays there
            }
            break;
        case Btn::R2:
            if (mModel.logWhen) {
                mModel.logWhen = false;
                ScreenManager::instance().goTo(ScreenId::Log);
            } else {
                ScreenManager::instance().goTo(ScreenId::Menu);
            }
            break;
        default:
            break;
    }
}
