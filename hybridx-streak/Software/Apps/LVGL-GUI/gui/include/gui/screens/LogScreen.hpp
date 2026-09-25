/**
 ******************************************************************************
 * @file    LogScreen.hpp
 * @brief   Log a session the watch did not record: what, then today or yesterday (S7).
 ******************************************************************************
 */

#ifndef STREAK_LOGSCREEN_HPP
#define STREAK_LOGSCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class LogScreen : public Screen
{
public:
    explicit LogScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    Widgets::Wheel::Item             mItems[Streak::kKindCount] {};
    uint8_t                          mCount = 0;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Wheel>   mMenu;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_LOGSCREEN_HPP
