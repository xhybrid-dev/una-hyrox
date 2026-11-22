/**
 ******************************************************************************
 * @file    MenuScreen.hpp
 * @brief   The menu: This week, By sport, Your numbers, Log a session, Trophy
 *          case, Training load, Settings.
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
    void onDash() override;

protected:
    void build() override;

private:
    enum : uint8_t { kWeek, kSport, kNumbers, kLog, kTrophy, kLoad, kSettings, kCount };

    Widgets::Wheel::Item             mItems[kCount] {};
    char                             mWeekTip[32] {};
    char                             mLoadTip[32] {};
    char                             mSportTip[48] {};
    char                             mNumbersTip[48] {};
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Wheel>   mMenu;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_MENUSCREEN_HPP
