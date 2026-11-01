/**
 ******************************************************************************
 * @file    HoldConfirmScreen.cpp
 * @brief   Hold to end or discard (see HoldConfirmScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/HoldConfirmScreen.hpp"

#include <cstdio>

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;

HoldConfirmScreen::HoldConfirmScreen(Model& model)
    : Screen(model)
{
}

HoldConfirmScreen::~HoldConfirmScreen()
{
    lv_anim_delete(this, nullptr);
}

void HoldConfirmScreen::build()
{
    mMode = mModel.holdMode();
    const bool end = mMode == Model::HoldMode::End;

    mRing = std::make_unique<Widgets::TimerRing>(mRoot);
    mRing->setColor(end ? Color::CHARTREUSE : Color::RED);

    // R1 confirms (green end / red discard). No back hint: releasing R1 cancels.
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  end ? Widgets::Buttons::GREEN : Widgets::Buttons::RED, Widgets::Buttons::NONE);

    Theme::label(mRoot, Theme::Font::Medium18, end ? Strings::kHoldEnd : Strings::kHoldDiscard, 53, 52, 133);
    mNumber = Theme::label(mRoot, Theme::Font::Light60, "3", 0, 96, 240);
}

void HoldConfirmScreen::onShow()
{
    mModel.setIdleTimeout();

    // The hold began on the action menu (R1 press); count down right away.
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, 0, static_cast<int32_t>(kHoldMs));
    lv_anim_set_duration(&a, kHoldMs);
    lv_anim_set_exec_cb(&a, &HoldConfirmScreen::animExecCb);
    lv_anim_set_completed_cb(&a, &HoldConfirmScreen::animReadyCb);
    lv_anim_start(&a);
}

void HoldConfirmScreen::onKey(uint8_t code)
{
    // Releasing R1 before the countdown completes cancels back to the menu.
    if (code == SDK::GUI::Button::R1_RELEASE) {
        cancel();
    }
}

void HoldConfirmScreen::onIdleTimeout()
{
    cancel();
}

void HoldConfirmScreen::onSuspend()
{
    // The release cannot reach this screen while the GUI is suspended and the
    // countdown runs on the wall clock, so left alone it would complete by
    // itself on resume. A suspend mid-hold cancels, like a release would.
    cancel();
}

void HoldConfirmScreen::cancel()
{
    if (mFired) {
        return;
    }
    lv_anim_delete(this, nullptr);
    ScreenManager::instance().goTo(ScreenId::Action);
}

void HoldConfirmScreen::animExecCb(void* var, int32_t value)
{
    auto* self = static_cast<HoldConfirmScreen*>(var);
    if (value < 0) {
        value = 0;
    }
    self->mRing->setProgress(static_cast<uint32_t>(value) * 1000u / kHoldMs);

    // 0..kHoldMs maps onto 3, 2, 1; completion fires before 0 would show.
    uint32_t number = 3 - (static_cast<uint32_t>(value) / (kHoldMs / 3));
    if (number < 1) {
        number = 1;
    }
    self->setCountdown(number);
}

void HoldConfirmScreen::animReadyCb(lv_anim_t* a)
{
    auto* self = static_cast<HoldConfirmScreen*>(a->var);
    if (self->mFired) {
        return;
    }
    self->mFired = true;

    // Ending saves the session; discarding throws it away. The result screen
    // waits for the service to say which it was.
    self->mModel.stop(self->mMode == Model::HoldMode::End);
    ScreenManager::instance().goTo(ScreenId::Result);
}

void HoldConfirmScreen::setCountdown(uint32_t number)
{
    if (number != mShown) {
        mShown = number;
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(number));
        Theme::assign(mNumber, buf);
    }
}
