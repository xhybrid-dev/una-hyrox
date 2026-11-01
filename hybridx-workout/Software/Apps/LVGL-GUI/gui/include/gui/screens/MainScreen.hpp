/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Before a workout: Start and Auto lap on the wheel.
 *
 * L1 up, L2 down, R1 selects (Start begins; Auto lap steps through Off, 1, 5,
 * 10, 15, 20 and 30 minutes, the SDK Workout example's choices, and is
 * remembered), R2 exits the app. Start's hint is the heart-rate source: the
 * strap's state when the kernel reports one, else the wrist. Exits on idle. If
 * a workout is already running (the screen was closed and reopened) it goes
 * straight to the live screen.
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
    void onSessionUpdate(const Workout::SessionSnapshot& snapshot) override;
    void onSettings() override;
    void onStrap() override;

protected:
    void build() override;

private:
    enum Row : uint16_t { RowStart, RowAutoLap, RowCount };

    void confirm();
    void refresh();

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // MAIN_SCREEN_HPP
