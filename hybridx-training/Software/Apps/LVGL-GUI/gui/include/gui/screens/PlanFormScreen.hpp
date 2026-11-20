/**
 ******************************************************************************
 * @file    PlanFormScreen.hpp
 * @brief   X Training (TP8): Form. The last eight reviewed sessions as bars
 *          (height is the share of the time in the target band; green on pace,
 *          yellow for misses on the fast side, cyan for the slow side, grey for
 *          a session not judged), with a dot under each for how the runner felt
 *          (teal fresh or good, white steady, amber tired or spent), and the one
 *          line the pattern says (ReviewText). Reached from Plan with L2; R2 or
 *          L1 goes back.
 ******************************************************************************
 */

#ifndef PLAN_FORM_SCREEN_HPP
#define PLAN_FORM_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/TextStack.hpp"
#include "gui/widgets/Widgets.hpp"
#include "Commands.hpp"

class PlanFormScreen : public Screen
{
public:
    explicit PlanFormScreen(Model& model);
    ~PlanFormScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onPlanForm() override;

    static constexpr int32_t kBaseY  = 112;   ///< the bars' foot
    static constexpr int32_t kMaxH   = 46;    ///< a bar of 100%
    static constexpr int32_t kBarW   = 14;
    static constexpr int32_t kGap    = 6;

protected:
    void build() override;

private:
    void fill();

    TextStack mText;   ///< the caption over the bars and the lines under them
    lv_obj_t* mBar[CustomMessage::PlanFormInfo::kMax] = {};
    lv_obj_t* mDot[CustomMessage::PlanFormInfo::kMax] = {};
    char      mLine[3][64] = {};
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // PLAN_FORM_SCREEN_HPP
