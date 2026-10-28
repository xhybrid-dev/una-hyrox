/**
 ******************************************************************************
 * @file    ResultScreen.hpp
 * @brief   What became of the workout: Saved, Not saved (the file could not
 *          be written) or Discarded. It dismisses itself after about 2 s, to
 *          the summary, or to the main menu after a discard.
 *
 * Saved shows the Streak line in amber in place of the sync note ("3 of 4 this
 * week", "8-week streak"; hybridx-link StreakNow), held 3.5 s, when HybridX
 * Streak has left its summary; the service sends it just before SESSION_SAVED.
 *
 * It waits for the service to say which it was (SESSION_SAVED), showing
 * "Saving" until then, and gives up on the wait after five seconds. A stray
 * key does nothing: the R1 that held the hold-to-confirm must not skip this.
 ******************************************************************************
 */

#ifndef RESULT_SCREEN_HPP
#define RESULT_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class ResultScreen : public Screen
{
public:
    explicit ResultScreen(Model& model);
    ~ResultScreen() override;

    void onShow() override;
    void onSessionSaved(const Workout::SavedSession& saved) override;
    void onStreakLine() override;

protected:
    void build() override;

private:
    void showResult();
    /// The Streak line in the note, when there is one. True if it was shown.
    bool showStreakLine();
    void arm(uint32_t ms);
    static void timerCb(lv_timer_t* t);

    lv_obj_t*   mRing    = nullptr;
    lv_obj_t*   mHeading = nullptr;
    lv_obj_t*   mNote    = nullptr;
    lv_timer_t* mTimer   = nullptr;
    bool        mShown   = false;
    bool        mToSummary = false;
};

#endif // RESULT_SCREEN_HPP
