/**
 ******************************************************************************
 * @file    ResultScreen.cpp
 * @brief   Saved, Not saved or Discarded (see ResultScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/ResultScreen.hpp"

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include <cstdio>

using namespace SDK::GUI;

namespace
{
constexpr uint32_t kShowMs = 2000;   // how long the result stays
constexpr uint32_t kStreakShowMs = 3500;   // with the Streak line, as Run, Race and Trail
constexpr uint32_t kWaitMs = 5000;   // how long to wait for the service
} // namespace

ResultScreen::ResultScreen(Model& model)
    : Screen(model)
{
}

ResultScreen::~ResultScreen()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
}

void ResultScreen::build()
{
    using F = Theme::Font;
    mRing    = Theme::arc(mRoot, Widgets::kCx, Widgets::kCy, 113, 6, 0, 360, Color::GRAY_DARK);
    mHeading = Theme::label(mRoot, F::SemiBold30, Strings::kSaving, 0, 62, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mNote    = Theme::label(mRoot, F::Medium18, "", 0, 118, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void ResultScreen::onShow()
{
    mModel.setIdleTimeout();

    if (mModel.discarded()) {
        showResult();
    } else if (mModel.hasSaved()) {
        showResult();
    } else {
        arm(kWaitMs);   // "Saving" until the service answers
    }
}

void ResultScreen::onSessionSaved(const Workout::SavedSession&)
{
    showResult();
}

void ResultScreen::showResult()
{
    if (mShown) {
        return;
    }
    mShown = true;

    uint32_t    color;
    const char* heading;
    const char* note;
    if (mModel.discarded()) {
        color = Color::GRAY;       heading = Strings::kDiscarded; note = Strings::kDiscardedNote;
        mToSummary = false;
    } else if (mModel.saved().saved) {
        color = Color::CHARTREUSE; heading = Strings::kSaved;     note = Strings::kSavedNote;
        mToSummary = true;
    } else {
        color = Color::RED;        heading = Strings::kNotSaved;  note = Strings::kNotSavedNote;
        mToSummary = true;
    }

    Theme::setArcColor(mRing, color);
    Theme::setText(mHeading, heading, color);
    Theme::setText(mNote, note, Color::WHITE);
    const bool streak = mToSummary && mModel.saved().saved && showStreakLine();
    arm(streak ? kStreakShowMs : kShowMs);
}

bool ResultScreen::showStreakLine()
{
    if (!mModel.streakShow() || mModel.streakLine1()[0] == '\0') {
        return false;
    }
    char buf[48];
    if (mModel.streakLine2()[0] != '\0') {
        std::snprintf(buf, sizeof(buf), "%s\n%s", mModel.streakLine1(), mModel.streakLine2());
    } else {
        std::snprintf(buf, sizeof(buf), "%s", mModel.streakLine1());
    }
    Theme::setText(mNote, buf, Color::YELLOW_DARK);
    return true;
}

void ResultScreen::onStreakLine()
{
    // Normally here before the result; if it comes after, it still replaces the note.
    if (mShown && mToSummary && mModel.saved().saved && showStreakLine()) {
        arm(kStreakShowMs);
    }
}

void ResultScreen::arm(uint32_t ms)
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
    mTimer = lv_timer_create(&ResultScreen::timerCb, ms, this);
    lv_timer_set_repeat_count(mTimer, 1);
}

void ResultScreen::timerCb(lv_timer_t* t)
{
    auto* self = static_cast<ResultScreen*>(lv_timer_get_user_data(t));
    self->mTimer = nullptr;   // one-shot: LVGL deletes the timer after this call
    const bool summary = self->mShown && self->mToSummary && self->mModel.saved().summary.laps > 0;
    ScreenManager::instance().goTo(summary ? ScreenId::Summary : ScreenId::Main);
}
