/**
 ******************************************************************************
 * @file    WorkoutListScreen.hpp
 * @brief   HybridX Intervals: choose a workout (or none) from Workouts/.
 *
 * A wheel of the workouts the service found, by name, each with its totals
 * ("2.4 km, 34 min"); "No workout" first, for a plain run; and, last, how to
 * add workouts. A file that can't be read is listed in red with the reason; a
 * bike workout in grey ("bike: not yet"). R1 on a workout opens its preview;
 * on "No workout" it clears the choice and goes back. R2 goes back.
 * Modelled on HybridX Trail's RouteListScreen.
 ******************************************************************************
 */

#ifndef WORKOUT_LIST_SCREEN_HPP
#define WORKOUT_LIST_SCREEN_HPP

#include <memory>

#include "WorkoutStore.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/WheelMenu.hpp"
#include "gui/widgets/Widgets.hpp"

class WorkoutListScreen : public Screen
{
public:
    explicit WorkoutListScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onWorkouts() override;

protected:
    void build() override;

private:
    static constexpr uint16_t kMaxItems = Intervals::WorkoutStore::kMaxWorkouts + 2;   ///< + "No workout" + "Add workouts"

    void fill();
    void updateButtons();
    bool isWorkout(uint16_t item) const { return item >= 1 && item <= mModel.workoutCount(); }
    bool canOpen(uint16_t item) const;

    WheelMenu::Item mItems[kMaxItems] {};
    char            mNames[kMaxItems][32] {};
    char            mTips[kMaxItems][32] {};
    uint16_t        mItemCount = 0;

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // WORKOUT_LIST_SCREEN_HPP
