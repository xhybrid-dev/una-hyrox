/**
 ******************************************************************************
 * @file    ConfirmScreen.cpp
 * @brief   Undo, exclude or include one session (see the header).
 *
 * Laid out like the shield offer: a question, a line saying what happens,
 * a tick beside R1 and a cross beside R2 (UNA's convention).
 ******************************************************************************
 */

#include "gui/screens/ConfirmScreen.hpp"

#include <cstdio>

#include "StreakModel.hpp"
#include "gui/Assets.hpp"
#include "gui/copy/Coach.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{
/// "a run", "a ride", "a strength session": the kind inside a sentence.
const char* lowerKind(Streak::Kind k)
{
    switch (k) {
        case Streak::Kind::Run:      return "run";
        case Streak::Kind::Ride:     return "ride";
        case Streak::Kind::Walk:     return "walk";
        case Streak::Kind::Strength: return "strength session";
        case Streak::Kind::Workout:  return "workout";
        case Streak::Kind::Hybrid:   return "hybrid session";
        case Streak::Kind::Row:      return "row";
        default:                     return "session";
    }
}
} // namespace

void ConfirmScreen::build()
{
    const CustomMessage::WeekData& w = mModel.week();
    const uint8_t                  i = mModel.confirmIndex;
    if (i >= w.count) {
        ScreenManager::instance().goTo(ScreenId::Week);
        return;
    }
    const CustomMessage::WeekItem& it   = w.items[i];
    const auto                     kind = static_cast<Streak::Kind>(it.kind < Streak::kKindCount ? it.kind : 7);
    const bool excluded = static_cast<Streak::Status>(it.status) == Streak::Status::Excluded;

    char title[32];
    char body[80];
    if (it.manual) {
        snprintf(title, sizeof(title), "Undo this log?");
        snprintf(body, sizeof(body), "The %s you logged comes off this week.", lowerKind(kind));
    } else if (excluded) {
        snprintf(title, sizeof(title), "Count it again?");
        snprintf(body, sizeof(body), "This %s goes back into this week.", lowerKind(kind));
    } else {
        snprintf(title, sizeof(title), "Leave this out?");
        snprintf(body, sizeof(body), "This %s won't count towards this week.", lowerKind(kind));
    }

    // Below the tick (y 60-77) and clear of the cross (x 187), as the shield offer.
    Theme::label(mRoot, Theme::Font::SemiBold20, title, 30, 90, 180, LV_TEXT_ALIGN_CENTER, Palette::kText);
    lv_obj_t* b = Theme::label(mRoot, Theme::Font::Regular16, body, 40, 122, 140, LV_TEXT_ALIGN_CENTER,
                               Palette::kTextSoft);
    lv_label_set_long_mode(b, LV_LABEL_LONG_WRAP);

    Theme::imageTinted(mRoot, &img_tickgreen_22x17, 186, 60, SDK::GUI::Color::CHARTREUSE);
    Theme::imageTinted(mRoot, &img_crosswhite_17x17, 187, 163, SDK::GUI::Color::WHITE);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::GREEN, Widgets::Buttons::WHITE);
}

void ConfirmScreen::onShow()
{
    mModel.resetIdleTimer();
}

void ConfirmScreen::onHide() {}

void ConfirmScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    const CustomMessage::WeekData& w = mModel.week();
    if (code == Btn::R1 && mModel.confirmIndex < w.count) {
        const bool manual = w.items[mModel.confirmIndex].manual != 0;
        mModel.weekAction(manual ? CustomMessage::WeekAction::Undo : CustomMessage::WeekAction::ToggleExclude,
                          mModel.confirmIndex);
        ScreenManager::instance().goTo(ScreenId::Week);
    } else if (code == Btn::R2) {
        ScreenManager::instance().goTo(ScreenId::Week);
    }
}
