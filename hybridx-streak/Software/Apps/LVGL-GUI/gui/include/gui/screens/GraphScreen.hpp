/**
 ******************************************************************************
 * @file    GraphScreen.hpp
 * @brief   The training load graphs (NOTES "S8"), opened with R1 from the
 *          Training load screen. Three pages, L1 and L2 between them, R2 back:
 *
 *   Training load   fitness (cyan, 3 px, on top) and fatigue (orange, 2 px)
 *                   over the last 12 weeks, on one scale from 0;
 *   Form            fitness - fatigue, a white line about a zero line;
 *   Daily load      the last 4 weeks, a bar a day.
 *
 * Drawn as X Weigh-in draws its trend: LVGL lines over grey guides at the top
 * and bottom of the scale, the scale's ends on the left, the dates under the
 * plot, the newest day at the right edge (a young history fills from the
 * right). The legends are in the lines' own colours and take their numbers
 * from the Training load screen's, so the two screens agree. During the first
 * two weeks the line pages say "Learning", as the Form row does.
 *
 * The series come from the service when asked (Model::requestSeries); until
 * they have come the page says so. A page builds only what it shows.
 ******************************************************************************
 */

#ifndef STREAK_GRAPHSCREEN_HPP
#define STREAK_GRAPHSCREEN_HPP

#include <memory>

#include "LoadHistory.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class GraphScreen : public Screen
{
public:
    enum Page : uint8_t { FitnessFatigue, Form, DailyLoad, kPages };

    explicit GraphScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onKey(uint8_t code) override;
    void onSeries() override;

protected:
    void build() override;

private:
    // The plot: X Weigh-in's geometry, 2 px higher and 8 px shorter to leave two legend lines.
    static constexpr int32_t kLeft   = 80;
    static constexpr int32_t kTop    = 58;
    static constexpr int32_t kWidth  = 130;
    static constexpr int32_t kHeight = 92;
    static constexpr uint8_t kBarDays = 28;

    void buildFrame();
    void buildLines();
    void buildForm();
    void buildBars();
    void legend(const char* first, uint32_t firstColour, const char* second, uint32_t secondColour);
    void scale(const char* top, const char* bottom);
    void message(const char* big, const char* small);
    void span(uint8_t days);

    lv_point_precise_t                mFirst[Streak::kTrendDays] {};
    lv_point_precise_t                mSecond[Streak::kTrendDays] {};
    lv_point_precise_t                mZero[2] {};
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // STREAK_GRAPHSCREEN_HPP
