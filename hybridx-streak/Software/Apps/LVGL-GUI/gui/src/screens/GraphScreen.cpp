/**
 ******************************************************************************
 * @file    GraphScreen.cpp
 * @brief   The training load graphs (see the header).
 ******************************************************************************
 */

#include "gui/screens/GraphScreen.hpp"

#include <cstdio>

#include "FormText.hpp"
#include "GraphMath.hpp"
#include "gui/copy/DashWords.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace Color = SDK::GUI::Color;
using CustomMessage::SeriesData;
using Streak::GraphMath::Range;

namespace
{
// The two lines must read apart at a glance (NOTES "S8"): cyan against orange (apart in the common
// kinds of colour blindness, and cyan is about twice as bright), 3 px against 2 px, fitness on top.
constexpr uint32_t kFitnessColour = Color::CYAN;          // 40 C0 C0
constexpr uint32_t kFatigueColour = Color::ORANGE_DARK;   // C0 40 00: not Streak's amber, which means "at risk"
constexpr int32_t  kFitnessWidth  = 3;
constexpr int32_t  kFatigueWidth  = 2;
constexpr uint32_t kFormColour    = Color::WHITE;
constexpr uint32_t kBarColour     = Color::TEAL;
constexpr uint32_t kGuideColour   = Color::GRAY_DARK;

lv_obj_t* line(lv_obj_t* parent, const lv_point_precise_t* points, uint32_t count, uint32_t colour, int32_t width)
{
    lv_obj_t* l = lv_line_create(parent);
    lv_obj_set_pos(l, 0, 0);
    lv_obj_set_style_line_width(l, width, LV_PART_MAIN);
    lv_obj_set_style_line_color(l, Theme::rgb(colour), LV_PART_MAIN);
    lv_obj_set_style_line_rounded(l, true, LV_PART_MAIN);
    lv_line_set_points(l, points, count);
    return l;
}
} // namespace

void GraphScreen::build()
{
    const uint8_t page = static_cast<uint8_t>(mModel.graphPage % kPages);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, DashWords::kGraphTitles[page]);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::NONE, Widgets::Buttons::WHITE);

    // Which page of three, as X Weigh-in's pages show it.
    for (uint8_t i = 0; i < kPages; ++i) {
        Theme::dot(mRoot, 110 + i * 10, 224, 2, i == page ? Color::WHITE : Color::GRAY_DARK);
    }

    if (!mModel.hasSeries()) {
        message("", "");   // the service answers at once; onSeries() builds the page again
        return;
    }
    switch (page) {
        case FitnessFatigue: buildLines(); break;
        case Form:           buildForm(); break;
        default:             buildBars(); break;
    }
}

void GraphScreen::buildFrame()
{
    // Faint guides at the top and bottom of the scale.
    Theme::box(mRoot, kLeft, kTop, kWidth, 1, kGuideColour);
    Theme::box(mRoot, kLeft, kTop + kHeight - 1, kWidth, 1, kGuideColour);
}

void GraphScreen::scale(const char* top, const char* bottom)
{
    // X Weigh-in's scale labels, in a box 4 px wider (DashWords::kMaxScalePx): "80000" is 44 px.
    Theme::label(mRoot, Theme::Font::Regular14, top, kLeft - 50, kTop - 9, 46, LV_TEXT_ALIGN_RIGHT, Color::GRAY);
    Theme::label(mRoot, Theme::Font::Regular14, bottom, kLeft - 50, kTop + kHeight - 10, 46, LV_TEXT_ALIGN_RIGHT,
                 Color::GRAY);
}

void GraphScreen::span(uint8_t days)
{
    const SeriesData& s = mModel.series(SeriesData::Fitness);
    char              text[32];
    DashWords::graphSpan(s.lastDay - (days - 1), s.lastDay, text, sizeof(text));
    Theme::label(mRoot, Theme::Font::Regular14, text, 30, kTop + kHeight + 5, 180, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void GraphScreen::legend(const char* first, uint32_t firstColour, const char* second, uint32_t secondColour)
{
    Theme::label(mRoot, Theme::Font::Regular16, first, 30, 174, 180, LV_TEXT_ALIGN_CENTER, firstColour);
    if (second != nullptr && second[0] != '\0') {
        Theme::label(mRoot, Theme::Font::Regular16, second, 30, 194, 180, LV_TEXT_ALIGN_CENTER, secondColour);
    }
}

void GraphScreen::message(const char* big, const char* small)
{
    Theme::label(mRoot, Theme::Font::Medium18, big, 30, kTop + 24, 180);
    Theme::label(mRoot, Theme::Font::Regular16, small, 30, kTop + 50, 180, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void GraphScreen::buildLines()
{
    const SeriesData&              fit = mModel.series(SeriesData::Fitness);
    const SeriesData&              fat = mModel.series(SeriesData::Fatigue);
    const CustomMessage::LoadData& now = mModel.load();
    char a[32], b[32];
    DashWords::fitnessLegend(now.fitness, a, sizeof(a));
    DashWords::fatigueLegend(now.fatigue, b, sizeof(b));
    legend(a, kFitnessColour, b, kFatigueColour);

    if (fit.historyDays < Streak::kWarmUpDays || fit.count < 2) {
        char day[24];
        snprintf(day, sizeof(day), "Day %u of %u", static_cast<unsigned>(fit.historyDays),
                 static_cast<unsigned>(Streak::kWarmUpDays));
        message("Learning", day);
        return;
    }
    const uint8_t n = fit.count;
    const Range   r = Streak::GraphMath::fromZero(fit.v, fat.v, n, 10, 100);
    buildFrame();
    DashWords::scaleLabel(r.hi, 10, false, a, sizeof(a));
    scale(a, "0");

    // The newest day at the right edge; a young history fills from the right.
    const uint8_t offset = static_cast<uint8_t>(Streak::kTrendDays - n);
    for (uint8_t i = 0; i < n; ++i) {
        const int32_t x = Streak::GraphMath::xFor(static_cast<uint8_t>(offset + i), Streak::kTrendDays, kLeft, kWidth);
        mFirst[i].x  = x;
        mFirst[i].y  = Streak::GraphMath::yFor(fit.v[i], r, kTop, kHeight);
        mSecond[i].x = x;
        mSecond[i].y = Streak::GraphMath::yFor(fat.v[i], r, kTop, kHeight);
    }
    line(mRoot, mSecond, n, kFatigueColour, kFatigueWidth);   // underneath
    line(mRoot, mFirst, n, kFitnessColour, kFitnessWidth);    // on top
    span(n);
}

void GraphScreen::buildForm()
{
    const SeriesData&              fit = mModel.series(SeriesData::Fitness);
    const SeriesData&              fat = mModel.series(SeriesData::Fatigue);
    const CustomMessage::LoadData& now = mModel.load();
    char a[48], b[16];
    DashWords::formLegend(now.form, now.word, a, sizeof(a));
    legend(a, now.word == static_cast<uint8_t>(Streak::FormWord::Overreaching) ? Palette::kAtRisk : kFormColour,
           nullptr, 0);

    if (fit.historyDays < Streak::kWarmUpDays || fit.count < 2) {
        char day[24];
        snprintf(day, sizeof(day), "Day %u of %u", static_cast<unsigned>(fit.historyDays),
                 static_cast<unsigned>(Streak::kWarmUpDays));
        message("Learning", day);
        return;
    }
    const uint8_t n = fit.count;
    const Range   r = Streak::GraphMath::aroundZero(fit.v, fat.v, n, 10, 50);
    buildFrame();
    // The zero line, a shade lighter than the guides: above it fresh, below it tired.
    const int32_t y0 = Streak::GraphMath::yFor(0, r, kTop, kHeight);
    Theme::box(mRoot, kLeft, y0, kWidth, 1, Color::GRAY);
    DashWords::scaleLabel(r.hi, 10, true, a, sizeof(a));
    DashWords::scaleLabel(r.lo, 10, true, b, sizeof(b));
    scale(a, b);

    const uint8_t offset = static_cast<uint8_t>(Streak::kTrendDays - n);
    for (uint8_t i = 0; i < n; ++i) {
        const int32_t form = static_cast<int32_t>(fit.v[i]) - static_cast<int32_t>(fat.v[i]);
        mFirst[i].x = Streak::GraphMath::xFor(static_cast<uint8_t>(offset + i), Streak::kTrendDays, kLeft, kWidth);
        mFirst[i].y = Streak::GraphMath::yFor(form, r, kTop, kHeight);
    }
    line(mRoot, mFirst, n, kFormColour, 2);
    span(n);
}

void GraphScreen::buildBars()
{
    const SeriesData&              ld  = mModel.series(SeriesData::Load);
    const CustomMessage::LoadData& now = mModel.load();
    char a[32], b[32];
    DashWords::weekLoadLegend(now.weekLoad, a, sizeof(a));
    DashWords::todayLoadLegend(now.todayLoad, b, sizeof(b));
    legend(a, Color::WHITE, b, Color::GRAY);

    const uint8_t n = ld.count < kBarDays ? ld.count : kBarDays;
    if (n == 0) {
        message("No load yet", "");
        return;
    }
    const uint16_t* v = ld.v + (ld.count - n);   // the newest n days
    const Range     r = Streak::GraphMath::fromZero(v, nullptr, n, 1, 50);
    buildFrame();
    DashWords::scaleLabel(r.hi, 1, false, a, sizeof(a));
    scale(a, "0");

    // A bar a day, 3 px wide, the newest at the right edge.
    constexpr int32_t kBarW   = 3;
    const int32_t     bottom  = kTop + kHeight - 1;
    const uint8_t     offset  = static_cast<uint8_t>(kBarDays - n);
    for (uint8_t i = 0; i < n; ++i) {
        if (v[i] == 0) {
            continue;
        }
        const int32_t x = Streak::GraphMath::xFor(static_cast<uint8_t>(offset + i), kBarDays, kLeft, kWidth - kBarW + 1);
        const int32_t y = Streak::GraphMath::yFor(v[i], r, kTop, kHeight);
        Theme::box(mRoot, x, y, kBarW, bottom - y + 1, kBarColour);
    }
    span(n);
}

void GraphScreen::onShow()
{
    mModel.resetIdleTimer();
}

void GraphScreen::onSeries()
{
    ScreenManager::instance().goTo(ScreenId::Graph);   // build the page with its data
}

void GraphScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1:
            mModel.graphPage = static_cast<uint8_t>((mModel.graphPage + kPages - 1) % kPages);
            ScreenManager::instance().goTo(ScreenId::Graph);
            break;
        case Btn::L2:
            mModel.graphPage = static_cast<uint8_t>((mModel.graphPage + 1) % kPages);
            ScreenManager::instance().goTo(ScreenId::Graph);
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Load);
            break;
        default:
            break;
    }
}
