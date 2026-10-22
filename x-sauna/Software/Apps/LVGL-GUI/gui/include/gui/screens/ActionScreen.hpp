/**
 ******************************************************************************
 * @file    ActionScreen.hpp
 * @brief   The action menu, opened with R1 on the live screen: Resume, Pause
 *          (or Resume timer), End session, Discard.
 *
 * The session keeps running while it is open, as in Race: pausing is its own
 * choice. End and Discard are unrecoverable, so they are not a click: R1
 * pressed on either opens the hold-to-confirm screen. It closes itself after
 * ten seconds and never exits the app on idle.
 ******************************************************************************
 */

#ifndef ACTION_SCREEN_HPP
#define ACTION_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class ActionScreen : public Screen
{
public:
    explicit ActionScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override;
    void onSessionUpdate(const Sauna::SessionSnapshot& snapshot) override;

protected:
    void build() override;

private:
    enum Row : uint16_t { RowResume, RowPause, RowEnd, RowDiscard, RowCount };

    void confirm();
    void refreshItems();

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // ACTION_SCREEN_HPP
