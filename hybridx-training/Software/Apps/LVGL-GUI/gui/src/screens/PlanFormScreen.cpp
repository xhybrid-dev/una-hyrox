/**
 ******************************************************************************
 * @file    PlanFormScreen.cpp
 * @brief   Form (see the header).
 ******************************************************************************
 */

#include "gui/screens/PlanFormScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/WorkoutUi.hpp"

#include "ReviewInsight.hpp"   // header-only (and the words)
#include "ReviewText.hpp"

#include <cstdio>

namespace Color = SDK::GUI::Color;
using F = Theme::Font;
using Line = TextStack::Line;

namespace
{
using XTraining::Review::Dir;

/// A session's bar: green on pace, yellow too fast, cyan too slow, grey not judged. On the display's
/// grid (each channel 0x00, 0x40, 0x80 or 0xC0).
uint32_t barColor(const CustomMessage::PlanFormInfo::Row& r)
{
    if (!r.judged) {
        return Color::GRAY_DARK;
    }
    if (r.inPct >= XTraining::Review::kOffPacePct) {
        return Color::GREEN;
    }
    return r.dir == static_cast<uint8_t>(Dir::Fast) ? Color::YELLOW_DARK : Color::CYAN;
}

uint32_t dotColor(uint8_t feel)
{
    using XTraining::Review::isFresh;
    using XTraining::Review::isTired;
    return isFresh(feel) ? Color::TEAL : isTired(feel) ? Color::ORANGE_DARK : Color::WHITE;
}
} // namespace

PlanFormScreen::PlanFormScreen(Model& model)
    : Screen(model)
{
}

PlanFormScreen::~PlanFormScreen() = default;

void PlanFormScreen::build()
{
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::NONE,
                  Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "FORM");
    mText.init(mRoot);
    for (uint8_t i = 0; i < CustomMessage::PlanFormInfo::kMax; ++i) {
        mBar[i] = Theme::box(mRoot, 0, kBaseY - 6, kBarW, 6, Color::GRAY);
        mDot[i] = Theme::box(mRoot, 0, kBaseY + 5, kBarW, 4, Color::WHITE);
        Theme::setHidden(mBar[i], true);
        Theme::setHidden(mDot[i], true);
    }
}

void PlanFormScreen::onShow()
{
    mModel.resetIdleTimer();
    fill();
    mModel.requestPlan(CustomMessage::PlanRequest::Form);
}

void PlanFormScreen::onPlanForm()
{
    fill();
}

void PlanFormScreen::fill()
{
    using namespace XTraining;
    const CustomMessage::PlanFormInfo& f = mModel.getPlanForm();
    for (auto& t : mLine) {
        t[0] = '\0';
    }
    Line lines[4];
    uint8_t n = 0;

    // The bars, centred, oldest to newest.
    const int32_t total = f.count * kBarW + (f.count > 0 ? (f.count - 1) * kGap : 0);
    const int32_t x0 = 120 - total / 2;
    for (uint8_t i = 0; i < CustomMessage::PlanFormInfo::kMax; ++i) {
        if (i >= f.count) {
            Theme::setHidden(mBar[i], true);
            Theme::setHidden(mDot[i], true);
            continue;
        }
        const CustomMessage::PlanFormInfo::Row& r = f.row[i];
        const int32_t h = r.judged ? (r.inPct * kMaxH + 50) / 100 : 4;
        const int32_t height = h < 4 ? 4 : h;
        const int32_t x = x0 + i * (kBarW + kGap);
        lv_obj_set_pos(mBar[i], x, kBaseY - height);
        lv_obj_set_size(mBar[i], kBarW, height);
        lv_obj_set_style_bg_color(mBar[i], Theme::rgb(barColor(r)), LV_PART_MAIN);
        Theme::setHidden(mBar[i], false);
        const bool asked = r.feel != 0;
        if (asked) {
            lv_obj_set_pos(mDot[i], x, kBaseY + 5);
            lv_obj_set_style_bg_color(mDot[i], Theme::rgb(dotColor(r.feel)), LV_PART_MAIN);
        }
        Theme::setHidden(mDot[i], !asked);
    }

    // The words: a caption over the bars is the title's job; under them, the pattern.
    const Review::Line line = static_cast<Review::Line>(f.line);
    ReviewText::insightLine(line, f.lineN, mLine[0], sizeof(mLine[0]));
    if (f.count == 0) {
        lines[n++] = Line{ "No sessions yet", F::Medium18, F::Regular16, Color::WHITE };
        lines[n++] = Line{ "Run one of the plan's", F::Regular16, F::Regular16, Color::GRAY };
        lines[n++] = Line{ "sessions to see it", F::Regular16, F::Regular16, Color::GRAY };
        mText.set(lines, n, 150);
    } else if (mLine[0][0] == '\0') {
        lines[n++] = Line{ "Nothing to flag", F::Medium18, F::Regular16, Color::WHITE };
        lines[n++] = Line{ "bars: time on pace", F::Regular16, F::Regular16, Color::GRAY };
        mText.set(lines, n, 160);
    } else {
        WorkoutUi::wrapTwo(mLine[0], mLine[1], mLine[2], sizeof(mLine[1]), Theme::font(F::Medium18), 180, 170);
        lines[n++] = Line{ mLine[1], F::Medium18, F::Regular16, line == Review::Line::Fast ? Color::YELLOW : Color::WHITE };
        lines[n++] = Line{ mLine[2], F::Medium18, F::Regular16, line == Review::Line::Fast ? Color::YELLOW : Color::WHITE };
        mText.set(lines, n, 160);
    }
}

void PlanFormScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code == Btn::R2 || code == Btn::L1) {
        ScreenManager::instance().goTo(ScreenId::PlanOverview);
    }
}
