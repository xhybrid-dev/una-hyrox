/**
 ******************************************************************************
 * @file    WorkoutListScreen.cpp
 * @brief   HybridX Intervals: choose a workout (see WorkoutListScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/WorkoutListScreen.hpp"

#include <cstdio>

#include "WorkoutText.hpp"
#include "gui/WorkoutFormat.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;

namespace
{
/// The selected item's faces, largest first, and the room it has in the lens.
const lv_font_t* const kNameFaces[] = {
    Theme::font(Theme::Font::SemiBold30),
    Theme::font(Theme::Font::SemiBold25),
    Theme::font(Theme::Font::SemiBold20),
    Theme::font(Theme::Font::Medium18),   // the face of the items around it
};
constexpr int32_t kNameMaxW = 204;
} // namespace

WorkoutListScreen::WorkoutListScreen(Model& model)
    : Screen(model)
{
}

bool WorkoutListScreen::canOpen(uint16_t item) const
{
    // A bike workout or an unreadable file can be seen in the list, not opened.
    return isWorkout(item) && mModel.workoutAt(static_cast<uint8_t>(item - 1)).runnable;
}

void WorkoutListScreen::fill()
{
    using Style    = WheelMenu::Item::Style;
    const bool imp = mModel.isUnitsImperial();
    mItemCount     = 0;

    WheelMenu::Item& none = mItems[mItemCount++];
    none                  = WheelMenu::Item {};
    none.style            = Style::Tip;
    none.text             = "No workout";
    none.tip              = "a plain run";
    none.tipColor         = Color::GRAY;

    for (uint8_t i = 0; i < mModel.workoutCount() && mItemCount < kMaxItems - 1; ++i) {
        const Intervals::WorkoutInfo& w = mModel.workoutAt(i);
        WorkoutFmt::name(mNames[mItemCount], sizeof(mNames[0]), w.name);
        const lv_font_t* face = WorkoutFmt::fit(mNames[mItemCount], sizeof(mNames[0]), kNameFaces,
                                                sizeof(kNameFaces) / sizeof(kNameFaces[0]), kNameMaxW);
        uint32_t tipColor = Color::GRAY;
        if (w.error != Intervals::ParseError::Ok) {
            std::snprintf(mTips[mItemCount], sizeof(mTips[0]), "%s", Intervals::Text::problem(w.error, w.validation));
            tipColor = Color::RED;
        } else if (!w.runnable) {
            std::snprintf(mTips[mItemCount], sizeof(mTips[0]), "bike: not yet");
        } else {
            Intervals::Text::summary(mTips[mItemCount], sizeof(mTips[0]), w.summary, imp);
        }
        WheelMenu::Item& item = mItems[mItemCount];
        item                  = WheelMenu::Item {};
        item.style            = Style::Tip;
        item.text             = mNames[mItemCount];
        item.centerFont       = face;
        item.tip              = mTips[mItemCount];
        item.tipColor         = tipColor;
        ++mItemCount;
    }

    WheelMenu::Item& add = mItems[mItemCount++];
    add                  = WheelMenu::Item {};
    add.style            = Style::Tip;
    add.text             = "Add workouts";
    add.tip              = mModel.workoutsTruncated() ? "16 shown: remove some" : "copy files by USB";
    add.tipColor         = Color::GRAY;
}

void WorkoutListScreen::build()
{
    fill();
    mMenu = std::make_unique<WheelMenu>(mRoot, mItems, mItemCount);
    mMenu->setSlideMidCallback([](void* ctx, uint16_t) { static_cast<WorkoutListScreen*>(ctx)->updateButtons(); },
                               this);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "WORKOUTS");
}

void WorkoutListScreen::onShow()
{
    mModel.resetIdleTimer();
    // Start on the workout just previewed (back from its preview), else the
    // one in use. The list starts with "No workout", so workout i is item i + 1.
    const int8_t sel = mModel.previewWorkout() >= 0 ? mModel.previewWorkout() : mModel.selectedWorkout();
    mMenu->select(sel >= 0 && sel < static_cast<int8_t>(mModel.workoutCount()) ? static_cast<uint16_t>(sel + 1) : 0);
    updateButtons();
}

void WorkoutListScreen::onWorkouts()
{
    // The service re-sends the list after every choice. The same workouts: new
    // text in the same items. A different number of them: build the screen
    // afresh (a WheelMenu leaves its objects behind when destroyed).
    const uint16_t count = mItemCount;
    fill();
    if (mItemCount != count) {
        ScreenManager::instance().goTo(ScreenId::WorkoutList);
        return;
    }
    mMenu->refresh();
    updateButtons();
}

void WorkoutListScreen::updateButtons()
{
    const uint16_t sel     = mMenu->selected();
    const bool     canPick = sel == 0 || canOpen(sel);
    mMenu->setBackground(canPick ? Color::TEAL_DARK : Color::GRAY_DARK);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  canPick ? Widgets::Buttons::AMBER : Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
}

void WorkoutListScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    mModel.resetIdleTimer();
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: {
            const uint16_t sel = mMenu->selected();
            if (sel == 0) {
                mModel.selectWorkout(-1);
                mModel.setPreviewWorkout(-1);
                ScreenManager::instance().goTo(ScreenId::Main);
            } else if (canOpen(sel)) {
                mModel.setPreviewWorkout(static_cast<int8_t>(sel - 1));
                ScreenManager::instance().goTo(ScreenId::WorkoutPreview);
            }
        } break;
        case Btn::R2:
            mModel.setPreviewWorkout(-1);
            ScreenManager::instance().goTo(ScreenId::Main);
            break;
        default: break;
    }
}
