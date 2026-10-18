/**
 ******************************************************************************
 * @file    ClearScreen.cpp
 * @brief   A stage's boss is beaten (see ClearScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/ClearScreen.hpp"

#include "SDK/GUI/Button.hpp"

#include "Layout.hpp"
#include "Narrate.hpp"
#include "gui/Sprites.hpp"
#include "gui/screens/ScreenManager.hpp"

namespace Btn   = SDK::GUI::Button;
namespace Label = TrailQuest::Narrate::Label;
using SDK::LVGL::Buttons;
using namespace TrailQuest;

void ClearScreen::build()
{
    mHead = Ui::rowLabel(mRoot, Layout::kHeadRow, "", Ui::Ink::kGold);
    mCard.create(mRoot);
    // The runner, arms up, under the card.
    mRunner.create(mRoot, 96, 164, TRAILQUEST_SPRITE_RUNNER, SpriteBuffers::small(0), SpriteBuffers::kSmall);
    mCorners.create(mRoot);
}

bool ClearScreen::last() const
{
    const WorldView& w = mModel.world();
    return w.phase == RunPhase::StageClear && w.state.stage + 1u >= w.playableStages;
}

void ClearScreen::onShow()
{
    TextFit::Lines lines;
    Narrate::stageClear(lines, mModel.world(), last());
    mCard.show(lines);
    Ui::setText(mHead, "WELL RUN", Ui::Ink::kGold);
    mCorners.set(nullptr, last() ? Label::kOk : Label::kGo, nullptr, nullptr);
    setArcs(Buttons::NONE, Buttons::NONE, Buttons::GREEN, Buttons::RED);
    mModel.playFx(FxId::Win);
}

void ClearScreen::onKey(uint8_t code)
{
    if (code != Btn::R1 || mWaiting) {
        return;
    }
    if (last()) {
        ScreenManager::instance().goTo(ScreenId::Title);
    } else {
        mWaiting = true;
        mModel.advance();
    }
}

void ClearScreen::onWorld()
{
    if (!mWaiting || mModel.waiting()) {
        return;
    }
    mWaiting = false;
    if (mModel.lost()) {
        return;   // try again with R1
    }
    ScreenManager::instance().goTo(ScreenManager::screenFor(mModel.world()));
}

void ClearScreen::onFrame(uint32_t nowMs)
{
    if (nowMs - mLastFlip >= 300) {
        mLastFlip = nowMs;
        mFrame ^= 1u;
        mRunner.setFrame(mFrame);
    }
}
