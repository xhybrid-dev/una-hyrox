/**
 ******************************************************************************
 * @file    RouteMap.hpp
 * @brief   The breadcrumb map: the route as lines, the runner, start and finish.
 *
 * Draws what Trail::MapView (the route core, host-tested) works out: the route
 * clipped to this widget in up to MapView::kMaxRuns lines, one lv_line each.
 * Two uses:
 *
 *   - follow(): centred on the runner at a zoom, turned so the direction of
 *     travel is up (or north, when there is no direction yet). The runner is
 *     a white arrow, drawn a little below centre so more of the way ahead
 *     shows; a small "N" at the edge says where north is.
 *   - fitWhole(): the whole route, north-up, for the route preview.
 *
 * Start is a lime dot, finish a red one, the route amber: the colours of
 * RunLVGL's own summary map. The widget keeps a pointer to the route
 * (Model's copy) and its own fixed point buffer; nothing is allocated per
 * frame. One Frame (2 KB) is shared by every map, as only one is drawn at once.
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

    /// Set up the route lines' shared style; once per GUI start, after Theme::init().
    static void initStyle();

    RouteMap(const RouteMap&)            = delete;
    RouteMap& operator=(const RouteMap&) = delete;

    /// The route to draw; @p points must outlive the widget (Model's copy).
    void setRoute(const Trail::GeoPoint* points, uint16_t count);

    /// Centred on @p runner, @p radiusM from the runner to the edge, turned to
    /// @p headingDeg when @p headingValid (north-up otherwise).
    void follow(const Trail::GeoPoint& runner, uint16_t radiusM, bool headingValid, float headingDeg);

    /// The whole route, north-up, no runner.
    void fitWhole();

    lv_obj_t* root() const { return mRoot; }

private:
    void draw();
    void place(lv_obj_t* marker, const Trail::GeoPoint& p, int32_t radius);
    void drawRunner(bool show, float turnDeg);
    void drawNorth(bool show, float turnDeg);

    lv_obj_t* mRoot = nullptr;
    lv_obj_t* mLines[Trail::MapView::kMaxRuns] {};
    lv_obj_t* mStart  = nullptr;
    lv_obj_t* mFinish = nullptr;
    lv_obj_t* mRunner = nullptr;
    lv_obj_t* mNorth  = nullptr;

    lv_point_precise_t mPoints[Trail::MapView::kMaxPoints] {};
    lv_point_precise_t mArrow[5] {};

    const Trail::GeoPoint* mRoute = nullptr;
    uint16_t               mCount = 0;
    Trail::MapView::View   mView {};
    int32_t                mW = 0;
    int32_t                mH = 0;
};

} // namespace Widgets

#endif // ROUTE_MAP_HPP
