/**
 ******************************************************************************
 * @file    Widgets.cpp
 * @brief   HybridX Streak's small drawn pieces (see the header).
 ******************************************************************************
 */

#include "gui/widgets/Widgets.hpp"

#include "gui/theme/Theme.hpp"
#include "gui/widgets/Paint.hpp"

using Summit::Pt;

namespace Widgets
{

namespace
{
Pt P(int32_t x, int32_t y)
{
    return { static_cast<int16_t>(x), static_cast<int16_t>(y) };
}

/// A looping ping-pong animation on `var`, the way every breathing thing here moves.
void pingPong(void* var, lv_anim_exec_xcb_t cb, int32_t from, int32_t to, uint32_t ms)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, var);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, ms);
    lv_anim_set_reverse_duration(&a, ms);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, cb);
    lv_anim_start(&a);
}
} // namespace

// --- Climber -----------------------------------------------------------------

namespace
{
constexpr int32_t kClimberBox = 26;   // room for the halo at its widest
}

Climber::Climber(lv_obj_t* parent)
{
    mObj = Paint::surface(parent, 0, 0, kClimberBox, kClimberBox, &Climber::drawCb, this);
    pingPong(this, &Climber::pulseCb, 0, 3, 900);
}

Climber::~Climber()
{
    lv_anim_delete(this, nullptr);
}

void Climber::moveTo(Pt p)
{
    lv_obj_set_pos(mObj, p.x - kClimberBox / 2, p.y - kClimberBox / 2);
}

void Climber::setVisible(bool visible)
{
    Theme::setHidden(mObj, !visible);
}

void Climber::drawCb(lv_event_t* e)
{
    const auto*         self = static_cast<const Climber*>(lv_event_get_user_data(e));
    const Paint::Canvas c    = Paint::Canvas::of(e);
    const Pt            mid  = P(kClimberBox / 2, kClimberBox / 2);
    Paint::ring(c, mid, 9 + self->mPulse, 1, Palette::kWin);
    Paint::disc(c, mid, 6, Palette::kText);
    Paint::disc(c, mid, 4, Palette::kWin);
}

void Climber::pulseCb(void* var, int32_t v)
{
    auto* self = static_cast<Climber*>(var);
    if (self->mPulse != v) {
        self->mPulse = v;
        lv_obj_invalidate(self->mObj);
    }
}

// --- Burst -------------------------------------------------------------------

namespace
{
constexpr int32_t kBurstBox = 48;
// Unit directions of the eight rays, x16.
constexpr int8_t kRay[8][2] = { { 16, 0 }, { 11, 11 }, { 0, 16 }, { -11, 11 },
                                { -16, 0 }, { -11, -11 }, { 0, -16 }, { 11, -11 } };
}

Burst::Burst(lv_obj_t* parent)
{
    mObj = Paint::surface(parent, 0, 0, kBurstBox, kBurstBox, &Burst::drawCb, this);
    Theme::setHidden(mObj, true);
}

Burst::~Burst()
{
    lv_anim_delete(this, nullptr);
}

void Burst::play(Pt centre)
{
    lv_obj_set_pos(mObj, centre.x - kBurstBox / 2, centre.y - kBurstBox / 2);
    Theme::setHidden(mObj, false);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, 0, 12);
    lv_anim_set_duration(&a, 500);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, &Burst::growCb);
    lv_anim_set_completed_cb(&a, &Burst::doneCb);
    lv_anim_start(&a);
}

void Burst::drawCb(lv_event_t* e)
{
    const auto*         self = static_cast<const Burst*>(lv_event_get_user_data(e));
    const Paint::Canvas c    = Paint::Canvas::of(e);
    const int32_t       mid  = kBurstBox / 2;
    const int32_t       r0   = 10 + self->mLen / 2;
    const int32_t       r1   = 10 + self->mLen;
    for (const auto& d : kRay) {
        Paint::line(c, P(mid + d[0] * r0 / 16, mid + d[1] * r0 / 16), P(mid + d[0] * r1 / 16, mid + d[1] * r1 / 16), 2,
                    Palette::kWin);
    }
}

void Burst::growCb(void* var, int32_t v)
{
    auto* self = static_cast<Burst*>(var);
    self->mLen = v;
    lv_obj_invalidate(self->mObj);
}

void Burst::doneCb(lv_anim_t* a)
{
    auto* self = static_cast<Burst*>(a->var);
    Theme::setHidden(self->mObj, true);
}

// --- WeekPips ----------------------------------------------------------------

namespace
{
constexpr int32_t kPipPitch  = 16;
constexpr int32_t kPipRadius = 5;
constexpr int32_t kPipBox    = 20;
constexpr uint8_t kMaxBonus  = 3;
}

WeekPips::WeekPips(lv_obj_t* parent, int32_t y)
{
    mObj = Paint::surface(parent, 0, y, kPipBox, kPipBox, &WeekPips::drawCb, this);
}

WeekPips::~WeekPips()
{
    lv_anim_delete(this, nullptr);
}

int32_t WeekPips::set(uint8_t target, uint8_t sessions, int32_t centreX)
{
    mTarget   = target == 0 ? 1 : target;
    mSessions = sessions;
    uint8_t bonus = sessions > mTarget ? static_cast<uint8_t>(sessions - mTarget) : 0;
    if (bonus > kMaxBonus) {
        bonus = kMaxBonus;
    }
    const int32_t count = mTarget + bonus;
    mWidth = (count - 1) * kPipPitch + kPipBox;
    lv_obj_set_size(mObj, mWidth, kPipBox);
    lv_obj_set_x(mObj, centreX - mWidth / 2);
    lv_obj_invalidate(mObj);
    return mWidth;
}

void WeekPips::pop()
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, 0, 4);
    lv_anim_set_duration(&a, 220);
    lv_anim_set_reverse_duration(&a, 220);
    lv_anim_set_exec_cb(&a, &WeekPips::popCb);
    lv_anim_start(&a);
}

void WeekPips::drawCb(lv_event_t* e)
{
    const auto*         self = static_cast<const WeekPips*>(lv_event_get_user_data(e));
    const Paint::Canvas c    = Paint::Canvas::of(e);
    const int32_t       total = (self->mWidth - kPipBox) / kPipPitch + 1;
    for (int32_t i = 0; i < total; ++i) {
        const Pt   mid    = P(kPipBox / 2 + i * kPipPitch, kPipBox / 2);
        const bool newest = i + 1 == self->mSessions;
        const int32_t r   = kPipRadius + (newest ? self->mPop / 2 : 0);
        if (i >= self->mTarget) {
            Paint::disc(c, mid, r - 1, Palette::kShield);   // a bonus session
        } else if (i < self->mSessions) {
            Paint::disc(c, mid, r, Palette::kWin);
        } else {
            Paint::ring(c, mid, kPipRadius, 2, Palette::kTrail);
        }
    }
}

void WeekPips::popCb(void* var, int32_t v)
{
    auto* self = static_cast<WeekPips*>(var);
    self->mPop = v;
    lv_obj_invalidate(self->mObj);
}

// --- Flag --------------------------------------------------------------------

Flag::Flag(lv_obj_t* parent, Pt foot, int32_t poleHeight)
    : mPole(poleHeight)
{
    mObj = Paint::surface(parent, foot.x - 2, foot.y - poleHeight - 4, 24, poleHeight + 6, &Flag::drawCb, this);
    pingPong(this, &Flag::waveCb, -2, 2, 380);
}

Flag::~Flag()
{
    lv_anim_delete(this, nullptr);
}

void Flag::drawCb(lv_event_t* e)
{
    const auto*         self = static_cast<const Flag*>(lv_event_get_user_data(e));
    const Paint::Canvas c    = Paint::Canvas::of(e);
    const int32_t       top  = 3;
    Paint::line(c, P(2, top + self->mPole), P(2, top), 2, Palette::kText);
    Paint::tri(c, P(3, top), P(20, top + 6 + self->mWave), P(3, top + 12), Palette::kWin);
}

void Flag::waveCb(void* var, int32_t v)
{
    auto* self = static_cast<Flag*>(var);
    if (self->mWave != v) {
        self->mWave = v;
        lv_obj_invalidate(self->mObj);
    }
}

// --- Shield ------------------------------------------------------------------

namespace
{
void drawShield(lv_event_t* e)
{
    const Paint::Canvas c = Paint::Canvas::of(e);
    // Outer shield.
    Paint::tri(c, P(4, 4), P(60, 4), P(60, 40), Palette::kShield);
    Paint::tri(c, P(4, 4), P(60, 40), P(4, 40), Palette::kShield);
    Paint::tri(c, P(4, 40), P(60, 40), P(32, 70), Palette::kShield);
    // Its dark field.
    Paint::tri(c, P(10, 10), P(54, 10), P(54, 38), SDK::GUI::Color::TEAL_DARK);
    Paint::tri(c, P(10, 10), P(54, 38), P(10, 38), SDK::GUI::Color::TEAL_DARK);
    Paint::tri(c, P(10, 38), P(54, 38), P(32, 62), SDK::GUI::Color::TEAL_DARK);
    // A small mountain with its flag: this shield guards the climb.
    Paint::tri(c, P(32, 22), P(16, 46), P(32, 46), Palette::kSnowLit);
    Paint::tri(c, P(32, 22), P(32, 46), P(48, 46), Palette::kSnowShade);
    Paint::line(c, P(32, 22), P(32, 13), 2, Palette::kText);
    Paint::tri(c, P(33, 13), P(41, 16), P(33, 19), Palette::kWin);
}
} // namespace

lv_obj_t* shieldGlyph(lv_obj_t* parent, int32_t centreX, int32_t topY)
{
    return Paint::surface(parent, centreX - 32, topY, 64, 74, &drawShield, nullptr);
}

// --- Sunrise -----------------------------------------------------------------

namespace
{
constexpr int32_t kSunFrom = 116;
constexpr int32_t kSunTo   = 80;
}

Sunrise::Sunrise(lv_obj_t* parent)
    : mSunY(kSunFrom)
{
    mObj = Paint::surface(parent, 0, 0, 240, 120, &Sunrise::drawCb, this);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, kSunFrom, kSunTo);
    lv_anim_set_duration(&a, 1600);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, &Sunrise::riseCb);
    lv_anim_start(&a);
}

Sunrise::~Sunrise()
{
    lv_anim_delete(this, nullptr);
}

void Sunrise::drawCb(lv_event_t* e)
{
    const auto*         self = static_cast<const Sunrise*>(lv_event_get_user_data(e));
    const Paint::Canvas c    = Paint::Canvas::of(e);
    const Pt            sun  = P(120, self->mSunY);
    for (const auto& d : kRay) {
        Paint::line(c, P(sun.x + d[0] * 32 / 16, sun.y + d[1] * 32 / 16),
                    P(sun.x + d[0] * 41 / 16, sun.y + d[1] * 41 / 16), 2, Palette::kSunRay);
    }
    Paint::disc(c, sun, 24, Palette::kSun);
    // Low hills in front of the sun: the far one, then two near ones.
    Paint::tri(c, P(-20, 116), P(40, 90), P(110, 116), Palette::kFarRange);
    Paint::tri(c, P(-10, 116), P(80, 74), P(170, 116), Palette::kRockShade);
    Paint::tri(c, P(70, 116), P(170, 70), P(250, 116), Palette::kRockLit);
}

void Sunrise::riseCb(void* var, int32_t v)
{
    auto* self = static_cast<Sunrise*>(var);
    self->mSunY = v;
    lv_obj_invalidate(self->mObj);
}

} // namespace Widgets
