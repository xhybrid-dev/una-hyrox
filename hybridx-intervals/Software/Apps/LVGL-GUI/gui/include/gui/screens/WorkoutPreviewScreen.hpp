/**
 ******************************************************************************
 * @file    WorkoutPreviewScreen.hpp
 * @brief   HybridX Intervals: a workout's steps, before starting it.
 *
 * The name, the totals, then every step on its own line ("Run 400 m"), its
 * target under it ("@ 3:50-4:10 /km"), and a repeat block under a "6 x"
 * heading. L1/L2 scroll; R1 starts (via the countdown, or the "no GPS"
 * question); R2 goes back to the list and puts back the workout chosen
 * before. Opening the preview chooses the workout, as Trail's route preview
 * does: its steps only come from the service once it is loaded.
 ******************************************************************************
 */

#ifndef WORKOUT_PREVIEW_SCREEN_HPP
#define WORKOUT_PREVIEW_SCREEN_HPP

#include <memory>

#include "WorkoutTypes.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class WorkoutPreviewScreen : public Screen
{
public:
    explicit WorkoutPreviewScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onWorkout() override;

protected:
    void build() override;

private:
    static constexpr uint8_t kRows     = 5;
    static constexpr uint8_t kMaxLines = Intervals::Workout::kMaxSteps * 2 + Intervals::Workout::kMaxSteps / 2;

    struct Line {
        char     text[32] = {};
        uint32_t color    = 0;
    };

    bool ready() const;
    void show();
    void makeLines();
    void render();

    int8_t  mIndex    = -1;   ///< the list entry being previewed
    int8_t  mPrevious = -1;   ///< the workout chosen before, put back on R2
    Line    mLines[kMaxLines] {};
    uint8_t mLineCount = 0;
    uint8_t mTop       = 0;   ///< first line shown
    char    mName[32]  = {};
    char    mSummary[32] = {};

    lv_obj_t* mSummaryLabel = nullptr;
    lv_obj_t* mRowLabels[kRows] {};
    lv_obj_t* mMore    = nullptr;
    lv_obj_t* mLoading = nullptr;

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // WORKOUT_PREVIEW_SCREEN_HPP
