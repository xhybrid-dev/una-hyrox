/**
 ******************************************************************************
 * @file    Widgets.cpp
 * @brief   The Run app's own widgets, rebuilt from LVGL primitives.
 ******************************************************************************
 */

#include "gui/widgets/Widgets.hpp"

#include <cmath>
#include "gui/Assets.hpp"
#include "gui/Format.hpp"

#include <cstdio>

using namespace SDK::GUI;

namespace Widgets
{

// --- SensorStatusRow ---------------------------------------------------------

SensorStatusRow::SensorStatusRow(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
    : SDK::LVGL::SensorStatusRow(parent, x, y, w, h, &img_sensorgpslight, &img_sensorhrlight)
{
}

// --- HeartRateZone ---------------------------------------------------------

namespace
{

// Geometry measured from the TouchGFX design's bitmaps (HeartRateZoneGroup and
// HeartRateZone1..5), in the widget's own coordinates: one circle centred at
// (105, 116) carries the five-segment bar, the thicker marker over the active
// segment, and the arrow pointing at it from inside.
constexpr int32_t kArcCx         = 105;
constexpr int32_t kArcCy         = 116;
constexpr int32_t kBarRadius     = 113;   // centre-line radius of the bar
constexpr int32_t kBarWidth      = 8;
constexpr int32_t kMarkerRadius  = 111;   // the marker is thicker and reaches further in
constexpr int32_t kMarkerWidth   = 12;
constexpr int32_t kSegmentDeg    = 24;    // each zone spans 24 degrees, 2 degrees apart
constexpr float   kArrowTipR     = 99.0f; // arrow apex, just inside the marker
constexpr float   kArrowBaseR    = 89.0f;
constexpr float   kArrowHalfW    = 5.5f;  // half the base width, along the tangent
constexpr int32_t kSegmentStart[HeartRateZone::kZoneCount] = { -64, -38, -12, 14, 40 };
constexpr uint32_t kZoneColor[HeartRateZone::kZoneCount] = {
    Color::GRAY, Color::CHARTREUSE, Color::YELLOW, Color::YELLOW_DARK, Color::RED
};

/// Draw the arrow: the object's only content is one filled triangle.
void drawArrowCb(lv_event_t* e)
{
    auto* obj   = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto* arrow = static_cast<const HeartRateZone::Arrow*>(lv_event_get_user_data(e));

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    lv_draw_triangle_dsc_t dsc;
    lv_draw_triangle_dsc_init(&dsc);
    dsc.color = arrow->color;
    dsc.opa   = LV_OPA_COVER;
    for (int i = 0; i < 3; ++i) {
        dsc.p[i].x = coords.x1 + arrow->p[i].x;
        dsc.p[i].y = coords.y1 + arrow->p[i].y;
    }
    lv_draw_triangle(lv_event_get_layer(e), &dsc);
}

} // namespace

HeartRateZone::HeartRateZone(lv_obj_t* parent, int32_t x, int32_t y)
    : mX(x)
    , mY(y)
{
    for (uint8_t i = 0; i < kZoneCount; ++i) {
        Theme::arc(parent, x + kArcCx, y + kArcCy, kBarRadius, kBarWidth,
                   kSegmentStart[i], kSegmentStart[i] + kSegmentDeg, kZoneColor[i], false);
    }

    // One marker and one arrow, re-aimed at whichever zone is active.
    mMarker = Theme::arc(parent, x + kArcCx, y + kArcCy, kMarkerRadius, kMarkerWidth,
                         kSegmentStart[0], kSegmentStart[0] + kSegmentDeg, kZoneColor[0], false);
    lv_obj_add_flag(mMarker, LV_OBJ_FLAG_HIDDEN);

    // The arrow's host covers the whole bar area so any zone's triangle fits.
    mArrow = Theme::container(parent, x, y, 210, 69);
    lv_obj_add_event_cb(mArrow, drawArrowCb, LV_EVENT_DRAW_MAIN, &mArrowDsc);
    lv_obj_add_flag(mArrow, LV_OBJ_FLAG_HIDDEN);
}

void HeartRateZone::showZone(int zone)
{
    if (zone == mActive) {
        return;
    }
    mActive = zone;

    if (zone < 0) {
        lv_obj_add_flag(mMarker, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mArrow, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    Theme::setArc(mMarker, kSegmentStart[zone], kSegmentStart[zone] + kSegmentDeg);
    Theme::setArcColor(mMarker, kZoneColor[zone]);
    lv_obj_remove_flag(mMarker, LV_OBJ_FLAG_HIDDEN);

    // Triangle on the segment's mid angle: apex towards the bar, base inside.
    // TouchGFX angles: 0 = 12 o'clock, clockwise. Radial unit vector is
    // (sin a, -cos a); the tangent is (cos a, sin a).
    const float a  = (kSegmentStart[zone] + kSegmentDeg / 2.0f) * 3.14159265f / 180.0f;
    const float rx = sinf(a), ry = -cosf(a);
    const float tx = cosf(a), ty = sinf(a);
    const float cx = static_cast<float>(kArcCx), cy = static_cast<float>(kArcCy) + 0.4f;
    // lv_value_precise_t is integer unless LV_USE_FLOAT is on; round to the pixel grid.
    auto pt = [](float px, float py) {
        return lv_point_precise_t { static_cast<lv_value_precise_t>(lroundf(px)),
                                    static_cast<lv_value_precise_t>(lroundf(py)) };
    };
    mArrowDsc.p[0] = pt(cx + kArrowTipR * rx, cy + kArrowTipR * ry);
    mArrowDsc.p[1] = pt(cx + kArrowBaseR * rx + kArrowHalfW * tx, cy + kArrowBaseR * ry + kArrowHalfW * ty);
    mArrowDsc.p[2] = pt(cx + kArrowBaseR * rx - kArrowHalfW * tx, cy + kArrowBaseR * ry - kArrowHalfW * ty);
    mArrowDsc.color = Theme::rgb(kZoneColor[zone]);
    lv_obj_remove_flag(mArrow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(mArrow);
}

void HeartRateZone::setHR(float bpm, const uint8_t* thresholds, uint8_t thresholdCount)
{
    if (!thresholds || thresholdCount == 0) {
        return;
    }
    if (thresholdCount > kZoneCount) {
        thresholdCount = kZoneCount;
    }
    // Highest threshold the HR exceeds selects the zone; below all: none.
    // Threshold i is the entry to zone i, so the walk starts at the zone of
    // the highest supplied threshold.
    int active = thresholdCount - 1;
    for (int i = thresholdCount - 1; i >= 0; --i) {
        if (bpm > thresholds[i]) {
            break;
        }
        --active;
    }
    showZone(active);
}

// --- PauseIndicator --------------------------------------------------------

PauseIndicator::PauseIndicator(lv_obj_t* parent, int32_t y)
{
    // The TouchGFX design clips a radius-116 circle centred 82 px above the
    // panel to a 164 x 34 box, giving a shallow dome. Reproduce it the same
    // way: a clipping container with an oversized circle inside.
    lv_obj_t* clip = Theme::container(parent, 38, y, 164, 34);
    lv_obj_t* dome = Theme::dot(clip, 82, -82, 116, Color::GRAY_DARK);
    (void)dome;
    Theme::imageTinted(parent, &img_pause_14x14, 76, y + 10, Color::WHITE);
    mLabel = Theme::label(parent, Theme::Font::Italic18, "", 88, y + 5, 85);
}

void PauseIndicator::setTime(std::time_t seconds)
{
    const unsigned h = static_cast<unsigned>(seconds / 3600);
    const unsigned m = static_cast<unsigned>((seconds % 3600) / 60);
    const unsigned s = static_cast<unsigned>(seconds % 60);
    if (h > 0) {
        lv_label_set_text_fmt(mLabel, "%u:%02u:%02u", h, m, s);
    } else {
        lv_label_set_text_fmt(mLabel, "%02u:%02u", m, s);
    }
}

// --- InfoCarousel ----------------------------------------------------------

InfoCarousel::InfoCarousel(lv_obj_t* parent, int32_t x, int32_t y)
{
    mTitle = Theme::label(parent, Theme::Font::Italic18, "", x, y + 10, 160);
    mValue = Theme::label(parent, Theme::Font::SemiBold30, "", x, y + 29, 160);
    Theme::hline(parent, x, y + 65, 160, Color::GRAY_DARK);
}

InfoCarousel::~InfoCarousel()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
}

void InfoCarousel::setPeriodMs(uint32_t ms)
{
    mPeriodMs = ms ? ms : 1;
    if (mTimer) {
        lv_timer_set_period(mTimer, mPeriodMs);
    }
}

void InfoCarousel::setCallback(UpdateCb cb, void* user)
{
    mCb   = cb;
    mUser = user;
}

void InfoCarousel::setCount(uint16_t count)
{
    mCount = count;
    mIndex = 0;
    if (mCount > 1 && !mTimer) {
        mTimer = lv_timer_create(&InfoCarousel::tickCb, mPeriodMs, this);
    } else if (mCount <= 1 && mTimer) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
    if (mTimer) {
        lv_timer_reset(mTimer);
    }
    fire();
}

void InfoCarousel::refresh()
{
    fire();
}

void InfoCarousel::setTitle(const char* text)
{
    lv_label_set_text(mTitle, text);
}

void InfoCarousel::setValue(const char* text)
{
    if (text) {
        lv_label_set_text(mValue, text);
        lv_obj_remove_flag(mValue, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mValue, LV_OBJ_FLAG_HIDDEN);
    }
}

void InfoCarousel::tickCb(lv_timer_t* t)
{
    auto* self = static_cast<InfoCarousel*>(lv_timer_get_user_data(t));
    if (self->mCount > 0) {
        self->mIndex = static_cast<uint16_t>((self->mIndex + 1) % self->mCount);
    }
    self->fire();
}

void InfoCarousel::fire()
{
    if (mCb) {
        mCb(mUser, static_cast<int16_t>(mIndex));
    }
}

// --- Map -------------------------------------------------------------------

namespace
{
constexpr std::time_t kMaxIntervalSec = 5999;   // 99:59
} // namespace

namespace
{
constexpr uint32_t kPickerActive   = Color::TEAL;
constexpr uint32_t kPickerInactive = Color::WHITE;
} // namespace
} // namespace Widgets
