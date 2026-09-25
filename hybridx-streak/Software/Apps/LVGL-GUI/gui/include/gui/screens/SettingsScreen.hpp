/**
 ******************************************************************************
 * @file    SettingsScreen.hpp
 * @brief   The goal: weekly target, week start, what counts, shortest session, one per day.
 ******************************************************************************
 */

#ifndef STREAK_SETTINGSSCREEN_HPP
#define STREAK_SETTINGSSCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class SettingsScreen : public Screen
{
public:
    explicit SettingsScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onGoal() override;
protected:
    void build() override;

private:
    enum : uint8_t { kTarget, kWeekStart, kCounts, kMinimum, kOnePerDay, kCount };

    void fill();
    Streak::Goal effective() const;

    Widgets::Wheel::Item             mItems[kCount] {};
    char                             mTip[kCount][32] {};
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Wheel>   mMenu;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_SETTINGSSCREEN_HPP
