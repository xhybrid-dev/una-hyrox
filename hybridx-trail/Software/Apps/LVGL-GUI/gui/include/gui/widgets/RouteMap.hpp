/**
 ******************************************************************************
 * @file    RouteMap.hpp
 * @brief   The breadcrumb map: the route as a glowing line, the runner, start
 *          and finish, a north marker and a scale bar.
 *
 * Draws what Trail::MapView (the route core, host-tested) works out: the route
 * clipped to this widget in up to MapView::kMaxRuns lines, one lv_line each,
 * each with a wider, dimmer line beneath it for the glow. Two uses:
 *
 *   - follow(): centred on the runner at a zoom, turned so the direction of
 *     travel is up (or north, when there is no direction yet or the map is
 *     set north-up). The runner is a white arrow, drawn a little below
 *     centre when the map is turned, so more of the way ahead shows.
 *   - fitWhole(): the whole route, north-up. With a runner position, the
 *     arrow sits where the runner is on it; without (the route preview), none.
 *
 * Route magenta, start green, finish red, north marker red: the colours of
 * the HybridX Trail promo. With showScale(), a scale bar names a round
 * distance (200 m, 1 km; or ft and miles) about a third of the way across.
 * The widget keeps a pointer to the route (Model's copy) and its own fixed
 * point buffer; nothing is allocated per frame. One Frame (2 KB) is shared
 * by every map, as only one is drawn at once.
 ******************************************************************************
 */

#ifndef ROUTE_MAP_HPP
#define ROUTE_MAP_HPP

#include <cstdint>

#include "lvgl.h"

#include "GeoPoint.hpp"
#include "MapView.hpp"

namespace Widgets
{

class RouteMap
{
public:
    RouteMap(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h);

    RouteMap(const RouteMap&)            = delete;
    RouteMap& operator=(const RouteMap&) = delete;

    /// Set up the route lines' shared styles; once per GUI start, after Theme::init().
    static void initStyle();

    /// The route to draw; @p points must outlive the widget (Model's copy).
    void setRoute(const Trail::GeoPoint* points, uint16_t count);

    /// Show the north marker and the scale bar (the run map; not the preview).
    void showFurniture(bool north, bool scale, bool imperial);

    /// Centred on @p runner, @p radiusM from the runner to the edge, turned to
    /// @p headingDeg when @p headingValid (north-up otherwise).
    void follow(const Trail::GeoPoint& runner, uint16_t radiusM, bool headingValid, float headingDeg);

    /// The whole route, north-up, inside @p marginPx of the edge; the runner
    /// at @p runner, if given.
    void fitWhole(const Trail::GeoPoint* runner = nullptr, int16_t marginPx = 8);

    lv_obj_t* root() const { return mRoot; }

private:
    void draw();
    void place(lv_obj_t* marker, const Trail::GeoPoint& p, int32_t radius);
    void drawRunner(bool show, int32_t x, int32_t y, float turnDeg);
    void drawNorth(float turnDeg);
    void drawScale();

    lv_obj_t* mRoot = nullptr;
    lv_obj_t* mGlow[Trail::MapView::kMaxRuns] {};
    lv_obj_t* mLines[Trail::MapView::kMaxRuns] {};
    lv_obj_t* mStart  = nullptr;
    lv_obj_t* mFinish = nullptr;
    lv_obj_t* mRunner = nullptr;
    lv_obj_t* mNorth  = nullptr;
    lv_obj_t* mScaleLine  = nullptr;
    lv_obj_t* mScaleLabel = nullptr;

    lv_point_precise_t mPoints[Trail::MapView::kMaxPoints] {};
    lv_point_precise_t mArrow[5] {};
    lv_point_precise_t mBar[4] {};

    const Trail::GeoPoint* mRoute = nullptr;
    uint16_t               mCount = 0;
    Trail::MapView::View   mView {};
    int32_t                mW = 0;
    int32_t                mH = 0;
    bool                   mNorthOn  = false;
    bool                   mScaleOn  = false;
    bool                   mImperial = false;
};

} // namespace Widgets

#endif // ROUTE_MAP_HPP
