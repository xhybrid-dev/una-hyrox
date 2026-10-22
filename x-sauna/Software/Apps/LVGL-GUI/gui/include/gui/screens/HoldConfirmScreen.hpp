/**
 ******************************************************************************
 * @file    HoldConfirmScreen.hpp
 * @brief   Hold to end or discard: a ring fills over 1.5 s while R1 stays
 *          held and the centre counts 3, 2, 1. Releasing early returns to the
 *          action menu; completing ends the session (saved) or discards it.
 *
 * As Race's. The ring is driven by an lv_anim; a suspend cancels, as a release
 * would (the release cannot reach a suspended screen).
 ******************************************************************************
 */

#ifndef HOLD_CONFIRM_SCREEN_HPP
#define HOLD_CONFIRM_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class HoldConfirmScreen : public Screen
{
public:
    explicit HoldConfirmScreen(Model& model);
    ~HoldConfirmScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override;
    void onSuspend() override;

protected:
    void build() override;

private:
    static constexpr uint32_t kHoldMs = 1500;

    static void animExecCb(void* var, int32_t value);
    static void animReadyCb(lv_anim_t* a);
    void setCountdown(uint32_t number);
    void cancel();

    Model::HoldMode mMode  = Model::HoldMode::End;
    bool            mFired = false;
    uint32_t        mShown = 3;

    lv_obj_t* mNumber = nullptr;
    std::unique_ptr<Widgets::TimerRing> mRing;
    std::unique_ptr<Widgets::Buttons>   mButtons;
};

#endif // HOLD_CONFIRM_SCREEN_HPP
