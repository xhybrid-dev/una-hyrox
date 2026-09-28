/**
 ******************************************************************************
 * @file    RouteMap.cpp
 * @brief   The breadcrumb map widget (see RouteMap.hpp).
 ******************************************************************************
 */

#include "gui/widgets/RouteMap.hpp"

#include <cmath>

#include "SDK/GUI/Color.hpp"
#include "gui/theme/Theme.hpp"

using SDK::GUI::Color::RED;
using SDK::GUI::Color::WHITE;

namespace Widgets
{

namespace
{
constexpr uint32_t kRouteColour  = SDK::GUI::Color::YELLOW_DARK;   ///< RunLVGL's map amber
constexpr uint32_t kStartColour  = SDK::GUI::Color::LIME;
constexpr int32_t  kLineWidth    = 4;
constexpr int32_t  kMarkerRadius = 5;
constexpr float    kRunnerDrop   = 0.12f;   ///< runner this far below centre (share of height)
constexpr int32_t  kNorthInset   = 42;      ///< "N" this far in from the edge
constexpr float    kPi           = 3.14159265358979f;

/// Shared by every map: only one is drawn at a time.
Trail::MapView::Frame sFrame;

/// The route's look, shared by its lines: one style in LVGL's pool, not three
/// local properties on each of kMaxRuns lines.
lv_style_t sRouteStyle;
} // namespace

void RouteMap::initStyle()
{
    lv_style_init(&sRouteStyle);
    lv_style_set_line_width(&sRouteStyle, kLineWidth);
    lv_style_set_line_rounded(&sRouteStyle, true);
    lv_style_set_line_color(&sRouteStyle, Theme::rgb(kRouteColour));
}

RouteMap::RouteMap(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
    : mW(w)
    , mH(h)
{
    mRoot = Theme::container(parent, x, y, w, h);
    lv_obj_set_style_clip_corner(mRoot, false, LV_PART_MAIN);
    for (lv_obj_t*& line : mLines) {
        line = lv_line_create(mRoot);
        lv_obj_set_pos(line, 0, 0);
        lv_obj_add_style(line, &sRouteStyle, LV_PART_MAIN);
        lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
    }
    // Finish first, so start draws on top: a loop's start and finish coincide.
    mFinish = Theme::dot(mRoot, 0, 0, kMarkerRadius, RED);
    mStart  = Theme::dot(mRoot, 0, 0, kMarkerRadius, kStartColour);
    lv_obj_add_flag(mStart, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mFinish, LV_OBJ_FLAG_HIDDEN);

    mRunner = lv_line_create(mRoot);
    lv_obj_set_pos(mRunner, 0, 0);
    lv_obj_set_style_line_width(mRunner, 3, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(mRunner, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(mRunner, Theme::rgb(WHITE), LV_PART_MAIN);
    lv_obj_add_flag(mRunner, LV_OBJ_FLAG_HIDDEN);

    mNorth = Theme::label(mRoot, Theme::Font::SemiBold20, "N", 0, 0, 24);
    lv_obj_add_flag(mNorth, LV_OBJ_FLAG_HIDDEN);

    mView.width  = static_cast<int16_t>(w);
    mView.height = static_cast<int16_t>(h);
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
    // Heading-up: the arrow points up; north is somewhere else, so mark it.
    // North-up (no direction yet): a plain arrow, north needs no marker.
    drawRunner(true, 0.0f);
    drawNorth(headingValid, headingValid ? -headingDeg : 0.0f);
}

void RouteMap::fitWhole()
{
    const int16_t margin = 8;
    mView = Trail::MapView::fit(mRoute, mCount, static_cast<int16_t>(mW), static_cast<int16_t>(mH), margin);
    draw();
    drawRunner(false, 0.0f);
    drawNorth(false, 0.0f);
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
            lv_line_set_points(mLines[r], &mPoints[sFrame.runStart[r]], sFrame.runLength[r]);
            lv_obj_remove_flag(mLines[r], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(mLines[r], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (mCount >= 2) {
        place(mFinish, mRoute[mCount - 1], kMarkerRadius);
        place(mStart, mRoute[0], kMarkerRadius);
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

void RouteMap::drawRunner(bool show, float turnDeg)
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
        const float x = kShape[i][0] * c - kShape[i][1] * s;
        const float y = kShape[i][0] * s + kShape[i][1] * c;
        mArrow[i].x   = static_cast<lv_value_precise_t>(std::lround(mView.cx + x));
        mArrow[i].y   = static_cast<lv_value_precise_t>(std::lround(mView.cy + y));
    }
    lv_line_set_points(mRunner, mArrow, 5);
    lv_obj_remove_flag(mRunner, LV_OBJ_FLAG_HIDDEN);
}

void RouteMap::drawNorth(bool show, float turnDeg)
{
    if (!show) {
        lv_obj_add_flag(mNorth, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    // On a circle inside the edge (clear of the scroll indicator and the
    // button hints), at north's bearing on the turned map.
    const float   r      = turnDeg * kPi / 180.0f;
    const float   radius = static_cast<float>((mW < mH ? mW : mH) / 2 - kNorthInset);
    const int32_t x      = static_cast<int32_t>(std::lround(mW / 2 + radius * std::sin(r))) - 12;
    const int32_t y      = static_cast<int32_t>(std::lround(mH / 2 - radius * std::cos(r))) - 12;
    lv_obj_set_pos(mNorth, x, y);
    lv_obj_remove_flag(mNorth, LV_OBJ_FLAG_HIDDEN);
}

} // namespace Widgets
