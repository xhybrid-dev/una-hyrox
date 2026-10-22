/**
 ******************************************************************************
 * @file    SessionScreen.hpp
 * @brief   X Sauna's interim screen: plain text controls for a session. SA3
 *          replaces it with the real design.
 *
 *          Four views:
 *          - Menu:    R1 Sauna + plunge, L1 Just sauna, L2 Rest on/off (sauna +
 *                     plunge only), R2 exit.
 *          - Live:    the phase, the lap's time, heart rate and the total.
 *                     R1 next phase, L1 pause / resume, hold R2 to stop.
 *          - Confirm: R1 save, L1 discard, R2 back to the session.
 *          - Done:    what the session came to. R1 new session, R2 exit.
 *
 *          The R2 hold is timed here from the press and release codes, by
 *          HoldTimer (the kernel sends no long-press code). The temperature is
 *          not shown: it is recorded for later (it is not a measurement).
 ******************************************************************************
 */

#ifndef SESSION_SCREEN_HPP
#define SESSION_SCREEN_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "HoldTimer.hpp"
#include "SessionSnapshot.hpp"
#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class SessionScreen : public ModelListener
{
public:
    /// How long the R2 hold takes to ask about stopping.
    static constexpr uint32_t kStopHoldMs = 1200;
    /// How long a hint stays on the bottom line.
    static constexpr uint32_t kHintMs = 1800;

    explicit SessionScreen(Model& model);
    ~SessionScreen();

    SessionScreen(const SessionScreen&)            = delete;
    SessionScreen& operator=(const SessionScreen&) = delete;

    /// The LVGL screen object, to pass to lv_screen_load().
    lv_obj_t* root() const { return mRoot; }

    // ModelListener
    void onSessionUpdate(const Sauna::SessionSnapshot& snapshot) override;
    void onSessionSaved(const Sauna::SavedSession& saved) override;
    void onSuspend() override;

private:
    enum class View : uint8_t { Menu, Live, Confirm, Done };

    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);
    static void tickCb(lv_timer_t* t);

    void tick();
    void updateTimer();
    void setView(View v);
    void draw();
    void drawMenu();
    void drawLive();
    void drawConfirm();
    void drawDone();

    void text(lv_obj_t* label, const char* s, uint32_t color, const lv_font_t* font);
    void hint(const char* s);

    Model&    mModel;
    lv_obj_t* mRoot  = nullptr;
    lv_obj_t* mTitle = nullptr;
    lv_obj_t* mBig   = nullptr;
    lv_obj_t* mL[6]  = {};          ///< Text lines, top to bottom

    Sauna::SessionSnapshot mSnap {};
    Sauna::SavedSession    mSaved {};
    View                   mView = View::Menu;
    bool                   mRest = false;     ///< Menu: Rest in Sauna + plunge

    Sauna::HoldTimer mStop { kStopHoldMs };
    lv_timer_t*      mTimer = nullptr;
    char             mHint[32] = {};
    bool             mHasHint = false;
    uint32_t         mHintAt = 0;

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // SESSION_SCREEN_HPP
