/**
 ******************************************************************************
 * @file    WeekScreen.cpp
 * @brief   This week's sessions (see the header).
 *
 * Showing what did NOT count, and why, is what makes automatic counting feel
 * trustworthy rather than mysterious (PLAN 7). Each line is the kind, with a
 * hint: the minutes and the app it came from, or the reason it doesn't count.
 ******************************************************************************
 */

#include "gui/screens/WeekScreen.hpp"

#include <cstdio>

#include "StreakModel.hpp"
#include "gui/copy/Coach.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Style = Widgets::Wheel::Item::Style;

namespace
{
constexpr const char* kDot = "\xC2\xB7";
unsigned u(uint32_t v) { return static_cast<unsigned>(v); }
} // namespace

void WeekScreen::fill()
{
    const CustomMessage::WeekData&     w    = mModel.week();
    const CustomMessage::AppNamesData& apps = mModel.apps();
    mCount = w.count < kMaxItems ? w.count : kMaxItems;

    for (uint8_t i = 0; i < mCount; ++i) {
        const CustomMessage::WeekItem& it   = w.items[i];
        const auto                     kind = static_cast<Streak::Kind>(it.kind < Streak::kKindCount ? it.kind : 7);
        const auto                     st   = static_cast<Streak::Status>(it.status);
        snprintf(mText[i], sizeof(mText[i]), "%s", Coach::sportName(kind));

        char from[20] = "";
        if (it.manual) {
            snprintf(from, sizeof(from), "logged");
        } else if (it.app < CustomMessage::AppNamesData::kMax) {
            Coach::appName(apps.names[it.app], from, sizeof(from));
        }

        switch (st) {
            case Streak::Status::Counts:
                if (it.manual) {
                    snprintf(mTip[i], sizeof(mTip[i]), "Logged %s %s", kDot, Coach::dayShort(it.weekday));
                } else {
                    snprintf(mTip[i], sizeof(mTip[i]), "%u min %s %s", u(it.minutes), kDot, from);
                }
                break;
            case Streak::Status::TooShort:
                snprintf(mTip[i], sizeof(mTip[i]), "%u min %s under %u", u(it.minutes), kDot, u(w.minMinutes));
                break;
            case Streak::Status::Excluded:
                snprintf(mTip[i], sizeof(mTip[i]), "Excluded %s %s", kDot, from);
                break;
            case Streak::Status::OutOfScope:
                snprintf(mTip[i], sizeof(mTip[i]), "Goal: %s", Coach::scopeName(w.scope));
                break;
            case Streak::Status::SameDay:
                snprintf(mTip[i], sizeof(mTip[i]), "One a day %s %s", kDot, Coach::dayShort(it.weekday));
                break;
        }
        mItems[i]          = { Style::Tip, mText[i] };
        mItems[i].tip      = mTip[i];
        mItems[i].tipColor = st == Streak::Status::Counts ? Palette::kWin : Palette::kTextSoft;
    }

    if (mCount == 0) {
        mItems[0]          = { Style::Tip, "Nothing yet" };
        mItems[0].tip      = "Record or log one";
        mItems[0].tipColor = Palette::kTextSoft;
    }
}

void WeekScreen::build()
{
    fill();
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "This week");
    mMenu    = std::make_unique<Widgets::Wheel>(mRoot, mItems, mCount > 0 ? mCount : 1);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  mCount > 0 ? Widgets::Buttons::AMBER : Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
}

void WeekScreen::onShow()
{
    mMenu->select(mModel.weekAt < mMenu->count() ? mModel.weekAt : 0);
    mModel.resetIdleTimer();
}

void WeekScreen::onHide()
{
    mModel.weekAt = mMenu->selected();
}

void WeekScreen::onWeek()
{
    // A new list (after an undo or exclude): rebuild to match its length.
    ScreenManager::instance().goTo(ScreenId::Week);
}

void WeekScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            if (mCount > 0) {
                mModel.confirmIndex = static_cast<uint8_t>(mMenu->selected());
                ScreenManager::instance().goTo(ScreenId::Confirm);
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Menu);
            break;
        default:
            break;
    }
}
