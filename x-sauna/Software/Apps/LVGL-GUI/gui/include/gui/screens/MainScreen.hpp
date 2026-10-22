/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Before a session: Start, Mode and Rest on the wheel.
 *
 * L1 up, L2 down, R1 selects (Start begins; Mode and Rest change in place and
 * the hint line shows the choice), R2 exits the app. Exits on idle. If a
 * session is already running (the screen was closed and reopened) it goes
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
    void onSessionUpdate(const Sauna::SessionSnapshot& snapshot) override;

protected:
    void build() override;

private:
    enum Row : uint16_t { RowStart, RowMode, RowRest, RowCount };

    bool restLocked() const;
    void confirm();
    void refresh();

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // MAIN_SCREEN_HPP
