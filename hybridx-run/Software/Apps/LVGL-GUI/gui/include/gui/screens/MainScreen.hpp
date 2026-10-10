/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Pre-activity menu: Start / Intervals / Settings on the wheel, with
 *          the sensor-status row and the app title.
 *
 * Port of the Run app's MainView + MainPresenter. Start needs a GPS fix; without
 * one it asks for confirmation first. Intervals and Settings are M2 work and
 * currently do nothing.
 ******************************************************************************
 */

#ifndef MAIN_SCREEN_HPP
#define MAIN_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MainScreen : public Screen
{
public:
    explicit MainScreen(Model& model);

    // Screen
    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    // ModelListener
    void onIdleTimeout() override;
    void onGpsFix(bool acquired) override;
    void onAccessoryStatus(uint8_t state, const char* name) override;
    void onVo2Info(const CustomMessage::Vo2Info& info) override;   // HybridX Run

protected:
    void build() override;

private:
    using Menu = App::MenuNav::Root;

    void confirm();
    void updateBackground();

    bool mGpsFix = false;

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
    lv_obj_t*                                 mVo2 = nullptr;   ///< HybridX Run: "VO2max 52.3"
};

#endif // MAIN_SCREEN_HPP
