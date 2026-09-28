/**
 ******************************************************************************
 * @file    RouteMap.cpp
 * @brief   The breadcrumb map widget (see RouteMap.hpp).
 ******************************************************************************
 */

#include "gui/widgets/RouteMap.hpp"

#include <cmath>
#include <cstdio>

#include "SDK/GUI/Color.hpp"
#include "gui/theme/Theme.hpp"

using SDK::GUI::Color::RED;
using SDK::GUI::Color::WHITE;

namespace Widgets
{

namespace
{
constexpr uint32_t kRouteColour = 0xE040FF;   ///< the promo's magenta
constexpr uint32_t kGlowColour  = 0x4C1A66;   ///< the same, dimmed: the glow
constexpr uint32_t kStartColour = SDK::GUI::Color::LIME;
constexpr uint32_t kNorthColour = 0xF02828;
constexpr int32_t  kLineWidth   = 4;
constexpr int32_t  kGlowWidth   = 10;
constexpr int32_t  kMarkerRadius = 5;
constexpr float    kRunnerDrop  = 0.12f;   ///< runner this far below centre (share of height), when turned
constexpr int32_t  kNorthInset  = 14;      ///< "N" this far in from the edge
constexpr float    kPi          = 3.14159265358979f;
constexpr float    kBarMaxPx    = 96.0f;
constexpr int32_t  kBarFromBottom = 32;

struct Step {
    float       metres;
    const char* label;
};
constexpr Step kMetric[] = {
    { 20, "20 m" }, { 50, "50 m" }, { 100, "100 m" }, { 200, "200 m" }, { 500, "500 m" },
    { 1000, "1 km" }, { 2000, "2 km" }, { 5000, "5 km" }, { 10000, "10 km" }, { 20000, "20 km" },
    { 50000, "50 km" },
};
constexpr Step kImperial[] = {
    { 30.48f, "100 ft" }, { 60.96f, "200 ft" }, { 152.4f, "500 ft" }, { 304.8f, "1000 ft" },
    { 804.67f, "0.5 mi" }, { 1609.34f, "1 mi" }, { 3218.69f, "2 mi" }, { 8046.7f, "5 mi" },
    { 16093.4f, "10 mi" }, { 32186.9f, "20 mi" }, { 80467.0f, "50 mi" },
};

/// Shared by every map: only one is drawn at a time.
Trail::MapView::Frame sFrame;

/// The route's look, shared by its lines: one style in LVGL's pool, not three
/// local properties on each line.
lv_style_t sRouteStyle;
lv_style_t sGlowStyle;
} // namespace

void RouteMap::initStyle()
{
    lv_style_init(&sRouteStyle);
    lv_style_set_line_width(&sRouteStyle, kLineWidth);
    lv_style_set_line_rounded(&sRouteStyle, true);
    lv_style_set_line_color(&sRouteStyle, Theme::rgb(kRouteColour));
    lv_style_init(&sGlowStyle);
    lv_style_set_line_width(&sGlowStyle, kGlowWidth);
    lv_style_set_line_rounded(&sGlowStyle, true);
    lv_style_set_line_color(&sGlowStyle, Theme::rgb(kGlowColour));
}

RouteMap::RouteMap(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
    : mW(w)
    , mH(h)
{
    mRoot = Theme::container(parent, x, y, w, h);
    lv_obj_set_style_clip_corner(mRoot, false, LV_PART_MAIN);
    for (lv_obj_t*& line : mGlow) {
        line = lv_line_create(mRoot);
        lv_obj_set_pos(line, 0, 0);
        lv_obj_add_style(line, &sGlowStyle, LV_PART_MAIN);
        lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
    }
    for (lv_obj_t*& line : mLines) {
        line = lv_line_create(mRoot);
        lv_obj_set_pos(line, 0, 0);
        lv_obj_add_style(line, &sRouteStyle, LV_PART_MAIN);
        lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
    }
    // Finish first, so start draws on top: a loop's start and finish coincide.
    mFinish = Theme::dot(mRoot, 0, 0, kMarkerRadius, RED);
    mStart  = Theme::dot(mRoot, 0, 0, kMarkerRadius + 1, kStartColour);
    lv_obj_add_flag(mStart, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mFinish, LV_OBJ_FLAG_HIDDEN);

    mRunner = lv_line_create(mRoot);
    lv_obj_set_pos(mRunner, 0, 0);
    lv_obj_set_style_line_width(mRunner, 3, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(mRunner, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(mRunner, Theme::rgb(WHITE), LV_PART_MAIN);
    lv_obj_add_flag(mRunner, LV_OBJ_FLAG_HIDDEN);

    mView.width  = static_cast<int16_t>(w);
    mView.height = static_cast<int16_t>(h);
}

void RouteMap::showFurniture(bool north, bool scale, bool imperial)
{
    mNorthOn  = north;
    mScaleOn  = scale;
    mImperial = imperial;
    if (north && !mNorth) {
        mNorth = Theme::label(mRoot, Theme::Font::SemiBold20, "N", 0, 0, 24, LV_TEXT_ALIGN_CENTER, kNorthColour);
        lv_obj_add_flag(mNorth, LV_OBJ_FLAG_HIDDEN);
    }
    if (scale && !mScaleLine) {
        mScaleLine = lv_line_create(mRoot);
        lv_obj_set_pos(mScaleLine, 0, 0);
        lv_obj_set_style_line_width(mScaleLine, 2, LV_PART_MAIN);
        lv_obj_set_style_line_color(mScaleLine, Theme::rgb(WHITE), LV_PART_MAIN);
        lv_obj_add_flag(mScaleLine, LV_OBJ_FLAG_HIDDEN);
        mScaleLabel = Theme::label(mRoot, Theme::Font::Regular16, "", static_cast<int32_t>(mW / 2) - 50,
                                   mH - kBarFromBottom - 26, 100, LV_TEXT_ALIGN_CENTER, WHITE);
        lv_obj_add_flag(mScaleLabel, LV_OBJ_FLAG_HIDDEN);
    }
}

void RouteMap::setRoute(const Trail::GeoPoint* points, uint16_t count)
{
    mRoute = points;
    mCount = points ? count : 0;
}

void RouteMap::follow(const Trail::GeoPoint& runner, uint16_t radiusM, bool headingValid, float headingDeg)
{
    const int16_t radiusPx = static_cast<int16_t>((mW < mH ? mW : mH) / 2);
    mView.centre           = runner;
    mView.metresPerPx      = static_cast<float>(radiusM) / static_cast<float>(radiusPx);
    mView.rotationDeg      = headingValid ? headingDeg : 0.0f;
    mView.cx               = static_cast<int16_t>(mW / 2);
    mView.cy               = static_cast<int16_t>(mH / 2 + (headingValid ? kRunnerDrop * static_cast<float>(mH) : 0.0f));
    draw();
    drawRunner(true, mView.cx, mView.cy, 0.0f);   // the map is turned, not the arrow
    drawNorth(headingValid ? -headingDeg : 0.0f);
    drawScale();
}

void RouteMap::fitWhole(const Trail::GeoPoint* runner, int16_t marginPx)
{
    mView = Trail::MapView::fit(mRoute, mCount, static_cast<int16_t>(mW), static_cast<int16_t>(mH), marginPx);
    draw();
    Trail::ScreenPoint s;
    if (runner != nullptr && Trail::MapView::toScreen(*runner, mView, s)) {
        drawRunner(true, s.x, s.y, 0.0f);
    } else {
        drawRunner(false, 0, 0, 0.0f);
    }
    drawNorth(0.0f);
    drawScale();
}

void RouteMap::draw()
{
    Trail::MapView::project(mRoute, mCount, mView, sFrame);
    for (uint16_t i = 0; i < sFrame.count; ++i) {
        mPoints[i].x = sFrame.points[i].x;
        mPoints[i].y = sFrame.points[i].y;
    }
    for (uint8_t r = 0; r < Trail::MapView::kMaxRuns; ++r) {
        if (r < sFrame.runs) {
            lv_line_set_points(mGlow[r], &mPoints[sFrame.runStart[r]], sFrame.runLength[r]);
            lv_line_set_points(mLines[r], &mPoints[sFrame.runStart[r]], sFrame.runLength[r]);
            lv_obj_remove_flag(mGlow[r], LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(mLines[r], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(mGlow[r], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(mLines[r], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (mCount >= 2) {
        place(mFinish, mRoute[mCount - 1], kMarkerRadius);
        place(mStart, mRoute[0], kMarkerRadius + 1);
    } else {
        lv_obj_add_flag(mStart, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mFinish, LV_OBJ_FLAG_HIDDEN);
    }
}

void RouteMap::place(lv_obj_t* marker, const Trail::GeoPoint& p, int32_t radius)
{
    Trail::ScreenPoint s;
    if (Trail::MapView::toScreen(p, mView, s)) {
        lv_obj_set_pos(marker, s.x - radius, s.y - radius);
        lv_obj_remove_flag(marker, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(marker, LV_OBJ_FLAG_HIDDEN);
    }
}

void RouteMap::drawRunner(bool show, int32_t x, int32_t y, float turnDeg)
{
    if (!show) {
        lv_obj_add_flag(mRunner, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    // A chevron pointing up, turned by turnDeg, around the runner's pixel.
    static const float kShape[5][2] = { { 0, -10 }, { 7, 8 }, { 0, 3 }, { -7, 8 }, { 0, -10 } };
    const float r = turnDeg * kPi / 180.0f;
    const float c = std::cos(r);
    const float s = std::sin(r);
    for (int i = 0; i < 5; ++i) {
        const float px = kShape[i][0] * c - kShape[i][1] * s;
        const float py = kShape[i][0] * s + kShape[i][1] * c;
        mArrow[i].x    = static_cast<lv_value_precise_t>(std::lround(static_cast<float>(x) + px));
        mArrow[i].y    = static_cast<lv_value_precise_t>(std::lround(static_cast<float>(y) + py));
    }
    lv_line_set_points(mRunner, mArrow, 5);
    lv_obj_remove_flag(mRunner, LV_OBJ_FLAG_HIDDEN);
}

void RouteMap::drawNorth(float turnDeg)
{
    if (!mNorthOn || !mNorth) {
        return;
    }
    // On a circle just inside the edge, at north's bearing on the turned map:
    // straight up when the map is north-up.
    const float   r      = turnDeg * kPi / 180.0f;
    const float   radius = static_cast<float>((mW < mH ? mW : mH) / 2 - kNorthInset - 12);
    const int32_t x      = static_cast<int32_t>(std::lround(mW / 2 + radius * std::sin(r))) - 12;
    const int32_t y      = static_cast<int32_t>(std::lround(mH / 2 - radius * std::cos(r))) - 12;
    lv_obj_set_pos(mNorth, x, y);
    lv_obj_remove_flag(mNorth, LV_OBJ_FLAG_HIDDEN);
}

void RouteMap::drawScale()
{
    if (!mScaleOn || !mScaleLine) {
        return;
    }
    // The largest round distance that fits kBarMaxPx.
    const Step* steps = mImperial ? kImperial : kMetric;
    const int   n     = mImperial ? static_cast<int>(sizeof(kImperial) / sizeof(kImperial[0]))
                                  : static_cast<int>(sizeof(kMetric) / sizeof(kMetric[0]));
    int pick = 0;
    for (int i = 0; i < n; ++i) {
        if (steps[i].metres / mView.metresPerPx <= kBarMaxPx) {
            pick = i;
        }
    }
    const int32_t px = static_cast<int32_t>(std::lround(steps[pick].metres / mView.metresPerPx));
    const int32_t x0 = mW / 2 - px / 2;
    const int32_t x1 = x0 + px;
    const int32_t y  = mH - kBarFromBottom;
    mBar[0] = { x0, y - 6 };
    mBar[1] = { x0, y };
    mBar[2] = { x1, y };
    mBar[3] = { x1, y - 6 };
    lv_line_set_points(mScaleLine, mBar, 4);
    lv_obj_remove_flag(mScaleLine, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(mScaleLabel, steps[pick].label);
    lv_obj_remove_flag(mScaleLabel, LV_OBJ_FLAG_HIDDEN);
}

} // namespace Widgets
