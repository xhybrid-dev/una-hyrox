/**
 ******************************************************************************
 * @file    HomeScreen.cpp
 * @brief   The mountain, the streak, this week, and the coach (see the header).
 ******************************************************************************
 */

#include "gui/screens/HomeScreen.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#if HYBRIDXSTREAK_DEMO
#include "gui/demo/Demo.hpp"
#endif

using Streak::Mood;

namespace
{
// Layout (DESIGN.md "Home"). The bottom of a round screen narrows fast: the
// coach line sits where the disc is about 160 px wide.
constexpr int32_t kMountainLineY = 119;
constexpr int32_t kBaseline      = 163;   ///< the headline's shared baseline
constexpr int32_t kWeekRowY      = 172;
constexpr int32_t kCoachY        = 197;
constexpr int32_t kRowGap        = 8;
constexpr int32_t kHeadGap       = 6;
constexpr int32_t kHeadMaxWidth  = 196;   ///< clear of the L2/R2 hint arcs at this height

int32_t textWidth(const char* text, const lv_font_t* font)
{
    lv_point_t size;
    lv_text_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
}

/// Where a label's top must go for its text to sit on `baseline`.
int32_t topFor(const lv_font_t* font, int32_t baseline)
{
    return baseline - (font->line_height - font->base_line);
}

constexpr uint32_t kToastMs      = 2200;   ///< a toast's time on screen
constexpr uint32_t kBeatMs       = 900;    ///< from the bead popping to the step up
constexpr uint32_t kGlideMs      = 1100;   ///< one step up the trail
constexpr uint32_t kSavourMs     = 1700;   ///< "Week complete!" before settling
constexpr uint32_t kToSummitMs   = 900;    ///< from arriving at the top to the summit screen

uint32_t coachColour(Mood mood)
{
    switch (mood) {
        case Mood::Done:   return Palette::kWin;
        case Mood::AtRisk: return Palette::kAtRisk;
        default:           return Palette::kTextSoft;
    }
}

/// The week's tone after a change in sessions (the demo's stand-in for the
/// rules engine, which arrives in phase S1).
Mood moodFor(const Streak::HomeView& v)
{
    if (v.sessions >= v.target) {
        return Mood::Done;
    }
    if (v.mood == Mood::Trial) {
        return Mood::Trial;
    }
    return (v.target - v.sessions) >= v.daysLeft ? Mood::AtRisk : Mood::Climbing;
}
} // namespace

HomeScreen::HomeScreen(Model& model)
    : Screen(model)
{
}

HomeScreen::~HomeScreen()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
    lv_anim_delete(this, nullptr);
}

void HomeScreen::build()
{
    SummitScene::Options opts;
    mScene   = std::make_unique<SummitScene>(mRoot, opts);
    mClimber = std::make_unique<Widgets::Climber>(mRoot);
    mBurst   = std::make_unique<Widgets::Burst>(mRoot);

    mMountainLine = Theme::label(mRoot, Theme::Font::Regular14, "", 20, kMountainLineY, 200,
                                 LV_TEXT_ALIGN_CENTER, Palette::kTextSoft);
    mHeadNumber   = Theme::label(mRoot, Theme::Font::SemiBold30, "", 0, 0, 60, LV_TEXT_ALIGN_LEFT, Palette::kWin);
    mHeadWords    = Theme::label(mRoot, Theme::Font::Medium18, "", 0, 0, 200, LV_TEXT_ALIGN_LEFT, Palette::kText);
    mPips         = std::make_unique<Widgets::WeekPips>(mRoot, kWeekRowY + 1);
    mWeekLabel    = Theme::label(mRoot, Theme::Font::Medium18, "this week", 0, kWeekRowY, 100,
                                 LV_TEXT_ALIGN_LEFT, Palette::kText);
    mCoach        = Theme::label(mRoot, Theme::Font::Regular14, "", 24, kCoachY, 192,
                                 LV_TEXT_ALIGN_CENTER, Palette::kTextSoft);

    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
#if HYBRIDXSTREAK_DEMO
    // L1/L2 step through the scenarios; R1 plays the moment; R2 exits.
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::AMBER,
                  Widgets::Buttons::WHITE);
#else
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::WHITE);
#endif

    render();
}

void HomeScreen::onShow()
{
    mModel.resetIdleTimer();
}

void HomeScreen::onHomeView()
{
    if (mStage == Stage::Idle) {
        render();
    }
}

void HomeScreen::render()
{
    const Streak::HomeView&     v   = mModel.home();
    const Streak::ClimbPosition pos = Streak::climbFor(v.weeksAchieved);

    mScene->setProgress(pos.climb, pos.stepsClimbed, pos.steps);
    mClimber->moveTo(mScene->climberPoint());

    char buf[48];
    Coach::mountainLine(v, buf, sizeof(buf));
    lv_label_set_text(mMountainLine, buf);

    Coach::Headline h;
    Coach::headline(v, h);
    setHeadline(h.number, h.words, Palette::kText);

    layoutWeekRow(v);
    setCoach(v);
}

void HomeScreen::setHeadline(const char* number, const char* words, uint32_t wordsColour)
{
    const lv_font_t* numFont = Theme::font(Theme::Font::SemiBold30);
    const bool       hasNum  = number[0] != '\0';

    // With a number the words are the quiet half; alone they take the stage,
    // as large as the width allows.
    const lv_font_t* wordFont = Theme::font(Theme::Font::Medium18);
    if (!hasNum) {
        wordFont = Theme::font(Theme::Font::SemiBold25);
        if (textWidth(words, wordFont) > kHeadMaxWidth) {
            wordFont = Theme::font(Theme::Font::SemiBold20);
        }
    }

    const int32_t numW   = hasNum ? textWidth(number, numFont) : 0;
    const int32_t wordW  = textWidth(words, wordFont);
    const int32_t total  = numW + (hasNum ? kHeadGap : 0) + wordW;
    const int32_t left   = 120 - total / 2;

    Theme::setHidden(mHeadNumber, !hasNum);
    lv_label_set_text(mHeadNumber, number);
    lv_obj_set_pos(mHeadNumber, left, topFor(numFont, kBaseline));
    lv_obj_set_width(mHeadNumber, numW + 2);

    lv_obj_set_style_text_font(mHeadWords, wordFont, LV_PART_MAIN);
    Theme::setColor(mHeadWords, wordsColour);
    lv_label_set_text(mHeadWords, words);
    lv_obj_set_pos(mHeadWords, left + numW + (hasNum ? kHeadGap : 0), topFor(wordFont, kBaseline));
    lv_obj_set_width(mHeadWords, wordW + 2);
}

void HomeScreen::layoutWeekRow(const Streak::HomeView& v)
{
    lv_point_t size;
    lv_text_get_size(&size, "this week", Theme::font(Theme::Font::Medium18), 0, 0, LV_COORD_MAX,
                     LV_TEXT_FLAG_NONE);
    // Pips and words centred as one group.
    const int32_t pipsW = mPips->set(v.target, v.sessions, 0);
    const int32_t total = pipsW + kRowGap + size.x;
    const int32_t left  = 120 - total / 2;
    mPips->set(v.target, v.sessions, left + pipsW / 2);
    lv_obj_set_x(mWeekLabel, left + pipsW + kRowGap);
    lv_obj_set_width(mWeekLabel, size.x + 2);
}

void HomeScreen::setCoach(const Streak::HomeView& v)
{
    char buf[48];
    Coach::coachLine(v, buf, sizeof(buf));
    lv_label_set_text(mCoach, buf);
    Theme::setColor(mCoach, coachColour(v.mood));
}

void HomeScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
#if HYBRIDXSTREAK_DEMO
    if (code == Btn::L1 || code == Btn::L2) {
        if (mStage != Stage::Idle) {
            return;
        }
        const ScreenId id = mModel.demoGo(code == Btn::L1 ? -1 : 1);
        if (id == ScreenId::Home) {
            render();
        } else {
            ScreenManager::instance().goTo(id);
        }
        return;
    }
    if (code == Btn::R1) {
        if (mStage == Stage::Idle) {
            const Demo::Scenario& s = Demo::kScenarios[mModel.demoIndex()];
            sessionArrives(s.sport, s.minutes);
        }
        return;
    }
#endif
    if (code == Btn::R2) {
        mModel.exitApp();
    }
}

// -- A session arrives ------------------------------------------------------------

void HomeScreen::sessionArrives(Coach::Sport sport, uint16_t minutes)
{
    const Streak::HomeView& now = mModel.home();
    mPending = now;
    mPending.sessions = static_cast<uint8_t>(now.sessions + 1);
    const bool banksTheWeek = mPending.sessions == now.target;
    mPending.mood = moodFor(mPending);

    char buf[40];
    Coach::sessionToast(sport, minutes, buf, sizeof(buf));
    lv_label_set_text(mCoach, buf);
    Theme::setColor(mCoach, Palette::kWin);
    layoutWeekRow(mPending);
    mPips->pop();
    mModel.celebrate(CustomMessage::Moment::SessionFound);

    if (banksTheWeek) {
        mFrom = Streak::climbFor(now.weeksAchieved);
        schedule(Stage::StepUp, kBeatMs);
    } else {
        schedule(Stage::Settle, kToastMs);
    }
}

void HomeScreen::schedule(Stage next, uint32_t ms)
{
    mStage = next;
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
    mTimer = lv_timer_create(&HomeScreen::timerCb, ms, this);
    lv_timer_set_repeat_count(mTimer, 1);
}

void HomeScreen::timerCb(lv_timer_t* t)
{
    auto* self   = static_cast<HomeScreen*>(lv_timer_get_user_data(t));
    self->mTimer = nullptr;   // a one-shot timer is deleted by LVGL after this call
    self->advance();
}

void HomeScreen::advance()
{
    switch (mStage) {
        case Stage::StepUp:
            setHeadline("", "Week complete!", Palette::kWin);
            mModel.celebrate(CustomMessage::Moment::StepUp);
            startGlide();
            break;

        case Stage::Arrived: {
            mPending.weeksAchieved = static_cast<uint16_t>(mPending.weeksAchieved + 1);
            mPending.streakWeeks   = static_cast<uint16_t>(mPending.streakWeeks + 1);
            if (mFrom.stepsClimbed + 1 >= mFrom.steps) {
                // The top: the summit screen takes it from here.
                mModel.setSummited(mFrom.climb);
                mModel.setHome(mPending);
                mStage = Stage::Idle;
                ScreenManager::instance().goTo(ScreenId::Summit);
                return;
            }
            schedule(Stage::Settle, kSavourMs);
            break;
        }

        case Stage::Settle:
            mStage = Stage::Idle;
            mModel.setHome(mPending);   // re-renders through onHomeView()
            break;

        default:
            mStage = Stage::Idle;
            break;
    }
}

void HomeScreen::startGlide()
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_duration(&a, kGlideMs);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, &HomeScreen::glideCb);
    lv_anim_set_completed_cb(&a, &HomeScreen::glideDoneCb);
    lv_anim_start(&a);
}

void HomeScreen::glideCb(void* var, int32_t v)
{
    auto* self = static_cast<HomeScreen*>(var);
    self->mScene->setProgress(self->mFrom.climb, self->mFrom.stepsClimbed, self->mFrom.steps,
                              static_cast<uint32_t>(v));
    self->mClimber->moveTo(self->mScene->climberPoint());
}

void HomeScreen::glideDoneCb(lv_anim_t* a)
{
    auto* self = static_cast<HomeScreen*>(a->var);
    self->mScene->setProgress(self->mFrom.climb, self->mFrom.stepsClimbed + 1u, self->mFrom.steps);
    self->mClimber->moveTo(self->mScene->climberPoint());
    self->mBurst->play(self->mScene->climberPoint());
    self->schedule(Stage::Arrived, kToSummitMs);
}
