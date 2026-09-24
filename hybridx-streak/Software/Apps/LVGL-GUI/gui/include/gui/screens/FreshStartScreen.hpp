/**
 ******************************************************************************
 * @file    FreshStartScreen.hpp
 * @brief   The streak ended, and the climb is safe: a sunrise, not a scolding.
 *
 * Progress up the mountains is cumulative (PLAN S10), so this screen can say
 * exactly how far up the athlete still is.
 ******************************************************************************
 */

#ifndef STREAK_FRESH_START_SCREEN_HPP
#define STREAK_FRESH_START_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class FreshStartScreen : public Screen
{
public:
    explicit FreshStartScreen(Model& model);
    ~FreshStartScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    std::unique_ptr<Widgets::Sunrise> mSunrise;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_FRESH_START_SCREEN_HPP
