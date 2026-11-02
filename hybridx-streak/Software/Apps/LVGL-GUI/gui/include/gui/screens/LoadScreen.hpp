/**
 ******************************************************************************
 * @file    LoadScreen.hpp
 * @brief   Training load: form and its word, fitness, fatigue, this week's and
 *          today's load, from every HybridX app's sessions (LoadHistory.hpp).
 *
 * A wheel, as the trophy case: L1 and L2 move, R2 back to the menu. Form is
 * amber when it says to ease off, white otherwise (lime is for what is earned,
 * DESIGN 2). For the first two weeks it says it is learning.
 ******************************************************************************
 */

#ifndef STREAK_LOADSCREEN_HPP
#define STREAK_LOADSCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class LoadScreen : public Screen
{
public:
    explicit LoadScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onLoad() override;

protected:
    void build() override;

private:
    static constexpr uint8_t kItems = 5;   // form, fitness, fatigue, this week, today

    void fill();

    Widgets::Wheel::Item             mItems[kItems] {};
    char                             mTip[kItems][32] {};
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Wheel>   mMenu;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_LOADSCREEN_HPP
