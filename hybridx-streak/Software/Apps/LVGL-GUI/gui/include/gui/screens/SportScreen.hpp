/**
 ******************************************************************************
 * @file    SportScreen.hpp
 * @brief   This week by sport (NOTES "S7"): every X app's sessions, their time
 *          and distance, kind by kind, then last week's.
 *
 *   This week   5 sessions · 3 h 20
 *   Distance    24.5 km                (when any session had one)
 *   Run         2 · 1 h 05 · 14.2 km   (each kind done this week)
 *   By hand     1 session              (manual logs: no time or distance)
 *   Last week   4 sessions · 2 h 50
 *
 * Everything recorded counts here, whatever the streak's rules, as for the
 * training load. A wheel, as the Training load screen: L1 and L2 move, R2 back
 * to the menu.
 ******************************************************************************
 */

#ifndef STREAK_SPORTSCREEN_HPP
#define STREAK_SPORTSCREEN_HPP

#include <memory>

#include "StreakTypes.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class SportScreen : public Screen
{
public:
    explicit SportScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onDash() override;

protected:
    void build() override;

private:
    /// This week, distance, each kind, by hand, last week.
    static constexpr uint8_t kMaxItems = 4 + Streak::kKindCount;

    void fill();

    Widgets::Wheel::Item             mItems[kMaxItems] {};
    char                             mLabel[kMaxItems][24] {};
    char                             mTip[kMaxItems][48] {};
    uint8_t                          mCount = 0;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Wheel>   mMenu;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_SPORTSCREEN_HPP
