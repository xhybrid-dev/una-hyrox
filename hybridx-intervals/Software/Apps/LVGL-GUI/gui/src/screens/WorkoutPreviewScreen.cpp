/**
 ******************************************************************************
 * @file    WorkoutPreviewScreen.cpp
 * @brief   HybridX Intervals: a workout's steps (see WorkoutPreviewScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/WorkoutPreviewScreen.hpp"

#include <cstdio>
#include <cstring>

#include "WorkoutText.hpp"
#include "gui/WorkoutFormat.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;
using F = Theme::Font;

namespace
{
constexpr int32_t kNameMaxW = 116;   ///< the SDK title label is 120 px wide
constexpr int32_t kRowY     = 76;
constexpr int32_t kRowH     = 22;
} // namespace

WorkoutPreviewScreen::WorkoutPreviewScreen(Model& model)
    : Screen(model)
{
}

void WorkoutPreviewScreen::build()
{
    mSummaryLabel = Theme::label(mRoot, F::Regular16, "", 30, 48, 180);
    for (uint8_t i = 0; i < kRows; ++i) {
        mRowLabels[i] = Theme::label(mRoot, F::Regular16, "", 36, kRowY + kRowH * i, 172, LV_TEXT_ALIGN_LEFT);
    }
    mMore    = Theme::label(mRoot, F::Regular16, "", 40, kRowY + kRowH * kRows, 160, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mLoading = Theme::label(mRoot, F::Italic18, "Reading workout...", 20, 110, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "");
}

bool WorkoutPreviewScreen::ready() const
{
    // The loaded workout is the one being previewed (its file matches).
    return mIndex >= 0 && mModel.hasWorkout() &&
           std::strcmp(mModel.workoutInfo().file, mModel.workoutAt(static_cast<uint8_t>(mIndex)).file) == 0;
}

void WorkoutPreviewScreen::onShow()
{
    mModel.resetIdleTimer();
    mPrevious = mModel.selectedWorkout();
    mIndex    = mModel.previewWorkout();
    if (mIndex < 0 || mIndex >= static_cast<int8_t>(mModel.workoutCount())) {
        ScreenManager::instance().goTo(ScreenId::WorkoutList);
        return;
    }
    static const lv_font_t* const kFaces[] = { Theme::font(F::Italic18) };
    WorkoutFmt::name(mName, sizeof(mName), mModel.workoutAt(static_cast<uint8_t>(mIndex)).name);
    WorkoutFmt::fit(mName, sizeof(mName), kFaces, 1, kNameMaxW);
    mTitle->setText(mName);

    if (ready()) {
        show();
    } else {
        mModel.selectWorkout(mIndex);   // the service answers with onWorkout()
    }
}

void WorkoutPreviewScreen::onWorkout()
{
    show();
}

void WorkoutPreviewScreen::show()
{
    if (!ready()) {
        lv_label_set_text(mLoading, mModel.hasWorkout() ? "Reading workout..." : "Can't read this workout");
        return;
    }
    Theme::setHidden(mLoading, true);
    Intervals::Text::summary(mSummary, sizeof(mSummary), mModel.workoutInfo().summary, mModel.isUnitsImperial());
    static const lv_font_t* const kFaces[] = { Theme::font(F::Regular16), Theme::font(F::Regular14) };
    lv_obj_set_style_text_font(mSummaryLabel, WorkoutFmt::fit(mSummary, sizeof(mSummary), kFaces, 2, 176), 0);
    lv_label_set_text(mSummaryLabel, mSummary);
    makeLines();
    mTop = 0;
    render();
}

void WorkoutPreviewScreen::makeLines()
{
    const Intervals::Workout& w   = mModel.workout();
    const bool                imp = mModel.isUnitsImperial();
    mLineCount                    = 0;
    bool inBlock                  = false;

    for (uint8_t i = 0; i < w.stepCount && mLineCount < kMaxLines; ++i) {
        const Intervals::Step& step = w.steps[i];
        if (step.durationType == Intervals::DurationKind::RepeatUntilStepsComplete) {
            inBlock = false;   // the block ends at its marker
            continue;
        }
        // A block starts here if a later marker loops back to this step.
        for (uint8_t m = static_cast<uint8_t>(i + 1); m < w.stepCount; ++m) {
            const Intervals::Step& marker = w.steps[m];
            if (marker.durationType == Intervals::DurationKind::RepeatUntilStepsComplete &&
                marker.durationValue == i && mLineCount < kMaxLines) {
                Line& h = mLines[mLineCount++];
                std::snprintf(h.text, sizeof(h.text), "%u x", static_cast<unsigned>(marker.repeatCount));
                h.color = Color::YELLOW_DARK;
                inBlock = true;
                break;
            }
        }

        Line& line = mLines[mLineCount++];
        char  text[28];
        Intervals::Text::stepLine(text, sizeof(text), step, imp);
        std::snprintf(line.text, sizeof(line.text), "%s%s", inBlock ? "  " : "", text);
        line.color = Color::WHITE;

        if (step.target.kind != Intervals::TargetKind::Open && mLineCount < kMaxLines) {
            Line& t = mLines[mLineCount++];
            char  target[24];
            Intervals::Text::target(target, sizeof(target), step.target, imp);
            std::snprintf(t.text, sizeof(t.text), "%s  @ %s", inBlock ? "  " : "", target);
            t.color = Color::GRAY;
        }
    }
}

void WorkoutPreviewScreen::render()
{
    for (uint8_t r = 0; r < kRows; ++r) {
        const uint8_t i = static_cast<uint8_t>(mTop + r);
        if (i < mLineCount) {
            lv_label_set_text(mRowLabels[r], mLines[i].text);
            lv_obj_set_style_text_color(mRowLabels[r], Theme::rgb(mLines[i].color), 0);
        } else {
            lv_label_set_text(mRowLabels[r], "");
        }
    }
    const uint8_t below = mLineCount > mTop + kRows ? static_cast<uint8_t>(mLineCount - mTop - kRows) : 0;
    if (below > 0) {
        char more[16];
        std::snprintf(more, sizeof(more), "%u more", static_cast<unsigned>(below));
        lv_label_set_text(mMore, more);
    } else {
        lv_label_set_text(mMore, "R1 to start");
    }
}

void WorkoutPreviewScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    mModel.resetIdleTimer();
    switch (code) {
        case Btn::L1:
            if (mTop > 0) {
                --mTop;
                render();
            }
            break;
        case Btn::L2:
            if (mTop + kRows < mLineCount) {
                ++mTop;
                render();
            }
            break;
        case Btn::R1:
            if (ready()) {
                // Start, as RunLVGL's intervals menu did: the countdown with a
                // GPS fix, else the "no GPS, start anyway?" question.
                if (mModel.hasGpsFix()) {
                    ScreenManager::instance().goTo(ScreenId::TrackIntervalsCountdown);
                } else {
                    mModel.setPendingIntervalsMode(true);
                    ScreenManager::instance().goTo(ScreenId::TrackStartConfirm);
                }
            }
            break;
        case Btn::R2:
            if (mPrevious != mIndex) {
                mModel.selectWorkout(mPrevious);   // put back what was chosen before
            }
            ScreenManager::instance().goTo(ScreenId::WorkoutList);
            break;
        default:
            break;
    }
}
