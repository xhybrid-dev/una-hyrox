/**
 ******************************************************************************
 * @file    SummitScreen.hpp
 * @brief   A summit reached: the whole climb lit in lime, a waving flag,
 *          confetti, and the next mountain on the horizon.
 ******************************************************************************
 */

#ifndef STREAK_SUMMIT_SCREEN_HPP
#define STREAK_SUMMIT_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/SummitScene.hpp"
#include "gui/widgets/Widgets.hpp"

class SummitScreen : public Screen
{
public:
    explicit SummitScreen(Model& model);
    ~SummitScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    void confetti();

    std::unique_ptr<SummitScene>      mScene;
    std::unique_ptr<Widgets::Flag>    mFlag;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_SUMMIT_SCREEN_HPP
