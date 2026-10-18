/**
 ******************************************************************************
 * @file    StoryScreen.cpp
 * @brief   A few pages of story (see StoryScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/StoryScreen.hpp"

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "Layout.hpp"
#include "Narrate.hpp"
#include "gui/Art.hpp"
#include "gui/Sprites.hpp"
#include "gui/screens/ScreenManager.hpp"

namespace Draw  = SDK::LVGL::Draw;
namespace Btn   = SDK::GUI::Button;
namespace Label = TrailQuest::Narrate::Label;
using SDK::LVGL::Buttons;
using namespace TrailQuest;

StoryPlan StoryScreen::sPlan;
uint8_t   StoryScreen::sBossSeen = 0;

namespace
{
constexpr int32_t kGap = 8;   ///< Between the runner and whoever stands with them, px.
} // namespace

void StoryScreen::play(const StoryPlan& plan)
{
    sPlan = plan;
    ScreenManager::instance().goTo(ScreenId::Story);
}

bool StoryScreen::firstSightOfBoss(uint8_t stage)
{
    const uint8_t bit = static_cast<uint8_t>(1u << (stage < 8 ? stage : 7));
    if ((sBossSeen & bit) != 0) {
        return false;
    }
    sBossSeen = static_cast<uint8_t>(sBossSeen | bit);
    return true;
}

void StoryScreen::build()
{
    // The scene: the stage's sky and ground, under the card.
    const uint8_t   stage = sPlan.stage < kStageCount ? sPlan.stage : 0u;
    const StageDef& st    = stageDef(stage);
    mSky    = Draw::box(mRoot, 0, Layout::kSceneTop, 240, Layout::kSceneGround - Layout::kSceneTop,
                        Art::skyFor(static_cast<uint8_t>(st.weather[0]), false));
    mGround = Draw::box(mRoot, 0, Layout::kSceneGround, 240, 240 - Layout::kSceneGround,
                        Art::groundFor(stage, static_cast<uint8_t>(TimeOfDay::Day)));
    mRunner.create(mRoot, 0, 0, TRAILQUEST_SPRITE_RUNNER, SpriteBuffers::small(0), SpriteBuffers::kSmall);
    mWho.create(mRoot, 0, 0, TRAILQUEST_SPRITE_FOX, SpriteBuffers::large(), SpriteBuffers::kLarge);

    mHead = Ui::rowLabel(mRoot, Layout::kHeadRow, "", Ui::Ink::kGold);
    mCard.create(mRoot);
    mCorners.create(mRoot);
    setArcs(Buttons::AMBER, Buttons::NONE, Buttons::GREEN, Buttons::RED);
}

void StoryScreen::onShow()
{
    mPage = 0;
    showPage();
}

void StoryScreen::showPage()
{
    const StoryPage* p = sPlan.page(mPage);
    if (p == nullptr) {
        finish();
        return;
    }
    Ui::setText(mHead, p->head, Ui::Ink::kGold);
    TextFit::Lines lines;
    Narrate::storyPage(lines, *p);
    mCard.show(lines, Ui::Ink::kText, Ui::Ink::kText);
    mCorners.raise();

    // The runner on the left, facing whoever the page is about; alone, in
    // the middle. Both stand on the ground.
    const int32_t runner = mRunner.size();
    if (p->who == kNone) {
        Draw::setHidden(mWho.object(), true);
        lv_obj_set_pos(mRunner.object(), 120 - runner / 2, Layout::kSceneGround - runner);
    } else {
        mWho.show(Art::spriteFor(p->who));
        const int32_t who  = mWho.size();
        const int32_t left = 120 - (runner + kGap + who) / 2;
        lv_obj_set_pos(mRunner.object(), left, Layout::kSceneGround - runner);
        lv_obj_set_pos(mWho.object(), left + runner + kGap, Layout::kSceneGround - who);
    }
    const bool last = mPage + 1u >= sPlan.pageCount();
    mCorners.set(last ? nullptr : Label::kSkip, last ? Label::kOk : Label::kNext, nullptr, nullptr);
    setArcs(last ? Buttons::NONE : Buttons::AMBER, Buttons::NONE, Buttons::GREEN, Buttons::RED);
}

void StoryScreen::finish()
{
    if (mDone) {
        return;
    }
    mDone = true;
    ScreenManager::instance().goTo(ScreenManager::screenFor(mModel.world()));
}

void StoryScreen::onKey(uint8_t code)
{
    if (mDone) {
        return;
    }
    switch (code) {
        case Btn::R1:
            ++mPage;
            showPage();
            break;
        case Btn::L1:
            finish();
            break;
        default:
            break;
    }
}

void StoryScreen::onFrame(uint32_t nowMs)
{
    if (nowMs - mLastFlip >= 500) {
        mLastFlip = nowMs;
        mFrame ^= 1u;
        mRunner.setFrame(mFrame);
        mWho.setFrame(mFrame ^ 1u);
    }
}
