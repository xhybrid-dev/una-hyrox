/**
 ******************************************************************************
 * @file    ValueScreen.hpp
 * @brief   The choices for one setting, as a wheel.
 ******************************************************************************
 */

#ifndef STREAK_VALUESCREEN_HPP
#define STREAK_VALUESCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class ValueScreen : public Screen
{
public:
    explicit ValueScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    static constexpr uint8_t kMaxChoices = 10;

    Widgets::Wheel::Item             mItems[kMaxChoices] {};
    char                             mText[kMaxChoices][20] {};
    uint8_t                          mValues[kMaxChoices] {};
    uint8_t                          mCount = 0;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Wheel>   mMenu;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_VALUESCREEN_HPP
