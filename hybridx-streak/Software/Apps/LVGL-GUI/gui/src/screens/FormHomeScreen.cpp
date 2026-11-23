/**
 ******************************************************************************
 * @file    FormHomeScreen.cpp
 * @brief   Home's second page: form, fitness and fatigue (see the header).
 ******************************************************************************
 */

#include "gui/screens/FormHomeScreen.hpp"

#include <cstdio>

#include "LoadFormText.hpp"
#include "GraphMath.hpp"
#include "gui/screens/GraphScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace Color = SDK::GUI::Color;
using CustomMessage::SeriesData;

namespace
{
// The graphs' colours (GraphScreen, NOTES "S8"): the same lines, the same meaning.
constexpr uint32_t kFitnessColour = Color::CYAN;
constexpr uint32_t kFatigueColour = Color::ORANGE_DARK;

lv_obj_t* sparkLine(lv_obj_t* parent, uint32_t colour, int32_t width)
{
    lv_obj_t* l = lv_line_create(parent);
    lv_obj_set_pos(l, 0, 0);
    lv_obj_set_style_line_width(l, width, LV_PART_MAIN);
    lv_obj_set_style_line_color(l, Theme::rgb(colour), LV_PART_MAIN);
    lv_obj_set_style_line_rounded(l, true, LV_PART_MAIN);
    return l;
}
} // namespace

void FormHomeScreen::build()
{
    Theme::label(mRoot, Theme::Font::Regular14, "Form", 70, 14, 100, LV_TEXT_ALIGN_CENTER, Palette::kTextSoft);
    mNumber  = Theme::label(mRoot, Theme::Font::SemiBold60, "", 20, 30, 200);
    mWord    = Theme::label(mRoot, Theme::Font::SemiBold25, "", 20, 98, 200);
    mFitness = Theme::label(mRoot, Theme::Font::SemiBold25, "", 40, 128, 80, LV_TEXT_ALIGN_CENTER, kFitnessColour);
    mFatigue = Theme::label(mRoot, Theme::Font::SemiBold25, "", 120, 128, 80, LV_TEXT_ALIGN_CENTER, kFatigueColour);
    Theme::label(mRoot, Theme::Font::Regular14, "Fitness", 40, 158, 80, LV_TEXT_ALIGN_CENTER, Palette::kTextSoft);
    Theme::label(mRoot, Theme::Font::Regular14, "Fatigue", 120, 158, 80, LV_TEXT_ALIGN_CENTER, Palette::kTextSoft);

    // The small graph: fatigue underneath, fitness on top, as on the big one. Both 2 px: at this
    // size a 1 px line all but disappears, and the colours carry the difference.
    mFatLine = sparkLine(mRoot, kFatigueColour, 2);
    mFitLine = sparkLine(mRoot, kFitnessColour, 2);

    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    // L1 and L2 back to the mountain; R1 the graphs; R2 leaves, as on the mountain.
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);

    fill();
    drawSpark();
}

void FormHomeScreen::fill()
{
    const CustomMessage::LoadData& d    = mModel.load();
    const auto                     word = static_cast<Streak::FormWord>(d.word);
    const bool known = word != Streak::FormWord::Learning && word != Streak::FormWord::NoLoad;
    char       text[24];
    if (known) {
        Streak::formatFormNumber(text, sizeof(text), d.form);
        lv_obj_set_style_text_font(mNumber, Theme::font(Theme::Font::SemiBold60), LV_PART_MAIN);
        lv_obj_set_y(mNumber, 30);
        lv_label_set_text(mNumber, text);
        lv_label_set_text(mWord, Streak::formWordName(word));
    } else {
        // Learning (or no load yet): the words in the number's place, the day under them.
        lv_obj_set_style_text_font(mNumber, Theme::font(Theme::Font::SemiBold25), LV_PART_MAIN);
        lv_obj_set_y(mNumber, 52);
        lv_label_set_text(mNumber, Streak::formWordName(word));
        if (word == Streak::FormWord::Learning) {
            const unsigned day = d.historyDays < 1 ? 1u : (d.historyDays > Streak::kWarmUpDays ? Streak::kWarmUpDays
                                                                                                 : d.historyDays);
            snprintf(text, sizeof(text), "Day %u of %u", day, static_cast<unsigned>(Streak::kWarmUpDays));
        } else {
            text[0] = '\0';
        }
        lv_label_set_text(mWord, text);
    }
    const uint32_t colour = word == Streak::FormWord::Overreaching ? Palette::kAtRisk
                                                                  : (known ? Palette::kText : Palette::kTextSoft);
    Theme::setColor(mNumber, colour);
    Theme::setColor(mWord, colour);

    snprintf(text, sizeof(text), "%u", static_cast<unsigned>(d.fitness));
    lv_label_set_text(mFitness, text);
    snprintf(text, sizeof(text), "%u", static_cast<unsigned>(d.fatigue));
    lv_label_set_text(mFatigue, text);
}

void FormHomeScreen::drawSpark()
{
    const SeriesData& fit = mModel.series(SeriesData::Fitness);
    const SeriesData& fat = mModel.series(SeriesData::Fatigue);
    const bool show = mModel.hasSeries() && fit.historyDays >= Streak::kWarmUpDays && fit.count >= 2;
    Theme::setHidden(mFitLine, !show);
    Theme::setHidden(mFatLine, !show);
    if (!show) {
        return;
    }
    const uint8_t                    n = fit.count < kDays ? fit.count : kDays;
    const uint16_t*                  a = fit.v + (fit.count - n);   // the newest n days
    const uint16_t*                  b = fat.v + (fat.count - n);
    const Streak::GraphMath::Range   r = Streak::GraphMath::ownRange(a, b, n, 20);   // the shape, not the size
    const uint8_t                    offset = static_cast<uint8_t>(kDays - n);
    for (uint8_t i = 0; i < n; ++i) {
        const int32_t x = Streak::GraphMath::xFor(static_cast<uint8_t>(offset + i), kDays, kLeft, kWidth);
        mFit[i].x = x;
        mFit[i].y = Streak::GraphMath::yFor(a[i], r, kTop, kHeight);
        mFat[i].x = x;
        mFat[i].y = Streak::GraphMath::yFor(b[i], r, kTop, kHeight);
    }
    lv_line_set_points(mFatLine, mFat, n);
    lv_line_set_points(mFitLine, mFit, n);
}

void FormHomeScreen::onShow()
{
    mModel.resetIdleTimer();
    mModel.requestSeries();   // the small graph's days; onSeries() draws them
}

void FormHomeScreen::onLoad()
{
    fill();
}

void FormHomeScreen::onSeries()
{
    drawSpark();
}

void FormHomeScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1:
        case Btn::L2:
            ScreenManager::instance().goTo(ScreenId::Home);
            break;
        case Btn::R1:
            mModel.graphPage   = GraphScreen::FitnessFatigue;
            mModel.graphReturn = ScreenId::FormHome;
            ScreenManager::instance().goTo(ScreenId::Graph);   // the series are here already, or coming
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            break;
    }
}
