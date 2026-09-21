/**
 ******************************************************************************
 * @file    TrackResultScreen.cpp
 * @brief   The finished screen, and the "Saved" / "Discarded" confirmations.
 ******************************************************************************
 */

#include "gui/screens/TrackResultScreen.hpp"

#include "gui/Assets.hpp"
#include "gui/Format.hpp"
#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{
constexpr uint32_t kDismissMs = 2000;
}  // namespace

TrackResultScreen::TrackResultScreen(Model& model, Result result)
    : Screen(model)
    , mResult(result)
{
}

TrackResultScreen::~TrackResultScreen()
{
    if (mDismiss) {
        lv_timer_delete(mDismiss);
    }
}

void TrackResultScreen::build()
{
    using F = Theme::Font;

    if (mResult == Result::Finished) {
        // Brief 8.2 item 6: total time large, R1 saves, L2 undoes.
        Theme::label(mRoot, F::Italic18, "Race time", 0, 62, 240);
        mTotal = Theme::label(mRoot, F::SemiBold40, "0:00:00", 0, 92, 240);
        mHint = Theme::label(mRoot, F::Medium18, "R1 Save", 0, 160, 240,
                             LV_TEXT_ALIGN_CENTER, SDK::GUI::Color::GRAY);
        mTitle = std::make_unique<Widgets::Title>(mRoot, "Finished");
        mButtons = std::make_unique<Widgets::Buttons>(mRoot);
        return;
    }

    const bool saved = (mResult == Result::Saved);
    Theme::label(mRoot, F::SemiBold30, saved ? "Saved" : "Discarded", 41, 47, 159);
    Theme::imageTinted(mRoot, saved ? &img_circletick_50x50 : &img_circlecross_50x50, 95, 95,
                       SDK::GUI::Color::YELLOW_DARK);
    Theme::label(mRoot, F::Medium18,
                 saved ? "Race has\nbeen saved" : "Race has\nbeen deleted", 48, 156, 144);
    mTitle = std::make_unique<Widgets::Title>(mRoot, Strings::kAppNameUc);
}

void TrackResultScreen::onShow()
{
    mModel.resetIdleTimer();

    if (mResult == Result::Finished) {
        char buf[16];
        Fmt::hms(buf, sizeof(buf),
                 static_cast<std::time_t>(mModel.getRaceData().totalMs / 1000u));
        lv_label_set_text(mTotal, buf);
        mAutoSaveTicks = App::Config::kAutoSaveSteps;

        // Undo is only offered when there is a finishing press to take back.
        // A race ended early was stopped from the menu, and brief 7.3 refuses
        // UNDO_FINISH for it -- so do not advertise a button that does nothing.
        mCanUndo = mModel.raceCompleted();
        lv_label_set_text(mHint, mCanUndo ? "R1 Save    L2 Undo" : "R1 Save");
        mButtons->set(Widgets::Buttons::NONE,
                      mCanUndo ? Widgets::Buttons::WHITE : Widgets::Buttons::NONE,
                      Widgets::Buttons::GREEN, Widgets::Buttons::NONE);
        return;
    }

    if (mResult == Result::Saved) {
        mModel.raceSave();
    } else {
        mModel.raceDiscard();
    }

    mDismiss = lv_timer_create(&TrackResultScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void TrackResultScreen::onKey(uint8_t code)
{
    if (mResult != Result::Finished) {
        return;  // the confirmations dismiss themselves
    }

    namespace Btn = SDK::GUI::Button;
    switch (code) {
    case Btn::R1:
        ScreenManager::instance().goTo(ScreenId::RaceSaved);
        break;

    case Btn::L2:
        // Undo the finishing press and carry on racing (brief 7.3, F5).
        if (mCanUndo) {
            mModel.raceUndoFinish();
            ScreenManager::instance().goTo(ScreenId::Race);
        }
        break;

    default:
        break;
    }
}

void TrackResultScreen::onIdleTimeout()
{
    // Decision D4: auto-save after 60 s without input rather than sitting on an
    // unsaved race forever. This screen never exits the app on idle.
    if (mResult == Result::Finished) {
        ScreenManager::instance().goTo(ScreenId::RaceSaved);
    }
}

void TrackResultScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<TrackResultScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;  // a one-shot timer deletes itself after this call
    if (self->mResult == Result::Saved) {
        ScreenManager::instance().goTo(ScreenId::RaceSummary);
    } else {
        self->mModel.exitApp();
    }
}
