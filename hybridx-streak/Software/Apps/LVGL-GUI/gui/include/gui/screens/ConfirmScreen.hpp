/**
 ******************************************************************************
 * @file    ConfirmScreen.hpp
 * @brief   Undo a manual log, or exclude / include a recorded session: tick or cross.
 ******************************************************************************
 */

#ifndef STREAK_CONFIRMSCREEN_HPP
#define STREAK_CONFIRMSCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class ConfirmScreen : public Screen
{
public:
    explicit ConfirmScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_CONFIRMSCREEN_HPP
