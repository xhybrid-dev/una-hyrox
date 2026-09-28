/**
 ******************************************************************************
 * @file    MapView.hpp
 * @brief   The route, as screen pixels around the runner.
 *
 * The breadcrumb screen draws the route as a line, centred on the runner, at
 * a zoom level, turned so that either north (north-up) or the direction of
 * travel (heading-up) is at the top. This works out the pixels:
 *
 *   1. each point becomes metres east and north of the centre (flat-earth,
 *      fine across a screen's worth of ground);
 *   2. turned by the rotation, and scaled by metres per pixel;
 *   3. each segment is clipped to the screen (Liang-Barsky), so a route that
 *      leaves the screen and comes back becomes separate runs of points,
 *      one line each, rather than a line cutting across the screen;
 *   4. points closer than kMinStepPx to the last one kept are dropped: at
 *      2.5 km to the screen edge, a 2,000-point route would otherwise hand the
 *      display thousands of points it cannot tell apart.
 *
 * Output is a fixed Frame: at most kMaxPoints points in at most kMaxRuns runs.
 * A route too busy for that is cut short and says so (truncated), never
 * overflows. Pixel coordinates are only formed after clipping, so a point
 * 50 km away never has to fit in an int16.
 *
 * Pure arithmetic: the GUI owns a View and a Frame, and calls project() when
 * the runner moves or the zoom changes.
 ******************************************************************************
 */

#ifndef TRAIL_MAP_VIEW_HPP
#define TRAIL_MAP_VIEW_HPP

#include <cstdint>

#include "GeoPoint.hpp"

namespace Trail
{

struct ScreenPoint {
    int16_t x = 0;
    int16_t y = 0;
};

class MapView
{
public:
    static constexpr uint16_t kMaxPoints = 512;
    static constexpr uint8_t  kMaxRuns   = 16;
    static constexpr float    kMinStepPx = 2.0f;

    /// Zoom levels: metres from the runner to the edge of the screen's circle.
    static constexpr uint16_t kZoomRadiiM[] = { 100, 250, 500, 1000, 2500 };
    static constexpr uint8_t  kZoomLevels   = 5;

    struct View {
        GeoPoint centre {};               ///< the ground point drawn at (cx, cy)
        float    metresPerPx = 5.0f;
        float    rotationDeg = 0.0f;      ///< the bearing at the top of the screen: 0 north-up, else the heading
        int16_t  width       = 240;
        int16_t  height      = 240;
        int16_t  cx          = 120;
        int16_t  cy          = 120;
    };

    struct Frame {
        ScreenPoint points[kMaxPoints];
        uint16_t    count = 0;
        uint16_t    runStart[kMaxRuns] {};
        uint16_t    runLength[kMaxRuns] {};
        uint8_t     runs      = 0;
        bool        truncated = false;
    };

    /// The route @p points[0..count) as clipped screen runs.
    static void project(const GeoPoint* points, uint16_t count, const View& view, Frame& out);

    /// One point (the runner, the start, the finish). True if it is on screen;
    /// @p out is filled either way, clamped to the int16 range.
    static bool toScreen(const GeoPoint& p, const View& view, ScreenPoint& out);

    /// A north-up view that fits the whole route inside the screen's circle,
    /// @p marginPx inside its edge.
    static View fit(const GeoPoint* points, uint16_t count, int16_t width, int16_t height, int16_t marginPx);

    /// Metres per pixel for zoom @p level (clamped), with @p radiusPx from the
    /// centre to the edge of the circle.
    static float zoomScale(uint8_t level, int16_t radiusPx);

private:
    static void toView(const GeoPoint& p, const View& view, float cosR, float sinR, float& x, float& y);
};

} // namespace Trail

#endif // TRAIL_MAP_VIEW_HPP
