/**
 ******************************************************************************
 * @file    Widgets.hpp
 * @brief   HybridX Streak's small drawn pieces, and the SDK's button hints.
 *
 * Each is one LVGL object drawing flat shapes through Paint. Anything that
 * animates is its own small object, so a pulse or a wave redraws a few
 * hundred pixels rather than the screen (a full-screen redraw costs about
 * 40 ms of the watch's 100 ms frame; RunLVGL ARCHITECTURE.md).
 ******************************************************************************
 */

#ifndef STREAK_WIDGETS_HPP
#define STREAK_WIDGETS_HPP

#include <cstdint>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"
#include "SDK/GUI/LVGL/Title.hpp"
#include "SDK/GUI/LVGL/WheelMenu.hpp"

#include "gui/summit/SummitGeometry.hpp"

namespace Widgets
{

using Buttons = SDK::LVGL::Buttons;

/// The SDK's scroll-wheel menu (the UNA apps' menus) in the Streak's faces:
/// SemiBold 25 selected (the activity apps' 30 is too wide for "Log a
/// session"), Medium 18 around it, Italic 18 hints.
class Wheel : public SDK::LVGL::WheelMenu
{
public:
    Wheel(lv_obj_t* parent, const Item* items, uint16_t count);
};

/// The SDK's screen title (text over a short rule) in Italic 18, as RunLVGL.
class Title : public SDK::LVGL::Title
{
public:
    Title(lv_obj_t* parent, const char* text);
};

/// "You are here": a lime bead in a white ring, with a slow breathing halo.
class Climber
{
public:
    explicit Climber(lv_obj_t* parent);
    ~Climber();
    void moveTo(Summit::Pt p);
    void setVisible(bool visible);

private:
    static void drawCb(lv_event_t* e);
    static void pulseCb(void* var, int32_t v);
    lv_obj_t* mObj   = nullptr;
    int32_t   mPulse = 0;
};

/// Eight lime rays thrown out from a point: the "step up" flourish.
class Burst
{
public:
    explicit Burst(lv_obj_t* parent);
    ~Burst();
    void play(Summit::Pt centre);

private:
    static void drawCb(lv_event_t* e);
    static void growCb(void* var, int32_t v);
    static void doneCb(lv_anim_t* a);
    lv_obj_t* mObj = nullptr;
    int32_t   mLen = 0;
};

/// This week: one bead per session of the target, lime when done, plus any
/// bonus sessions in cyan. The newest bead can "pop".
class WeekPips
{
public:
    WeekPips(lv_obj_t* parent, int32_t y);
    ~WeekPips();
    /// Lays the pips out centred on x, returning their total width.
    int32_t set(uint8_t target, uint8_t sessions, int32_t centreX);
    void pop();   ///< animate the most recent session's bead
    int32_t width() const { return mWidth; }
    lv_obj_t* obj() const { return mObj; }

private:
    static void drawCb(lv_event_t* e);
    static void popCb(void* var, int32_t v);
    lv_obj_t* mObj      = nullptr;
    uint8_t   mTarget   = 3;
    uint8_t   mSessions = 0;
    int32_t   mWidth    = 0;
    int32_t   mPop      = 0;
};

/// A flag on a pole that waves, for the summit screen.
class Flag
{
public:
    Flag(lv_obj_t* parent, Summit::Pt foot, int32_t poleHeight);
    ~Flag();

private:
    static void drawCb(lv_event_t* e);
    static void waveCb(void* var, int32_t v);
    lv_obj_t* mObj  = nullptr;
    int32_t   mPole = 0;
    int32_t   mWave = 0;
};

/// The streak shield: a cyan shield with a little mountain on it.
lv_obj_t* shieldGlyph(lv_obj_t* parent, int32_t centreX, int32_t topY);

/// A sun rising behind low hills; animates up on creation.
class Sunrise
{
public:
    explicit Sunrise(lv_obj_t* parent);
    ~Sunrise();

private:
    static void drawCb(lv_event_t* e);
    static void riseCb(void* var, int32_t v);
    lv_obj_t* mObj  = nullptr;
    int32_t   mSunY = 0;
};

} // namespace Widgets

#endif // STREAK_WIDGETS_HPP
