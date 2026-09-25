/**
 ******************************************************************************
 * @file    MenuScreen.hpp
 * @brief   The menu: This week, Log a session, Trophy case, Settings.
 ******************************************************************************
 */

#ifndef STREAK_MENUSCREEN_HPP
#define STREAK_MENUSCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class MenuScreen : public Screen
{
public:
    explicit MenuScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onWeek() override;
    void onHomeView() override;
protected:
    void build() override;

private:
    enum : uint8_t { kWeek, kLog, kTrophy, kSettings, kCount };

    Widgets::Wheel::Item             mItems[kCount] {};
    char                             mWeekTip[32] {};
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Wheel>   mMenu;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_MENUSCREEN_HPP
