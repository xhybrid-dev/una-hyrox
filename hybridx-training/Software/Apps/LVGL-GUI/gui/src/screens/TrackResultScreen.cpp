/**
 ******************************************************************************
 * @file    TrackResultScreen.cpp
 * @brief   "Saved" / "Discarded" confirmation shown for two seconds.
 ******************************************************************************
 */

#include "gui/screens/TrackResultScreen.hpp"
#include "gui/WorkoutUi.hpp"
#include "Date.hpp"
#include "TimeText.hpp"
#include "PlanText.hpp"   // X Training: header-only
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Audit.hpp"
#include "gui/Assets.hpp"
#include "gui/Strings.hpp"

namespace
{
constexpr uint32_t kDismissMs = 2000;
constexpr uint32_t kStreakLineMs = 3500;   ///< with the Streak line, time to read it
constexpr uint32_t kPlanLinesMs  = 3500;   ///< X Training: with the plan's lines
constexpr int32_t  kTextY        = 156;    ///< the confirmation's text (and the Streak line)
constexpr int32_t  kPlanTextY    = 140;    ///< X Training: the plan's two lines
} // namespace

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
    const bool saved = mResult == Result::Saved;
    Theme::label(mRoot, Theme::Font::SemiBold30, saved ? "Saved" : "Discarded", 41, 47, 159);
    mTick = Theme::imageTinted(mRoot, saved ? &img_circletick_50x50 : &img_circlecross_50x50, 95, 95,
                               SDK::GUI::Color::YELLOW_DARK);
    // The session's training load (hybridx-link TrainingLoad.hpp) replaces the tick when it
    // comes with the Streak line: "Saved" above already says what the tick did.
    mLoad = Theme::label(mRoot, Theme::Font::SemiBold25, "", 0, 104, 240);
    mText = Theme::label(mRoot, Theme::Font::Medium18,
                 saved ? "Activity has\nbeen saved" : "Activity has\nbeen deleted", 48, kTextY, 144);
    mTitle = std::make_unique<Widgets::Title>(mRoot, Strings::kAppNameUc);
    Audit::track(mLoad);   // X Training: the plan's lines go in these two
    Audit::track(mText);
}

void TrackResultScreen::onShow()
{
    if (mResult == Result::Saved) {
        mModel.saveTrack();
    } else {
        mModel.resetIdleTimer();
        mModel.discardTrack();
    }
    startDismiss(kDismissMs);
}

void TrackResultScreen::startDismiss(uint32_t ms)
{
    if (mDismiss != nullptr) {
        lv_timer_set_period(mDismiss, ms);
        lv_timer_reset(mDismiss);
        return;
    }
    mDismiss = lv_timer_create(&TrackResultScreen::dismissCb, ms, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void TrackResultScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<TrackResultScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;   // a one-shot timer deletes itself after this call
    if (self->mStreakPending) {
        // X Training: the plan's lines have been read; the Streak line follows.
        self->mStreakPending = false;
        self->mPlanShown     = false;
        self->showStreakLine();
        self->startDismiss(kStreakLineMs);
        return;
    }
    if (self->mResult == Result::Saved) {
        // HybridX Run: a treadmill run asks for the console's distance first.
        ScreenManager::instance().goTo(self->mModel.getTrackData().treadmill ? ScreenId::TreadmillDistance
                                                                            : ScreenId::TrackSummary);
    } else {
        self->mModel.exitApp();
    }
}

void TrackResultScreen::onPlanSaved()
{
    // X Training (brief 8.3): in place of the tick, how the session counts; under it, the
    // week and the next session.
    using Saved = CustomMessage::PlanSavedInfo;
    const Saved& s = mModel.getPlanSaved();
    if (mResult != Result::Saved || s.outcome == Saved::None || mLoad == nullptr || mText == nullptr) {
        return;
    }
    static const char* const kWords[4] = { "", "Done", "Part done", "Not counted" };
    static constexpr uint32_t kColor[4] = { SDK::GUI::Color::WHITE, SDK::GUI::Color::GREEN,
                                            SDK::GUI::Color::YELLOW_DARK, SDK::GUI::Color::GRAY };
    const uint8_t o = s.outcome < 4 ? s.outcome : 0;
    lv_label_set_text(mLoad, kWords[o]);
    lv_obj_set_style_text_color(mLoad, Theme::rgb(kColor[o]), 0);
    if (mTick != nullptr) {
        lv_obj_add_flag(mTick, LV_OBJ_FLAG_HIDDEN);
    }
    char week[32] = "";
    if (s.weekSessions > 0) {
        snprintf(week, sizeof(week), "Week %u: %u of %u", static_cast<unsigned>(s.runWeek + 1),
                 static_cast<unsigned>(s.weekDone), static_cast<unsigned>(s.weekSessions));
    } else if (o == Saved::TooShort) {
        snprintf(week, sizeof(week), "Less than half run");
    }
    char next[40] = "";
    if (s.nextDate != 0) {
        snprintf(next, sizeof(next), "Next: %s, %s",
                 XTraining::TimeText::weekdayName(static_cast<unsigned>(XTraining::Date::weekday(s.nextDate))),
                 XTraining::PlanText::shortName(static_cast<XTraining::Kind>(s.nextKind)));
    }
    // Clear of the arcs at the second line's foot (y 184: 174 px), as the audit measures.
    WorkoutUi::fitWidth(week, sizeof(week), Theme::font(Theme::Font::Regular16), 172);
    WorkoutUi::fitWidth(next, sizeof(next), Theme::font(Theme::Font::Regular16), 172);
    snprintf(mPlanText, sizeof(mPlanText), "%.31s%s%.30s", week, next[0] ? "\n" : "", next);
    lv_label_set_text(mText, mPlanText);
    lv_obj_set_style_text_font(mText, Theme::font(Theme::Font::Regular16), 0);
    lv_obj_set_y(mText, kPlanTextY);   // higher than the confirmation's: two lines, clear of the foot
    lv_obj_set_x(mText, 33);
    lv_obj_set_width(mText, 174);
    mPlanShown = true;
    startDismiss(kPlanLinesMs);
}

void TrackResultScreen::onStreakLine()
{
    if (mPlanShown) {
        mStreakPending = mResult == Result::Saved && (mModel.hasStreakLine() || mModel.sessionLoad() > 0);
        return;   // after the plan's lines
    }
    showStreakLine();
    if (mResult == Result::Saved && mModel.hasStreakLine()) {
        startDismiss(kStreakLineMs);
    }
}

void TrackResultScreen::showStreakLine()
{
    // The Streak line (hybridx-link StreakNow) in place of the confirmation's text, held a
    // little longer to be read.
    if (mResult == Result::Saved && mLoad != nullptr && mModel.sessionLoad() > 0) {
        lv_label_set_text_fmt(mLoad, "Load %u", static_cast<unsigned>(mModel.sessionLoad()));
        if (mTick != nullptr) {
            lv_obj_add_flag(mTick, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (mText == nullptr || !mModel.hasStreakLine()) {
        return;
    }
    lv_obj_set_style_text_font(mText, Theme::font(Theme::Font::Medium18), 0);   // after the plan's lines
    lv_obj_set_y(mText, kTextY);
    if (mModel.streakLine2()[0] != '\0') {
        lv_label_set_text_fmt(mText, "%s\n%s", mModel.streakLine1(), mModel.streakLine2());
    } else {
        lv_label_set_text(mText, mModel.streakLine1());
    }
    lv_obj_set_style_text_color(mText, lv_color_hex(SDK::GUI::Color::YELLOW_DARK), 0);
    // Wider than the confirmation's 144 px, for "12 of 7 this week"; still inside the round face
    // (its chord at y = 200 is 179 px).
    lv_obj_set_x(mText, 35);
    lv_obj_set_width(mText, 170);
}
