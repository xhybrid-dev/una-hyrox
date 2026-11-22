/**
 ******************************************************************************
 * @file    NumbersScreen.hpp
 * @brief   Your numbers (NOTES "S7"): what the rest of the X family leaves in
 *          the shared folder, in one place.
 *
 *   Today's plan   Tempo 30 min        (X Training: today's session, or Done,
 *   Pace           4:18/km              Rest day, Plan paused, ...)
 *   Next           Tue: Intervals      (after a session done or a rest day)
 *   VO2max         52.3 · 5 runs       (X Run)
 *   Weight         78.4 kg             (X Weigh-in's trend)
 *   Trend          -0.4 kg this week
 *
 * A row shows only when its app has left its file; with none, one row says
 * what will appear. The words are DashWords'. A wheel, as the Training load
 * screen: L1 and L2 move, R2 back to the menu.
 ******************************************************************************
 */

#ifndef STREAK_NUMBERSSCREEN_HPP
#define STREAK_NUMBERSSCREEN_HPP

#include <memory>

#include "gui/copy/DashWords.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class NumbersScreen : public Screen
{
public:
    explicit NumbersScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onDash() override;

protected:
    void build() override;

private:
    void fill();

    Widgets::Wheel::Item             mItems[DashWords::kMaxRows] {};
    DashWords::Row                   mRows[DashWords::kMaxRows] {};
    uint8_t                          mCount = 0;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Wheel>   mMenu;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_NUMBERSSCREEN_HPP
