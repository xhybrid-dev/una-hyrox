/**
 ******************************************************************************
 * @file    PlanScreen.hpp
 * @brief   X Training: Plan (brief 8.1). The plan's name in the title; the
 *          week and its phase; a bar of the weeks, coloured by phase, this
 *          week marked; this week's sessions done, the plan's so far; the race
 *          or the last day. R2 goes back. (New plan, R1, is step 3's.)
 ******************************************************************************
 */

#ifndef PLAN_SCREEN_HPP
#define PLAN_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/TextStack.hpp"
#include "gui/widgets/Widgets.hpp"
#include "Commands.hpp"

class PlanScreen : public Screen
{
public:
    explicit PlanScreen(Model& model);
    ~PlanScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onPlanOverview() override;

    static constexpr int32_t kBarY = 114;   ///< the bar's centre line
    static constexpr int32_t kBarH = 10;

protected:
    void build() override;

private:
    void fill();

    TextStack mTop;      ///< the week and phase, over the bar
    TextStack mBottom;   ///< the counts and the end, under it
    lv_obj_t* mBar[CustomMessage::PlanOverviewInfo::kMaxWeeks] = {};
    lv_obj_t* mNow = nullptr;   ///< under this week's segment
    char      mText[6][44] = {};
    char      mTitleText[24] = "PLAN";
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // PLAN_SCREEN_HPP
