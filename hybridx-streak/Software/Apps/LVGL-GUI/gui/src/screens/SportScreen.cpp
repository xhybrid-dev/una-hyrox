/**
 ******************************************************************************
 * @file    SportScreen.cpp
 * @brief   This week by sport (see the header).
 ******************************************************************************
 */

#include "gui/screens/SportScreen.hpp"

#include <cstdio>

#include "gui/copy/Coach.hpp"
#include "gui/copy/DashWords.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Style = Widgets::Wheel::Item::Style;

void SportScreen::fill()
{
    const CustomMessage::DashData& d = mModel.dash();
    mCount = 0;
    auto add = [&](const char* label, uint32_t colour) -> char* {
        const uint8_t i = mCount++;
        snprintf(mLabel[i], sizeof(mLabel[i]), "%s", label);
        mItems[i]          = { Style::Tip, mLabel[i] };
        mItems[i].tip      = mTip[i];
        mItems[i].tipColor = colour;
        return mTip[i];
    };

    // This week, all kinds. With nothing recorded, the manual logs (if any) still show below.
    char* t = add("This week", Palette::kText);
    if (d.week.all.sessions > 0) {
        DashWords::allTip(d.week.all, t, sizeof(mTip[0]));
    } else {
        snprintf(t, sizeof(mTip[0]), "%s", d.manual > 0 ? "Nothing recorded" : "Nothing yet");
    }
    if (d.week.all.metres > 0) {
        DashWords::distance(d.week.all.metres, add("Distance", Palette::kText), sizeof(mTip[0]));
    }

    // Each kind done this week, in the kinds' own order (the This week list's).
    for (uint8_t k = 0; k < Streak::kKindCount && mCount < kMaxItems - 2; ++k) {
        const Streak::SportSum& s = d.week.kind[k];
        if (s.sessions == 0) {
            continue;
        }
        const auto kind = static_cast<Streak::Kind>(k);
        DashWords::sportTip(kind, s, add(Coach::sportName(kind), Palette::kTextSoft), sizeof(mTip[0]));
    }

    if (d.manual > 0) {
        DashWords::manualTip(d.manual, add("By hand", Palette::kTextSoft), sizeof(mTip[0]));
    }

    t = add("Last week", Palette::kTextSoft);
    if (d.last.sessions > 0) {
        DashWords::allTip(d.last, t, sizeof(mTip[0]));
    } else {
        snprintf(t, sizeof(mTip[0]), "Nothing recorded");
    }
}

void SportScreen::build()
{
    fill();
    mTitle   = std::make_unique<Widgets::Title>(mRoot, DashWords::kSportTitle);
    mMenu    = std::make_unique<Widgets::Wheel>(mRoot, mItems, mCount);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
}

void SportScreen::onShow()
{
    mMenu->select(mModel.sportAt < mCount ? mModel.sportAt : 0);
    mModel.resetIdleTimer();
}

void SportScreen::onHide()
{
    mModel.sportAt = mMenu->selected();
}

void SportScreen::onDash()
{
    // The rows can change in number (a manual log adds "By hand"): rebuild, as This week does.
    // onHide keeps the place.
    ScreenManager::instance().goTo(ScreenId::Sport);
}

void SportScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::Menu); break;
        default: break;
    }
}
