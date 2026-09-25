/**
 ******************************************************************************
 * @file    TrophyScreen.cpp
 * @brief   The trophy case (see the header).
 *
 * The five summits (DESIGN 5), climbed or with the weeks still to go; the
 * four session badges; then the best week, the longest streak, and every
 * session counted. Earned things are lime.
 ******************************************************************************
 */

#include "gui/screens/TrophyScreen.hpp"

#include <cstdio>

#include "Summits.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Style = Widgets::Wheel::Item::Style;

namespace
{
constexpr const char* kDot = "\xC2\xB7";
unsigned u(uint32_t v) { return static_cast<unsigned>(v); }
} // namespace

void TrophyScreen::fill()
{
    const CustomMessage::TrophyData& t = mModel.trophies();
    uint8_t                          n = 0;

    for (uint8_t c = 0; c < kSummits && c < Streak::kClimbCount; ++c, ++n) {
        const Streak::Climb& climb = Streak::kClimbs[c];
        const bool           done  = t.weeksAchieved >= climb.summitAt;
        if (done && c == Streak::kClimbCount - 1 && t.weeksAchieved >= climb.summitAt + Streak::kRepeatSteps) {
            snprintf(mTip[n], sizeof(mTip[n]), "Climbed x%u", u(1 + (t.weeksAchieved - climb.summitAt) / Streak::kRepeatSteps));
        } else if (done) {
            snprintf(mTip[n], sizeof(mTip[n]), "Climbed %s %u weeks", kDot, u(climb.summitAt));
        } else {
            snprintf(mTip[n], sizeof(mTip[n]), "%u of %u weeks", u(t.weeksAchieved), u(climb.summitAt));
        }
        mItems[n]          = { Style::Tip, climb.name };
        mItems[n].tip      = mTip[n];
        mItems[n].tipColor = done ? Palette::kWin : Palette::kTextSoft;
    }
    for (uint8_t b = 0; b < kBadges; ++b, ++n) {
        const Streak::SessionBadge& badge = Streak::kSessionBadges[b];
        const bool                  done  = (t.badges & (1u << b)) != 0;
        if (done) {
            snprintf(mTip[n], sizeof(mTip[n]), "Earned %s %u sessions", kDot, u(badge.sessions));
        } else {
            snprintf(mTip[n], sizeof(mTip[n]), "%u of %u sessions", u(t.lifetime), u(badge.sessions));
        }
        mItems[n]          = { Style::Tip, badge.name };
        mItems[n].tip      = mTip[n];
        mItems[n].tipColor = done ? Palette::kWin : Palette::kTextSoft;
    }
    snprintf(mTip[n], sizeof(mTip[n]), "%u session%s", u(t.bestWeek), t.bestWeek == 1 ? "" : "s");
    mItems[n]          = { Style::Tip, "Best week" };
    mItems[n].tip      = mTip[n];
    mItems[n].tipColor = Palette::kWin;
    ++n;
    snprintf(mTip[n], sizeof(mTip[n]), "%u week%s", u(t.longest), t.longest == 1 ? "" : "s");
    mItems[n]          = { Style::Tip, "Longest streak" };
    mItems[n].tip      = mTip[n];
    mItems[n].tipColor = Palette::kWin;
    ++n;
    snprintf(mTip[n], sizeof(mTip[n]), "%u counted", u(t.lifetime));
    mItems[n]          = { Style::Tip, "All sessions" };
    mItems[n].tip      = mTip[n];
    mItems[n].tipColor = Palette::kWin;
}

void TrophyScreen::build()
{
    fill();
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "Trophy case");
    mMenu    = std::make_unique<Widgets::Wheel>(mRoot, mItems, kItems);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
}

void TrophyScreen::onShow()
{
    mMenu->select(mModel.trophyAt < kItems ? mModel.trophyAt : 0);
    mModel.resetIdleTimer();
}

void TrophyScreen::onHide()
{
    mModel.trophyAt = mMenu->selected();
}

void TrophyScreen::onTrophies()
{
    fill();
    mMenu->refresh();
}

void TrophyScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::Menu); break;
        default: break;
    }
}
