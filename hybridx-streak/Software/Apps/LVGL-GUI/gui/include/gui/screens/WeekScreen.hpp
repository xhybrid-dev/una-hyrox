/**
 ******************************************************************************
 * @file    WeekScreen.hpp
 * @brief   This week: every session, where it came from, and why it counts or not.
 ******************************************************************************
 */

#ifndef STREAK_WEEKSCREEN_HPP
#define STREAK_WEEKSCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class WeekScreen : public Screen
{
public:
    explicit WeekScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onWeek() override;
protected:
    void build() override;

private:
    static constexpr uint8_t kMaxItems = CustomMessage::WeekData::kMax;

    void fill();

    Widgets::Wheel::Item             mItems[kMaxItems] {};
    char                             mText[kMaxItems][24] {};
    char                             mTip[kMaxItems][40] {};
    uint8_t                          mCount = 0;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Wheel>   mMenu;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_WEEKSCREEN_HPP
