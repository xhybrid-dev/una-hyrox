/**
 ******************************************************************************
 * @file    ClockScreen.hpp
 * @brief   The watch has lost the time: nothing can be judged until it is set (PLAN 6.1).
 ******************************************************************************
 */

#ifndef STREAK_CLOCKSCREEN_HPP
#define STREAK_CLOCKSCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class ClockScreen : public Screen
{
public:
    explicit ClockScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onHomeView() override;
protected:
    void build() override;

private:
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_CLOCKSCREEN_HPP
