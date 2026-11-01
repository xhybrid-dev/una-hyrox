/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Before a workout: Start, the kind of workout, and its settings.
 *
 * L1 up, L2 down, R1 selects, R2 exits the app. Start begins. Type steps
 * round the kinds (Free, Strength, EMOM, AMRAP, Tabata, Intervals, Circuit;
 * remembered). The third row is the kind's: for Free it is the Auto lap (Off,
 * 1, 5, 10, 15, 20 or 30 minutes, the SDK Workout example's choices), stepped
 * by R1; for a plan it is Settings, with the plan in a line as its hint, and
 * R1 opens the kind's numbers (Tabata has none: 8 x 20/10 is fixed). Start's hint is the heart-rate source: the
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
    enum Row : uint16_t { RowStart, RowType, RowThird, RowCount };

    void confirm();
    void refresh();

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // MAIN_SCREEN_HPP
