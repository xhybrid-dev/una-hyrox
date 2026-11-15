/**
 ******************************************************************************
 * @file    WeekScreen.cpp
 * @brief   This week (see the header).
 ******************************************************************************
 */

#include "gui/screens/WeekScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Audit.hpp"

#include "PlanText.hpp"       // header-only (and PlanTypes.hpp's DayMark)
#include "TimeText.hpp"

#include <cstdio>

namespace Color = SDK::GUI::Color;
using F = Theme::Font;
using XTraining::DayMark;

namespace
{
uint32_t markColor(DayMark m)
{
    switch (m) {
        case DayMark::Done:      return Color::GREEN;
        case DayMark::PartDone:  return Color::YELLOW_DARK;
        case DayMark::Today:     return Color::CYAN;
        case DayMark::ToCome:    return Color::WHITE;
        case DayMark::Skipped:
        case DayMark::NotLogged: return Color::GRAY;
        case DayMark::Rest:
        default:                 return Color::GRAY_DARK;
    }
}

const char* markWords(DayMark m)
{
    switch (m) {
        case DayMark::Done:      return "Done";
        case DayMark::PartDone:  return "Part done";
        case DayMark::Today:     return "Today";
        case DayMark::Skipped:   return "Skipped";
        case DayMark::NotLogged: return "Not logged";
        case DayMark::Rest:      return "Rest day";
        case DayMark::ToCome:
        default:                 return "To come";
    }
}
} // namespace

WeekScreen::WeekScreen(Model& model)
    : Screen(model)
{
}

WeekScreen::~WeekScreen() = default;

void WeekScreen::build()
{
    mBand    = Theme::box(mRoot, 46, 0, 148, kRowPitch + 2, Color::TEAL_DARK);
    Theme::setHidden(mBand, true);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, mTitleText);
    mInfo  = Theme::label(mRoot, F::Italic18, "", 40, 44, 160, LV_TEXT_ALIGN_CENTER, Color::TEAL);
    const int32_t h = lv_font_get_line_height(Theme::font(F::Regular16));
    for (uint8_t i = 0; i < 7; ++i) {
        const int32_t y = kRowY0 + kRowPitch * i - h / 2;
        mDay[i]  = Theme::label(mRoot, F::Regular16, "", kDayRight - 50, y, 50, LV_TEXT_ALIGN_RIGHT, Color::GRAY);
        mName[i] = Theme::label(mRoot, F::Regular16, "", kNameLeft, y, 96, LV_TEXT_ALIGN_LEFT, Color::WHITE);
    }
    mEmpty[0] = Theme::label(mRoot, F::SemiBold25, "No plan", 20, 96, 200, LV_TEXT_ALIGN_CENTER, Color::WHITE);
    mEmpty[1] = Theme::label(mRoot, F::Medium18, "See Today", 20, 132, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    Theme::setHidden(mEmpty[0], true);
    Theme::setHidden(mEmpty[1], true);
    Audit::track(mInfo);
    for (uint8_t i = 0; i < 7; ++i) {
        Audit::track(mDay[i]);
        Audit::track(mName[i]);
    }
    Audit::track(mEmpty[0]);
    Audit::track(mEmpty[1]);
}

void WeekScreen::onShow()
{
    mModel.resetIdleTimer();
    fill();   // what the Model has (perhaps from last time); the service's answer follows
    mModel.requestPlan(CustomMessage::PlanRequest::Week);
}

void WeekScreen::onPlanWeek()
{
    fill();
}

void WeekScreen::fill()
{
    using namespace XTraining;
    const CustomMessage::PlanWeekInfo& w = mModel.getPlanWeek();
    mOk = w.ok;
    Theme::setHidden(mEmpty[0], mOk);
    Theme::setHidden(mEmpty[1], mOk);
    for (uint8_t i = 0; i < 7; ++i) {
        Theme::setHidden(mDay[i], !mOk);
        Theme::setHidden(mName[i], !mOk);
    }
    if (!mOk) {
        Theme::setHidden(mBand, true);
        lv_label_set_text(mInfo, "");
        mTitle->setText("THIS WEEK");
        mButtons->setR1(Widgets::Buttons::NONE);
        return;
    }
    snprintf(mTitleText, sizeof(mTitleText), "WEEK %u OF %u", static_cast<unsigned>(w.runWeek + 1),
             static_cast<unsigned>(w.weeks));
    mTitle->setText(mTitleText);
    uint8_t first = 0xFF;
    uint8_t today = 0xFF;
    for (uint8_t i = 0; i < 7; ++i) {
        const auto& d = w.day[i];
        const DayMark m = static_cast<DayMark>(d.mark);
        lv_label_set_text(mDay[i], TimeText::weekdayName(i));
        lv_label_set_text(mName[i], PlanText::shortName(static_cast<Kind>(d.kind)));
        lv_obj_set_style_text_color(mName[i], Theme::rgb(markColor(m)), LV_PART_MAIN);
        lv_obj_set_style_text_color(mDay[i], Theme::rgb(m == DayMark::Today ? Color::CYAN : Color::GRAY), LV_PART_MAIN);
        if (m != DayMark::Rest && first == 0xFF) {
            first = i;
        }
        if (d.date == mModel.todayDate()) {
            today = i;
        }
    }
    select(today != 0xFF ? today : (first != 0xFF ? first : 0));
}

void WeekScreen::select(uint8_t row)
{
    using namespace XTraining;
    if (!mOk || row >= 7) {
        return;
    }
    mSel = row;
    const auto& d = mModel.getPlanWeek().day[row];
    const DayMark m = static_cast<DayMark>(d.mark);
    lv_obj_set_y(mBand, kRowY0 + kRowPitch * row - (kRowPitch + 2) / 2);
    Theme::setHidden(mBand, false);
    // The state in words (the colours say it too), and the week's key session; the day page
    // has the date.
    snprintf(mInfoText, sizeof(mInfoText), "%s%s", markWords(m), d.key && m != DayMark::Rest ? ", key" : "");
    lv_label_set_text(mInfo, mInfoText);
    mButtons->setR1(m == DayMark::Rest ? Widgets::Buttons::NONE : Widgets::Buttons::AMBER);
}

void WeekScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1:
            if (mSel > 0) {
                select(static_cast<uint8_t>(mSel - 1));
            }
            break;
        case Btn::L2:
            if (mSel < 6) {
                select(static_cast<uint8_t>(mSel + 1));
            }
            break;
        case Btn::R1:
            if (mOk && static_cast<DayMark>(mModel.getPlanWeek().day[mSel].mark) != DayMark::Rest) {
                mModel.setChosenDay(mModel.getPlanWeek().day[mSel].date);
                ScreenManager::instance().goTo(ScreenId::PlanDay);
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Main);
            break;
        default:
            break;
    }
}
