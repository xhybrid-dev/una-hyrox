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
#include "gui/screens/StoryScreen.hpp"

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

bool ClearScreen::finished() const
{
    return mModel.world().phase == RunPhase::Finished;
}

void ClearScreen::onShow()
{
    TextFit::Lines lines;
    if (finished()) {
        Narrate::finale(lines, mModel.world());
        Ui::setText(mHead, "THE SUMMIT", Ui::Ink::kGold);
        mCorners.set(nullptr, Label::kAgain, Label::kBack, nullptr);
        setArcs(Buttons::NONE, Buttons::WHITE, Buttons::GREEN, Buttons::RED);
    } else {
        Narrate::stageClear(lines, mModel.world());
        Ui::setText(mHead, "WELL RUN", Ui::Ink::kGold);
        mCorners.set(nullptr, Label::kGo, nullptr, nullptr);
        setArcs(Buttons::NONE, Buttons::NONE, Buttons::GREEN, Buttons::RED);
    }
    mCard.show(lines);
    mModel.playFx(FxId::Win);
}

void ClearScreen::onKey(uint8_t code)
{
    if (mWaiting) {
        return;
    }
    if (code == Btn::L2 && finished()) {
        ScreenManager::instance().goTo(ScreenId::Title);   // the save stays at the summit
    } else if (code == Btn::R1) {
        mWaiting = true;   // the next stage, or (at the summit) the next lap
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
    const WorldView& w = mModel.world();
    if (w.phase == RunPhase::Finished) {
        onShow();   // the summit's clear card led on to the finale
        return;
    }
    if (w.cause == ViewCause::StageStarted) {
        StoryScreen::play(StoryPlan::of(StoryId::StageStart, w.state.stage));
        return;
    }
    ScreenManager::instance().goTo(ScreenManager::screenFor(w));
}

void ClearScreen::onFrame(uint32_t nowMs)
{
    if (nowMs - mLastFlip >= 300) {
        mLastFlip = nowMs;
        mFrame ^= 1u;
        mRunner.setFrame(mFrame);
    }
}
