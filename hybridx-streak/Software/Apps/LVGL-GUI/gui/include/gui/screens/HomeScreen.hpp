/**
 ******************************************************************************
 * @file    HomeScreen.hpp
 * @brief   The mountain, the streak, this week, and the coach.
 *
 *   sky and mountain    the current climb; the lime trail is the weeks done
 *   Ben Nevis · 8 weeks to go
 *   7 week streak       (a big lime 7)
 *   ● ● ○  this week
 *   1 more · 3 days left   (grey; amber at risk; lime once banked)
 *
 * When a session arrives the newest bead pops and the coach line becomes a
 * toast ("+1 Run · 42 min"). When it is the one that banks the week, the
 * headline says so, the climber walks up one step, the trail turns lime
 * behind them and a burst marks the spot; at the top of a mountain the
 * summit screen takes over.
 ******************************************************************************
 */

#ifndef STREAK_HOME_SCREEN_HPP
#define STREAK_HOME_SCREEN_HPP

#include <memory>

#include "Summits.hpp"
#include "gui/copy/Coach.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/SummitScene.hpp"
#include "gui/widgets/Widgets.hpp"

class HomeScreen : public Screen
{
public:
    explicit HomeScreen(Model& model);
    ~HomeScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onHomeView() override;
    void onMoments() override;

protected:
    void build() override;

private:
    enum class Stage : uint8_t { Idle, Toast, StepUp, Arrived, Settle, Next };

    void render();
    void render(const Streak::HomeView& v);
    /// The headline: a big lime number beside smaller words, both on one
    /// baseline and centred as a group. Words alone are set larger.
    void setHeadline(const char* number, const char* words, uint32_t wordsColour);
    void layoutWeekRow(const Streak::HomeView& v);
    void setCoach(const Streak::HomeView& v);

    /// A session has just been counted: the toast, the pip, the buzz. In the
    /// demo (autoStep) it also steps up when it completes the week; for real
    /// the service's StepUp moment follows it.
    void sessionArrives(Coach::Sport sport, uint16_t minutes, bool autoStep);
    /// A line of news in the coach's place, for a toast's time.
    void toast(const char* text, uint32_t colour);

    /// Play the service's moments (DESIGN 6), one after another, starting
    /// from the view as it was before them.
    void startMoments();
    void playNext();
    void schedule(Stage next, uint32_t ms);
    void advance();
    void startGlide();

    static void timerCb(lv_timer_t* t);
    static void glideCb(void* var, int32_t v);
    static void glideDoneCb(lv_anim_t* a);

    std::unique_ptr<SummitScene>       mScene;
    std::unique_ptr<Widgets::Climber>  mClimber;
    std::unique_ptr<Widgets::Burst>    mBurst;
    std::unique_ptr<Widgets::WeekPips> mPips;
    std::unique_ptr<Widgets::Buttons>  mButtons;

    lv_obj_t* mMountainLine = nullptr;
    lv_obj_t* mHeadNumber   = nullptr;
    lv_obj_t* mHeadWords    = nullptr;
    lv_obj_t* mWeekLabel    = nullptr;
    lv_obj_t* mCoach        = nullptr;

    lv_timer_t*           mTimer = nullptr;
    Stage                 mStage = Stage::Idle;
    Streak::HomeView      mPending {};   ///< the view once the moment has played out
    Streak::ClimbPosition mFrom {};      ///< where the glide starts
    bool                  mPlaying = false;   ///< playing the service's moments
};

#endif // STREAK_HOME_SCREEN_HPP
