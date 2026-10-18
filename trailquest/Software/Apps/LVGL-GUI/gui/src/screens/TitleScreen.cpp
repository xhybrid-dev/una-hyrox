/**
 ******************************************************************************
 * @file    TitleScreen.cpp
 * @brief   The title (see TitleScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TitleScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "Layout.hpp"
#include "Narrate.hpp"
#include "gui/Sprites.hpp"
#include "gui/screens/ScreenManager.hpp"

namespace Draw  = SDK::LVGL::Draw;
namespace Btn   = SDK::GUI::Button;
namespace Label = TrailQuest::Narrate::Label;
using SDK::LVGL::Buttons;
using namespace TrailQuest;

void TitleScreen::build()
{
    // A strip of the valley behind the three of them.
    Draw::box(mRoot, 0, 84, 240, 26, 0x0040C0);
    Draw::box(mRoot, 0, 106, 240, 16, 0x008000);
    Draw::box(mRoot, 0, 120, 240, 12, 0x804000);

    Ui::rowLabel(mRoot, 18, "TRAIL", Ui::Ink::kGold);
    Ui::rowLabel(mRoot, 36, "QUEST", Ui::Ink::kGold);
    mRunner.create(mRoot, 46, 76, TRAILQUEST_SPRITE_RUNNER, SpriteBuffers::small(0), SpriteBuffers::kSmall);
    mSpirit.create(mRoot, 104, 78, TRAILQUEST_SPRITE_STITCH, SpriteBuffers::small(1), SpriteBuffers::kSmall);
    mBear.create(mRoot, 152, 78, TRAILQUEST_SPRITE_BONK, SpriteBuffers::small(2), SpriteBuffers::kSmall);

    mStatus = Ui::rowLabel(mRoot, 136, mStatusText);
    mCorners.create(mRoot);
    mBeat = Ui::rowLabel(mRoot, 178, mBeatText, Ui::Ink::kDim);
    mHint = Ui::rowLabel(mRoot, 196, "Hold R2", Ui::Ink::kDim);
    // Under the corner labels' rows the face is narrow: no label there but
    // the hint, which says how to leave.
    refresh();
}

void TitleScreen::onShow()
{
    // The service sends the world when the GUI starts; ask as well, in case
    // that came before the Model was listening.
    if (!mModel.hasWorld()) {
        mModel.hello();
    }
}

void TitleScreen::refresh()
{
    const bool known = mModel.hasWorld();
    const bool saved = known && mModel.world().hasGame;
    if (!known) {
        std::snprintf(mStatusText, sizeof(mStatusText), "...");
    } else if (saved) {
        std::snprintf(mStatusText, sizeof(mStatusText), "Level %u", static_cast<unsigned>(mModel.world().state.level));
    } else {
        std::snprintf(mStatusText, sizeof(mStatusText), "A new run");
    }
    lv_label_set_text_static(mStatus, mStatusText);
    std::snprintf(mBeatText, sizeof(mBeatText), "Beat: %s", mModel.beatOn() ? "on" : "off");
    lv_label_set_text_static(mBeat, mBeatText);

    mCorners.set(saved ? Label::kNew : nullptr, known ? Label::kPlay : nullptr, Label::kBeat, nullptr);
    setArcs(saved ? Buttons::AMBER : Buttons::NONE, Buttons::WHITE, known ? Buttons::GREEN : Buttons::NONE, Buttons::RED);
}

void TitleScreen::onWorld()
{
    if (mStarting && !mModel.waiting()) {
        mStarting = false;
        ScreenManager::instance().goTo(ScreenManager::screenFor(mModel.world()));
        return;
    }
    refresh();
}

void TitleScreen::startNew()
{
    mStarting = true;
    mModel.newGame();
    mModel.playFx(FxId::Perfect);
}

void TitleScreen::onKey(uint8_t code)
{
    if (!mModel.hasWorld() || mStarting) {
        return;
    }
    const bool saved = mModel.world().hasGame;
    const uint32_t now = lv_tick_get();
    switch (code) {
        case Btn::R1:
            if (saved) {
                ScreenManager::instance().goTo(ScreenManager::screenFor(mModel.world()));
            } else {
                startNew();
            }
            break;
        case Btn::L1_PRESS:
            if (saved) {
                mNew.press(now);
            }
            break;
        case Btn::L1_RELEASE:
            if (mNew.release(now) && saved) {
                // A quick press: say how to start again, for two seconds.
                std::snprintf(mStatusText, sizeof(mStatusText), "Hold for new");
                lv_label_set_text_static(mStatus, mStatusText);
                mHintUntil = now + 2000u;
            }
            break;
        case Btn::L2:
            mModel.setBeat(!mModel.beatOn());
            refresh();
            break;
        default:
            break;
    }
}

void TitleScreen::onFrame(uint32_t nowMs)
{
    if (mNew.poll(nowMs)) {
        startNew();
    }
    if (mHintUntil != 0 && static_cast<int32_t>(nowMs - mHintUntil) >= 0) {
        mHintUntil = 0;
        refresh();
    }
    if (nowMs - mLastFlip >= 500) {
        mLastFlip = nowMs;
        mFrame ^= 1u;
        mRunner.setFrame(mFrame);
        mSpirit.setFrame(mFrame ^ 1u);
        mBear.setFrame(mFrame);
    }
}

void TitleScreen::onSuspend()
{
    Screen::onSuspend();
    mNew.cancel();
}
