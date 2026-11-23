/**
 ******************************************************************************
 * @file    FormHomeScreen.hpp
 * @brief   Home's second page (NOTES "S9"): form, fitness and fatigue up front.
 *
 *                 Form
 *                 +14              (60 pt; amber when it says to ease off)
 *                Fresh
 *            30          34        (cyan and orange, as the graphs' lines)
 *          Fitness     Fatigue
 *          ~~~~~~~~~~~~~~~~~~      the last 4 weeks of the two lines
 *
 * L1 and L2 turn back to the mountain, R1 opens the graphs (fitness and
 * fatigue first), R2 leaves the app, as on the mountain. The figures are the
 * Training load screen's own (LOAD_VIEW), so every screen agrees; the small
 * graph asks for the series as the graphs do. During the first two weeks it
 * says "Learning" and the day. The app can open here ("Opens on", Settings).
 ******************************************************************************
 */

#ifndef STREAK_FORMHOMESCREEN_HPP
#define STREAK_FORMHOMESCREEN_HPP

#include <memory>

#include "LoadHistory.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class FormHomeScreen : public Screen
{
public:
    explicit FormHomeScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onKey(uint8_t code) override;
    void onLoad() override;
    void onSeries() override;

protected:
    void build() override;

private:
    static constexpr uint8_t kDays   = 28;   ///< the small graph: 4 weeks
    // The small graph: as wide as the disc allows at its foot (x 43 to 197 at y 212).
    static constexpr int32_t kLeft   = 58;
    static constexpr int32_t kTop    = 184;
    static constexpr int32_t kWidth  = 124;
    static constexpr int32_t kHeight = 28;

    void fill();
    void drawSpark();

    lv_obj_t* mNumber   = nullptr;
    lv_obj_t* mWord     = nullptr;
    lv_obj_t* mFitness  = nullptr;
    lv_obj_t* mFatigue  = nullptr;
    lv_obj_t* mFitLine  = nullptr;
    lv_obj_t* mFatLine  = nullptr;

    lv_point_precise_t                mFit[kDays] {};
    lv_point_precise_t                mFat[kDays] {};
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_FORMHOMESCREEN_HPP
