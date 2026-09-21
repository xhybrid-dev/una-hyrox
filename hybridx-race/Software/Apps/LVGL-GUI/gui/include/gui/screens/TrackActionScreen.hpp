/**
 ******************************************************************************
 * @file    TrackActionScreen.hpp
 * @brief   In-race action menu (brief 8.2 item 5).
 *
 * Opened with R1 from the race screen. Unlike RunLVGL's equivalent it does NOT
 * pause the race on entry: brief 8.1 is explicit that the clock keeps running
 * while the menu is open, and pausing would corrupt a race whose athlete only
 * wanted to look at the options.
 *
 * It closes itself back to the race after ten seconds of no input, and never
 * exits the app on idle.
 ******************************************************************************
 */

#ifndef TRACK_ACTION_SCREEN_HPP
#define TRACK_ACTION_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class TrackActionScreen : public Screen
{
public:
    explicit TrackActionScreen(Model& model);

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override;

protected:
    void build() override;

private:
    using Menu = App::MenuNav::RaceView::Action;

    void confirm();
    void refreshItems();

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // TRACK_ACTION_SCREEN_HPP
