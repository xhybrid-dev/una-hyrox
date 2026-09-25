/**
 ******************************************************************************
 * @file    TrophyScreen.hpp
 * @brief   The trophy case: the five summits, the session badges, and the bests.
 ******************************************************************************
 */

#ifndef STREAK_TROPHYSCREEN_HPP
#define STREAK_TROPHYSCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrophyScreen : public Screen
{
public:
    explicit TrophyScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onTrophies() override;
protected:
    void build() override;

private:
    static constexpr uint8_t kSummits = 5;
    static constexpr uint8_t kBadges  = 4;
    static constexpr uint8_t kItems   = kSummits + kBadges + 3;

    void fill();

    Widgets::Wheel::Item             mItems[kItems] {};
    char                             mTip[kItems][32] {};
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Wheel>   mMenu;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_TROPHYSCREEN_HPP
