/**
 ******************************************************************************
 * @file    LoadScreen.cpp
 * @brief   Training load (see the header).
 ******************************************************************************
 */

#include "gui/screens/LoadScreen.hpp"

#include <cstdio>

#include "FormText.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Style = Widgets::Wheel::Item::Style;

namespace
{
unsigned u(uint32_t v) { return static_cast<unsigned>(v); }
} // namespace

void LoadScreen::fill()
{
    const CustomMessage::LoadData& d = mModel.load();
    const auto word = static_cast<Streak::FormWord>(d.word);
    const bool known = word != Streak::FormWord::Learning && word != Streak::FormWord::NoLoad;

    // Form: the number and the word once there is two weeks' history; else what is happening.
    if (known) {
        char n[8];
        Streak::formatFormNumber(n, sizeof(n), d.form);
        snprintf(mTip[0], sizeof(mTip[0]), "%s, %s", n, Streak::formWordName(word));
    } else {
        Streak::formatFormLine(mTip[0], sizeof(mTip[0]), d.form, word, d.historyDays);
    }
    mItems[0]          = { Style::Tip, "Form" };
    mItems[0].tip      = mTip[0];
    mItems[0].tipColor = word == Streak::FormWord::Overreaching ? Palette::kAtRisk : Palette::kText;

    snprintf(mTip[1], sizeof(mTip[1]), "%u, the 6-week level", u(d.fitness));
    mItems[1]          = { Style::Tip, "Fitness" };
    mItems[1].tip      = mTip[1];
    mItems[1].tipColor = Palette::kTextSoft;

    snprintf(mTip[2], sizeof(mTip[2]), "%u, the last week", u(d.fatigue));
    mItems[2]          = { Style::Tip, "Fatigue" };
    mItems[2].tip      = mTip[2];
    mItems[2].tipColor = Palette::kTextSoft;

    snprintf(mTip[3], sizeof(mTip[3]), "Load %u", u(d.weekLoad));
    mItems[3]          = { Style::Tip, "This week" };
    mItems[3].tip      = mTip[3];
    mItems[3].tipColor = Palette::kTextSoft;

    snprintf(mTip[4], sizeof(mTip[4]), "Load %u", u(d.todayLoad));
    mItems[4]          = { Style::Tip, "Today" };
    mItems[4].tip      = mTip[4];
    mItems[4].tipColor = Palette::kTextSoft;
}

void LoadScreen::build()
{
    fill();
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "Training load");
    mMenu    = std::make_unique<Widgets::Wheel>(mRoot, mItems, kItems);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
}

void LoadScreen::onShow()
{
    mMenu->select(mModel.loadAt < kItems ? mModel.loadAt : 0);
    mModel.resetIdleTimer();
}

void LoadScreen::onHide()
{
    mModel.loadAt = mMenu->selected();
}

void LoadScreen::onLoad()
{
    fill();
    mMenu->refresh();
}

void LoadScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::Menu); break;
        default: break;
    }
}
