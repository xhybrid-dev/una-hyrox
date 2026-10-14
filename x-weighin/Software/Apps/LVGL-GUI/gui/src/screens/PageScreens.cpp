/**
 ******************************************************************************
 * @file    PageScreens.cpp
 * @brief   The four pages: Home, Graph, Recent and Body.
 *
 * Every number is formatted from integers (WeighText, DateText): no %f.
 * Layouts keep text inside the round display and clear of the button marks.
 ******************************************************************************
 */

#include "gui/screens/Screens.hpp"

#include <cstdio>

#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "DateText.hpp"
#include "WeighText.hpp"
#include "gui/Assets.hpp"

namespace Draw  = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;
using namespace XWeighIn;

namespace
{

/// "-0.4 kg this week", or "-- kg this week" until the log spans a week.
void weekText(const CustomMessage::HomeData& h, char* out, size_t outSize)
{
    char change[16];
    if (h.hasWeekChange) {
        formatChange(h.weekChange, 1, change, sizeof(change));
    } else {
        std::snprintf(change, sizeof(change), "--");
    }
    std::snprintf(out, outSize, "%s kg this week", change);
}

/// "Last 80.8, today" (or the date when the clock is not set).
void lastText(const CustomMessage::HomeData& h, char* out, size_t outSize)
{
    char kg[16];
    char when[24];
    formatKg(h.latest, 1, kg, sizeof(kg));
    if (h.today != 0) {
        agoText(h.today, h.lastDay, when, sizeof(when));
    } else {
        formatShortDate(h.lastDay, when, sizeof(when));
    }
    std::snprintf(out, outSize, "Last %s, %s", kg, when);
}

}  // namespace

// -- Home --------------------------------------------------------------------------

void HomeScreen::build()
{
    buildFrame("");
    mCaption = Draw::label(mRoot, &poppins_regular_18, "", 40, 40, 160, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mTrend   = Draw::label(mRoot, &poppins_light_60, "", 20, 62, 200);
    mChange  = Draw::label(mRoot, &poppins_regular_18, "", 20, 132, 200);
    mLast    = Draw::label(mRoot, &poppins_regular_18, "", 20, 160, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void HomeScreen::refresh()
{
    char text[48];   // lastText(): "Last " + 15 + ", " + 23, terminated
    if (!mModel.hasHome()) {
        lv_label_set_text(mCaption, "");
        lv_label_set_text(mTrend, "");
        lv_label_set_text(mChange, "");
        lv_label_set_text(mLast, "");
        return;
    }
    const CustomMessage::HomeData& h = mModel.home();
    lv_label_set_text(mCaption, "Trend (kg)");
    if (h.days == 0) {
        lv_label_set_text(mTrend, "--");
        lv_label_set_text(mChange, "No weigh-ins yet");
        lv_label_set_text(mLast, "SELECT to weigh in");
        return;
    }
    formatKg(h.trend, 1, text, sizeof(text));
    lv_label_set_text(mTrend, text);
    weekText(h, text, sizeof(text));
    lv_label_set_text(mChange, text);
    lastText(h, text, sizeof(text));
    lv_label_set_text(mLast, text);
}

// -- Graph ---------------------------------------------------------------------------

void GraphScreen::build()
{
    buildFrame("Last 30 days");

    // Faint guides at the top and bottom of the scale.
    Draw::box(mRoot, kLeft, kTop, kWidth, 1, Color::GRAY_DARK);
    Draw::box(mRoot, kLeft, kTop + kHeight - 1, kWidth, 1, Color::GRAY_DARK);
    mTopLabel    = Draw::label(mRoot, &poppins_regular_14, "", kLeft - 40, kTop - 9, 36,
                               LV_TEXT_ALIGN_RIGHT, Color::GRAY);
    mBottomLabel = Draw::label(mRoot, &poppins_regular_14, "", kLeft - 40, kTop + kHeight - 10, 36,
                               LV_TEXT_ALIGN_RIGHT, Color::GRAY);

    // The day's weigh-ins as dots, under the trend line.
    for (lv_obj_t*& d : mDots) {
        d = Draw::dot(mRoot, 0, 0, 2, Color::GRAY);
        Draw::setHidden(d, true);
    }
    mLine = lv_line_create(mRoot);
    lv_obj_set_pos(mLine, 0, 0);
    lv_obj_set_style_line_width(mLine, 2, LV_PART_MAIN);
    lv_obj_set_style_line_color(mLine, Draw::rgb(Color::LIME), LV_PART_MAIN);
    lv_obj_set_style_line_rounded(mLine, true, LV_PART_MAIN);

    mSpan  = Draw::label(mRoot, &poppins_regular_14, "", 30, kTop + kHeight + 8, 180,
                         LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mEmpty = Draw::label(mRoot, &poppins_regular_18, "No weigh-ins yet", 20, 100, 200);
}

void GraphScreen::refresh()
{
    const Window& w = mModel.window();
    const uint8_t count = w.count <= kWindowDays ? w.count : kWindowDays;
    Range r;
    const bool any = count > 0 && rangeFor(w.weight, w.trend, count, r);

    Draw::setHidden(mEmpty, any);
    Draw::setHidden(mLine, !any);
    for (lv_obj_t* d : mDots) {
        Draw::setHidden(d, true);
    }
    if (!any) {
        lv_label_set_text(mTopLabel, "");
        lv_label_set_text(mBottomLabel, "");
        lv_label_set_text(mSpan, "");
        return;
    }

    // The newest day sits at the right edge; a young log fills from the right.
    const uint8_t offset = static_cast<uint8_t>(kWindowDays - count);
    for (uint8_t i = 0; i < count; ++i) {
        const int32_t x = xFor(static_cast<uint8_t>(offset + i), kWindowDays, kLeft, kWidth);
        mPoints[i].x = x;
        mPoints[i].y = yFor(w.trend[i], r, kTop, kHeight);
        if (w.weight[i] != 0) {
            const int32_t y = yFor(w.weight[i], r, kTop, kHeight);
            lv_obj_set_pos(mDots[i], x - 2, y - 2);
            Draw::setHidden(mDots[i], false);
        }
    }
    lv_line_set_points(mLine, mPoints, count);

    char text[32];
    formatKg(r.hi, 1, text, sizeof(text));
    lv_label_set_text(mTopLabel, text);
    formatKg(r.lo, 1, text, sizeof(text));
    lv_label_set_text(mBottomLabel, text);

    char from[12];
    char to[12];
    formatDayMonth(w.firstDay, from, sizeof(from));
    formatDayMonth(w.firstDay + count - 1, to, sizeof(to));
    std::snprintf(text, sizeof(text), "%s - %s", from, to);
    lv_label_set_text(mSpan, text);
}

// -- Recent --------------------------------------------------------------------------

void RecentScreen::build()
{
    buildFrame("Recent");
    for (uint8_t i = 0; i < kRows; ++i) {
        const int32_t y = 56 + i * 28;
        // Columns clear of the UP mark (left) and the BACK mark (right, lower rows).
        mDates[i]   = Draw::label(mRoot, &poppins_regular_18, "", 38, y, 104, LV_TEXT_ALIGN_LEFT);
        mWeights[i] = Draw::label(mRoot, &poppins_regular_18, "", 140, y, 60, LV_TEXT_ALIGN_RIGHT);
    }
    mEmpty = Draw::label(mRoot, &poppins_regular_18, "No weigh-ins yet", 20, 100, 200);
}

void RecentScreen::refresh()
{
    RecentDay rows[kRows];
    const uint8_t n = recentDays(mModel.window(), rows, kRows);
    Draw::setHidden(mEmpty, n > 0);
    char text[24];
    for (uint8_t i = 0; i < kRows; ++i) {
        if (i < n) {
            formatShortDate(rows[i].day, text, sizeof(text));
            lv_label_set_text(mDates[i], text);
            formatKg(rows[i].hundredths, 1, text, sizeof(text));
            lv_label_set_text(mWeights[i], text);
        } else {
            lv_label_set_text(mDates[i], "");
            lv_label_set_text(mWeights[i], "");
        }
    }
}

// -- Body ----------------------------------------------------------------------------

void BodyScreen::build()
{
    buildFrame("Body");
    mBmi    = Draw::label(mRoot, &poppins_semibold_25, "", 20, 54, 200);
    mBmr    = Draw::label(mRoot, &poppins_regular_18, "", 20, 92, 200);
    mHeight = Draw::label(mRoot, &poppins_regular_18, "", 20, 120, 200);
    mAge    = Draw::label(mRoot, &poppins_regular_18, "", 20, 148, 200);
    mNote   = Draw::label(mRoot, &poppins_regular_14, "", 30, 180, 180, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void BodyScreen::refresh()
{
    if (!mModel.hasHome()) {
        return;
    }
    const CustomMessage::HomeData& h = mModel.home();
    char text[40];

    if (h.bmiTenths > 0) {
        std::snprintf(text, sizeof(text), "BMI %d.%d",
                      static_cast<int>(h.bmiTenths / 10), static_cast<int>(h.bmiTenths % 10));
    } else {
        std::snprintf(text, sizeof(text), "BMI --");
    }
    lv_label_set_text(mBmi, text);

    if (h.bmrKcal > 0) {
        std::snprintf(text, sizeof(text), "Resting %d kcal", static_cast<int>(h.bmrKcal));
    } else {
        std::snprintf(text, sizeof(text), "Resting -- kcal");
    }
    lv_label_set_text(mBmr, text);

    if (h.heightCm != 0) {
        std::snprintf(text, sizeof(text), "Height %lu cm", static_cast<unsigned long>(h.heightCm));
    } else {
        std::snprintf(text, sizeof(text), "Height not set");
    }
    lv_label_set_text(mHeight, text);

    const char* sex = sexLabel(h.sex);
    if (h.ageYears != 0 && sex != nullptr) {
        std::snprintf(text, sizeof(text), "Age %u, %s", static_cast<unsigned>(h.ageYears), sex);
    } else if (h.ageYears != 0) {
        std::snprintf(text, sizeof(text), "Age %u", static_cast<unsigned>(h.ageYears));
    } else {
        std::snprintf(text, sizeof(text), "Age not set");
    }
    lv_label_set_text(mAge, text);

    // The first thing missing, else what the numbers come from.
    if (h.days == 0) {
        lv_label_set_text(mNote, "Weigh in to see these");
    } else if (h.heightCm == 0) {
        lv_label_set_text(mNote, "Height: the UNA profile");
    } else if (h.ageYears == 0 || sex == nullptr) {
        lv_label_set_text(mNote, "Age, sex: app_config.json");
    } else {
        lv_label_set_text(mNote, "From your trend weight");
    }
}
