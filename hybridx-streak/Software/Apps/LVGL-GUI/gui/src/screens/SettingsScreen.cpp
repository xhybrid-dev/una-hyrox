/**
 ******************************************************************************
 * @file    SettingsScreen.cpp
 * @brief   The goal's settings (see the header).
 *
 * Each line shows its value; R1 opens its choices (one per day toggles in
 * place). A change waits for next week, except the week's start day, which
 * closes this week early (S8): the hint says so. The same settings are
 * editable from the phone (AppConfig); the service keeps the two in step.
 ******************************************************************************
 */

#include "gui/screens/SettingsScreen.hpp"

#include <cstdio>

#include "gui/copy/Coach.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Style = Widgets::Wheel::Item::Style;

namespace
{
constexpr const char* kDot = "\xC2\xB7";
unsigned u(uint32_t v) { return static_cast<unsigned>(v); }
} // namespace

Streak::Goal SettingsScreen::effective() const
{
    const CustomMessage::GoalData& g = mModel.goal();
    return g.hasPending ? g.pending : g.goal;
}

void SettingsScreen::fill()
{
    const CustomMessage::GoalData& g   = mModel.goal();
    const Streak::Goal             now = g.goal;
    const Streak::Goal             e   = effective();

    if (g.hasPending && e.target != now.target) {
        snprintf(mTip[kTarget], sizeof(mTip[kTarget]), "%u now %s %u next week", u(now.target), kDot, u(e.target));
    } else {
        snprintf(mTip[kTarget], sizeof(mTip[kTarget]), "%u a week", u(e.target));
    }
    snprintf(mTip[kWeekStart], sizeof(mTip[kWeekStart]), "%s", Coach::dayLong(e.weekStart));
    snprintf(mTip[kCounts], sizeof(mTip[kCounts]), "%s%s", Coach::scopeName(e.scope),
             (g.hasPending && e.scope != now.scope) ? " next week" : "");
    if (e.minMinutes == 0) {
        snprintf(mTip[kMinimum], sizeof(mTip[kMinimum]), "Any length");
    } else {
        snprintf(mTip[kMinimum], sizeof(mTip[kMinimum]), "%u min%s", u(e.minMinutes),
                 (g.hasPending && e.minMinutes != now.minMinutes) ? " next week" : "");
    }

    mItems[kTarget]    = { Style::Tip, "Weekly target" };
    mItems[kWeekStart] = { Style::Tip, "Week starts" };
    mItems[kCounts]    = { Style::Tip, "What counts" };
    mItems[kMinimum]   = { Style::Tip, "Shortest" };
    // Two lines in SemiBold 20 when selected: the toggle shares the line, and
    // "One per day" in 25 ran under it (RunLVGL splits its toggle the same way).
    mItems[kOnePerDay] = { Style::Toggle, "One per\nday", "One per day", Theme::font(Theme::Font::SemiBold20) };
    for (uint8_t i = 0; i < kOnePerDay; ++i) {
        mItems[i].tip      = mTip[i];
        mItems[i].tipColor = Palette::kWin;
    }
    mItems[kOnePerDay].toggleState = e.onePerDay;
}

void SettingsScreen::build()
{
    fill();
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "Settings");
    mMenu    = std::make_unique<Widgets::Wheel>(mRoot, mItems, kCount);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
}

void SettingsScreen::onShow()
{
    mMenu->select(mModel.settingsAt < kCount ? mModel.settingsAt : 0);
    mModel.resetIdleTimer();
}

void SettingsScreen::onHide()
{
    mModel.settingsAt = mMenu->selected();
}

void SettingsScreen::onGoal()
{
    fill();
    mMenu->refresh();
}

void SettingsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            if (mMenu->selected() == kOnePerDay) {
                Streak::Goal g = effective();
                g.onePerDay    = !g.onePerDay;
                mModel.setGoal(g);
                mItems[kOnePerDay].toggleState = g.onePerDay;   // shown now; the service confirms
                mMenu->refresh();
            } else {
                mModel.editField = static_cast<uint8_t>(mMenu->selected());
                ScreenManager::instance().goTo(ScreenId::Value);
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Menu);
            break;
        default:
            break;
    }
}
