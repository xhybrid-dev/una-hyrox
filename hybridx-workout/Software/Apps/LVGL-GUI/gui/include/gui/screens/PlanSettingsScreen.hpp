/**
 ******************************************************************************
 * @file    PlanSettingsScreen.hpp
 * @brief   The numbers of the chosen kind of workout, on the wheel: each row a
 *          setting with its value as the hint (PlanText.hpp: planFields).
 *
 * L1 up, L2 down, R1 steps the setting to its next choice (the service clamps
 * and saves it, and answers with the new value), R2 back to the main menu.
 * Exits the app on idle, as every menu screen does. A kind with nothing to
 * set (Free, Tabata) never opens it; if it is opened anyway it goes back.
 ******************************************************************************
 */

#ifndef PLAN_SETTINGS_SCREEN_HPP
#define PLAN_SETTINGS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

#include "PlanText.hpp"

class PlanSettingsScreen : public Screen
{
public:
    explicit PlanSettingsScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override;
    void onSettings() override;

protected:
    void build() override;

private:
    static constexpr uint8_t kMaxRows = 3;

    void refresh();

    Workout::PlanField mFields[kMaxRows] {};
    uint8_t            mCount = 0;

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // PLAN_SETTINGS_SCREEN_HPP
