/**
 ******************************************************************************
 * @file    MenuVo2ProfileScreen.hpp
 * @brief   HybridX Run: the athlete's numbers behind VO2max (R5), from the
 *          VO2max page's R1.
 *
 * Birth date, max HR and resting HR (each opens its picker) and the tuning
 * log (R1 toggles it). Each item's tip shows the value in use and where it
 * comes from. Built as RunLVGL's lap-alerts menu (MenuAlertsScreen).
 ******************************************************************************
 */

#ifndef MENU_VO2_PROFILE_SCREEN_HPP
#define MENU_VO2_PROFILE_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuVo2ProfileScreen : public Screen
{
public:
    explicit MenuVo2ProfileScreen(Model& model);

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onVo2Info(const CustomMessage::Vo2Info& info) override;
    void onIdleTimeout() override { mModel.exitApp(); }

protected:
    void build() override;

private:
    using Menu = App::MenuNav::Root::Vo2Profile;

    void refreshTips();

    WheelMenu::Item mItems[Menu::ID_COUNT] {};
    char mBirthTip[24] {};
    char mMaxTip[24] {};
    char mRestTip[24] {};

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // MENU_VO2_PROFILE_SCREEN_HPP
