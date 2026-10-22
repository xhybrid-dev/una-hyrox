/**
 ******************************************************************************
 * @file    ResultScreen.hpp
 * @brief   What became of the session: Saved, Not saved (the file could not
 *          be written) or Discarded. It dismisses itself after about 2 s, to
 *          the summary, or to the main menu after a discard.
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
    void onSessionSaved(const Sauna::SavedSession& saved) override;

protected:
    void build() override;

private:
    void showResult();
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
