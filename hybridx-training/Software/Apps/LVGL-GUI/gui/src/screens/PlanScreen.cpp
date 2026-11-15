/**
 ******************************************************************************
 * @file    PlanScreen.cpp
 * @brief   Plan (see the header).
 ******************************************************************************
 */

#include "gui/screens/PlanScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/WorkoutUi.hpp"

#include "PlanText.hpp"   // header-only
#include "TimeText.hpp"

#include <cstdio>

namespace Color = SDK::GUI::Color;
using F = Theme::Font;
using Line = TextStack::Line;
using Info = CustomMessage::PlanTodayInfo;

namespace
{
/// A phase's colour, bright for the weeks run and dark for the weeks to come. On the
/// display's grid (each channel 0x00, 0x40, 0x80 or 0xC0): no opacity, which blends off it.
uint32_t phaseColor(XTraining::Phase p, bool ahead)
{
    using XTraining::Phase;
    switch (p) {
        case Phase::Base:     return ahead ? Color::TEAL_DARK   : Color::TEAL;
        case Phase::Build:    return ahead ? Color::TEAL        : Color::CYAN;
        case Phase::Peak:     return ahead ? Color::SIENNA_DARK : Color::YELLOW_DARK;
        case Phase::Specific: return ahead ? Color::MAROON      : Color::ORANGE_DARK;
        case Phase::Taper:    return ahead ? Color::GREEN_DARK  : Color::GREEN_MID;
        case Phase::RaceWeek: return ahead ? Color::GREEN_MID   : Color::GREEN;
        case Phase::CutBack:  return ahead ? Color::GRAY_DARK   : Color::GRAY;
        case Phase::Baseline:
        case Phase::Retest:   return ahead ? Color::TEAL_BLUE   : Color::SKY;
    }
    return Color::GRAY;
}

const char* planName(uint8_t plan)
{
    switch (plan) {
        case 1:  return "10K PLAN";
        case 2:  return "HALF PLAN";
        case 3:  return "VO2MAX PLAN";
        default: return "PLAN";
    }
}
} // namespace

PlanScreen::PlanScreen(Model& model)
    : Screen(model)
{
}

PlanScreen::~PlanScreen() = default;

void PlanScreen::build()
{
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    // R1: New plan (brief 5.2), whatever the plan's state.
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, mTitleText);
    mTop.init(mRoot);
    mBottom.init(mRoot);
    for (lv_obj_t*& seg : mBar) {
        seg = Theme::box(mRoot, 0, kBarY - kBarH / 2, 8, kBarH, Color::GRAY);
        Theme::setHidden(seg, true);
    }
    mNow = Theme::box(mRoot, 0, kBarY + kBarH / 2 + 2, 8, 3, Color::WHITE);
    Theme::setHidden(mNow, true);
}

void PlanScreen::onShow()
{
    mModel.resetIdleTimer();
    fill();
    mModel.requestPlan(CustomMessage::PlanRequest::Overview);
}

void PlanScreen::onPlanOverview()
{
    fill();
}

void PlanScreen::fill()
{
    using namespace XTraining;
    const CustomMessage::PlanOverviewInfo& o = mModel.getPlanOverview();
    for (auto& t : mText) {
        t[0] = '\0';
    }
    Line top[3];
    Line bottom[4];
    uint8_t nt = 0;
    uint8_t nb = 0;
    snprintf(mTitleText, sizeof(mTitleText), "%s", planName(o.plan));
    mTitle->setText(mTitleText);

    const bool have = o.plan != 0 && o.weeks != 0;
    if (!have) {
        // No usable plan: as Today says it.
        if (o.status == Info::Problem) {
            top[nt++] = Line{ "Check your plan", F::SemiBold25, F::SemiBold20, Color::WHITE };
            WorkoutUi::wrapTwo(o.problem, mText[0], mText[1], sizeof(mText[0]), Theme::font(F::Medium18), 190, 170);
            bottom[nb++] = Line{ mText[0], F::Medium18, F::Regular16, Color::YELLOW };
            bottom[nb++] = Line{ mText[1], F::Medium18, F::Regular16, Color::YELLOW };
        } else {
            top[nt++] = Line{ "No plan", F::SemiBold25, F::SemiBold20, Color::WHITE };
            bottom[nb++] = Line{ "R1: choose one here,", F::Medium18, F::Regular16, Color::GRAY };
            bottom[nb++] = Line{ "or in the UNA app", F::Medium18, F::Regular16, Color::GRAY };
        }
    } else {
        const Phase phase = static_cast<Phase>(o.phase[o.runWeek < CustomMessage::PlanOverviewInfo::kMaxWeeks ? o.runWeek : 0]);
        char day[24];
        if (o.status == Info::BeforeStart) {
            TimeText::dayDate(o.start, day, sizeof(day));
            snprintf(mText[0], sizeof(mText[0]), "Starts %s", day);
            top[nt++] = Line{ mText[0], F::SemiBold25, F::SemiBold20, Color::WHITE };
        } else if (o.status == Info::Finished) {
            top[nt++] = Line{ "Plan complete", F::SemiBold25, F::SemiBold20, Color::WHITE };
        } else {
            snprintf(mText[0], sizeof(mText[0]), "Week %u of %u", static_cast<unsigned>(o.runWeek + 1),
                     static_cast<unsigned>(o.weeks));
            top[nt++] = Line{ mText[0], F::SemiBold25, F::SemiBold20, Color::WHITE };
        }
        snprintf(mText[1], sizeof(mText[1]), "%s, level %u, %u days", PlanText::phaseName(phase),
                 static_cast<unsigned>(o.level), static_cast<unsigned>(o.runDays));
        top[nt++] = Line{ mText[1], F::Italic18, F::Italic18, Color::TEAL };

        snprintf(mText[2], sizeof(mText[2]), "%u of %u this week", static_cast<unsigned>(o.weekDone),
                 static_cast<unsigned>(o.weekSessions));
        bottom[nb++] = Line{ mText[2], F::Medium18, F::Regular16, Color::WHITE };
        snprintf(mText[3], sizeof(mText[3]), "%u of %u so far", static_cast<unsigned>(o.done),
                 static_cast<unsigned>(o.soFar));
        bottom[nb++] = Line{ mText[3], F::Italic18, F::Italic18, Color::GRAY };
        // "Race 24 Jan" or "Ends 20 Dec": the foot of the face has room for no more.
        const uint32_t last = o.raceDate != 0 ? o.raceDate : o.end;
        snprintf(mText[4], sizeof(mText[4]), "%s %u %s", o.raceDate != 0 ? "Race" : "Ends",
                 static_cast<unsigned>(Date::day(last)), TimeText::monthName(Date::month(last)));
        bottom[nb++] = Line{ mText[4], F::Italic18, F::Italic18, Color::TEAL };
    }
    mTop.set(top, nt, 76);
    mBottom.set(bottom, nb, 164);

    // The bar: one segment a week, centred, coloured by phase; this week marked under it.
    const uint8_t n = have ? (o.weeks < CustomMessage::PlanOverviewInfo::kMaxWeeks ? o.weeks
                                                                                   : CustomMessage::PlanOverviewInfo::kMaxWeeks)
                           : 0;
    const int32_t gap = 3;
    const int32_t seg = n > 0 ? ((150 - gap * (n - 1)) / n > 12 ? 12 : (150 - gap * (n - 1)) / n) : 0;
    const int32_t x0 = 120 - (seg * n + gap * (n > 0 ? n - 1 : 0)) / 2;
    for (uint8_t w = 0; w < CustomMessage::PlanOverviewInfo::kMaxWeeks; ++w) {
        lv_obj_t* s = mBar[w];
        if (w >= n) {
            Theme::setHidden(s, true);
            continue;
        }
        lv_obj_set_pos(s, x0 + w * (seg + gap), kBarY - kBarH / 2);
        lv_obj_set_width(s, seg);
        // The weeks run in full colour; the weeks to come dark.
        const bool run = o.status == Info::Finished || (o.status != Info::BeforeStart && w <= o.runWeek);
        lv_obj_set_style_bg_color(s, Theme::rgb(phaseColor(static_cast<Phase>(o.phase[w]), !run)), LV_PART_MAIN);
        Theme::setHidden(s, false);
    }
    const bool mark = n > 0 && o.status != Info::Finished && o.runWeek < n;
    if (mark) {
        lv_obj_set_pos(mNow, x0 + o.runWeek * (seg + gap), kBarY + kBarH / 2 + 2);
        lv_obj_set_width(mNow, seg);
    }
    Theme::setHidden(mNow, !mark);
}

void PlanScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code == Btn::R2) {
        ScreenManager::instance().goTo(ScreenId::Main);
    } else if (code == Btn::R1) {
        mModel.setDraftReady(false);   // a fresh draft, from the phone's set-up
        ScreenManager::instance().goTo(ScreenId::NewPlan);
    }
}
