/**
 ******************************************************************************
 * @file    ShieldScreen.hpp
 * @brief   A week missed with a shield in hand: spend it, or let the streak go.
 *
 * An explicit, visible choice at the boundary, never a silent save (brief
 * section 3): R1 (green tick) spends a shield, R2 (white cross) declines and
 * leads to the fresh start.
 ******************************************************************************
 */

#ifndef STREAK_SHIELD_SCREEN_HPP
#define STREAK_SHIELD_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class ShieldScreen : public Screen
{
public:
    explicit ShieldScreen(Model& model);
    ~ShieldScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    void spend();
    static void doneCb(lv_timer_t* t);

    std::unique_ptr<Widgets::Buttons> mButtons;
    lv_obj_t*   mTitle = nullptr;
    lv_obj_t*   mBody  = nullptr;
    lv_obj_t*   mCount = nullptr;
    lv_obj_t*   mTick  = nullptr;
    lv_obj_t*   mCross = nullptr;
    lv_timer_t* mTimer = nullptr;
    bool        mSpent = false;
};

#endif // STREAK_SHIELD_SCREEN_HPP
