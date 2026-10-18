/**
 ******************************************************************************
 * @file    Screen.hpp
 * @brief   Base class of every Trail Quest screen.
 *
 * A Screen owns one LVGL screen object, receives the kernel's button codes on
 * it (LV_EVENT_KEY), gets a call every frame, and is the Model's listener
 * while it is on display (HybridX Trail's pattern, from the SDK's RunLVGL).
 *
 * What every screen shares, done here once:
 *   - R2 held for kLeaveHoldMs leaves the game from anywhere (a red arc grows
 *     on R2's bezel while it is held). A quick R2 reaches the screen as one
 *     tap (the R2 click code), so a screen can still use R2;
 *   - a click counts only if its press came to this screen outside the beat:
 *     the click that trails a beat's strike, or a press made on the screen
 *     before, is dropped (the watch sends press, click, release in turn);
 *   - while a screen wants every press at once (the timing beat), rawKeys()
 *     passes the codes straight through and R2 does not leave;
 *   - the L1+R2 chord shows the developer numbers (render time, frames, LVGL
 *     pool), as the Q0 probe did, for reading on the watch.
 ******************************************************************************
 */

#ifndef TRAILQUEST_SCREEN_HPP
#define TRAILQUEST_SCREEN_HPP

#include <cstdint>
#include <optional>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "HoldTimer.hpp"
#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class Screen : public ModelListener
{
public:
    /// How long R2 is held to leave the game, ms.
    static constexpr uint32_t kLeaveHoldMs = 1500;

    explicit Screen(Model& model);
    ~Screen() override;

    Screen(const Screen&)            = delete;
    Screen& operator=(const Screen&) = delete;

    /// The empty, styled LVGL screen object (ScreenManager loads it before the
    /// old screen is freed, then create() builds into it).
    void createRoot();
    /// Build the widgets (build()), start the frame timer.
    void create();

    lv_obj_t* root() const { return mRoot; }

    /// Called once the screen is on display and bound to the Model.
    virtual void onShow() {}

    void onSuspend() override;

protected:
    /// Populate mRoot. It exists and has the black background.
    virtual void build() = 0;

    /// A button code (SDK/GUI/Button.hpp). R2 arrives as one tap (the click
    /// code) once the base has told it from a hold, unless rawKeys().
    virtual void onKey(uint8_t code) { (void)code; }

    /// Once a frame (each kernel tick, 10 a second), with lv_tick_get().
    virtual void onFrame(uint32_t nowMs) { (void)nowMs; }

    /// True while every press must reach onKey() at once (the timing beat).
    virtual bool rawKeys() const { return false; }

    /// The bezel arcs (DESIGN.md: amber choose, white back, green confirm, red leave).
    void setArcs(SDK::LVGL::Buttons::Color l1, SDK::LVGL::Buttons::Color l2, SDK::LVGL::Buttons::Color r1,
                 SDK::LVGL::Buttons::Color r2);

    Model&    mModel;
    lv_obj_t* mRoot = nullptr;

private:
    static void keyEventCb(lv_event_t* e);
    static void tickCb(lv_timer_t* t);
    static void renderCb(lv_event_t* e);
    void handleKey(uint8_t code);
    void frame();
    void updateDev(uint32_t nowMs);

    std::optional<SDK::LVGL::Buttons> mButtons;
    bool                              mArmed[4] = {};   ///< L1, L2, R1, R2: a press is waiting for its click.
    TrailQuest::HoldTimer             mLeave { kLeaveHoldMs };
    lv_obj_t*                         mLeaveArc = nullptr;
    lv_timer_t*                       mTimer    = nullptr;

    // The developer numbers (the L1+R2 chord).
    lv_obj_t* mDev        = nullptr;
    char      mDevText[40] = {};
    uint32_t  mDevAt       = 0;
    uint32_t  mRenderStart = 0;
    uint32_t  mRenderLast  = 0;
    uint32_t  mRenderMax   = 0;
    uint32_t  mFrames      = 0;
};

#endif // TRAILQUEST_SCREEN_HPP
