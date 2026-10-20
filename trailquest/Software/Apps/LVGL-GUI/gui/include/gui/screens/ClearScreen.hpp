/**
 ******************************************************************************
 * @file    ClearScreen.hpp
 * @brief   A stage's boss is beaten.
 *
 *   R1  on to the next stage, or (at the summit) the next lap
 *   L2  (at the summit) back to the title
 ******************************************************************************
 */

#ifndef TRAILQUEST_CLEAR_SCREEN_HPP
#define TRAILQUEST_CLEAR_SCREEN_HPP

#include "gui/Sprite.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Ui.hpp"

class ClearScreen : public Screen
{
public:
    explicit ClearScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onWorld() override;

protected:
    void build() override;
    void onKey(uint8_t code) override;
    void onFrame(uint32_t nowMs) override;

private:
    bool finished() const;

    Sprite      mRunner;
    lv_obj_t*   mHead = nullptr;
    Ui::Card    mCard;
    Ui::Corners mCorners;
    bool        mWaiting  = false;
    uint32_t    mLastFlip = 0;
    uint8_t     mFrame    = 0;
};

#endif // TRAILQUEST_CLEAR_SCREEN_HPP
