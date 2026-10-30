/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Before a ride: Start, the ride type, the heart-rate source and the
 *          auto lap on the wheel.
 *
 * L1 up, L2 down, R1 selects, R2 exits the app:
 *   - Start begins. Its hint is the GPS outdoors (finding it, or ready), or
 *     "Indoor ride". Outdoors with no fix the first R1 only warns ("No GPS: R1
 *     again"); the second starts anyway;
 *   - Ride flips Outdoor and Indoor (remembered);
 *   - Heart rate only shows the source: the strap's state when the kernel
 *     reports one, else the wrist;
 *   - Auto lap steps through Off, 1, 5, 10, 15, 20 and 30 minutes, the SDK
 *     Workout example's choices (remembered).
 * Exits on idle. If a ride is already running (the screen was closed and
 * reopened) it goes straight to the live screen.
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
    void onSessionUpdate(const Ride::SessionSnapshot& snapshot) override;
    void onSettings() override;
    void onStrap() override;
    void onGps() override;

protected:
    void build() override;

private:
    enum Row : uint16_t { RowStart, RowRide, RowSensor, RowAutoLap, RowCount };

    void confirm();
    void refresh();

    bool mWarnedNoGps = false;   ///< The first R1 on Start said there is no GPS yet

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // MAIN_SCREEN_HPP
