/**
 ******************************************************************************
 * @file    MapView.cpp
 * @brief   Route-to-screen projection and clipping (see the header).
 ******************************************************************************
 */

#include "MapView.hpp"

#include <cmath>

namespace Trail
{

namespace
{
constexpr float kPi = 3.14159265358979f;

/// Liang-Barsky: clip p0-p1 to [0, w] x [0, h]. False if nothing is left.
/// On return, @p enter/@p leave say whether each end was moved.
bool clip(float& x0, float& y0, float& x1, float& y1, float w, float h, bool& enter, bool& leave)
{
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    float       t0 = 0.0f;
    float       t1 = 1.0f;
    const float p[4] = { -dx, dx, -dy, dy };
    const float q[4] = { x0, w - x0, y0, h - y0 };
    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0.0f) {
            if (q[i] < 0.0f) {
                return false;   // parallel to this edge and outside it
            }
            continue;
        }
        const float r = q[i] / p[i];
        if (p[i] < 0.0f) {
            if (r > t1) {
                return false;
            }
            if (r > t0) {
                t0 = r;
            }
        } else {
            if (r < t0) {
                return false;
            }
            if (r < t1) {
                t1 = r;
            }
        }
    }
    enter = t0 > 0.0f;
    leave = t1 < 1.0f;
    const float nx0 = x0 + t0 * dx;
    const float ny0 = y0 + t0 * dy;
    x1              = x0 + t1 * dx;
    y1              = y0 + t1 * dy;
    x0              = nx0;
    y0              = ny0;
    return true;
}

int16_t toPixel(float v)
{
    if (v > 32767.0f) {
        return 32767;
    }
    if (v < -32768.0f) {
        return -32768;
    }
    return static_cast<int16_t>(std::lround(v));
}
} // namespace

void MapView::toView(const GeoPoint& p, const View& view, float cosR, float sinR, float& x, float& y)
{
    float e = 0.0f;
    float n = 0.0f;
    Geo::offsetM(view.centre, p, e, n);
    // Turn so that the bearing rotationDeg points up the screen.
    const float er = e * cosR - n * sinR;
    const float nr = e * sinR + n * cosR;
    x              = static_cast<float>(view.cx) + er / view.metresPerPx;
    y              = static_cast<float>(view.cy) - nr / view.metresPerPx;
}

bool MapView::toScreen(const GeoPoint& p, const View& view, ScreenPoint& out)
{
    const float r = view.rotationDeg * kPi / 180.0f;
    float       x = 0.0f;
    float       y = 0.0f;
    toView(p, view, std::cos(r), std::sin(r), x, y);
    out.x = toPixel(x);
    out.y = toPixel(y);
    return x >= 0.0f && y >= 0.0f && x <= static_cast<float>(view.width) && y <= static_cast<float>(view.height);
}

void MapView::project(const GeoPoint* points, uint16_t count, const View& view, Frame& out)
{
    out.count     = 0;
    out.runs      = 0;
    out.truncated = false;
    if (points == nullptr || count < 2 || view.metresPerPx <= 0.0f) {
        return;
    }
    const float r    = view.rotationDeg * kPi / 180.0f;
    const float cosR = std::cos(r);
    const float sinR = std::sin(r);
    const float w    = static_cast<float>(view.width);
    const float h    = static_cast<float>(view.height);

    float px = 0.0f;
    float py = 0.0f;
    toView(points[0], view, cosR, sinR, px, py);
    bool  inRun = false;
    float lastX = 0.0f;   // the last point kept, unrounded
    float lastY = 0.0f;

    for (uint16_t i = 1; i < count; ++i) {
        float cx = 0.0f;
        float cy = 0.0f;
        toView(points[i], view, cosR, sinR, cx, cy);

        float x0 = px, y0 = py, x1 = cx, y1 = cy;
        bool  enter = false;
        bool  leave = false;
        px          = cx;
        py          = cy;
        if (!clip(x0, y0, x1, y1, w, h, enter, leave)) {
            inRun = false;
            continue;
        }

        if (!inRun || enter) {
            if (out.runs == kMaxRuns || out.count >= kMaxPoints - 1) {
                out.truncated = true;
                break;
            }
            out.runStart[out.runs]  = out.count;
            out.runLength[out.runs] = 0;
            ++out.runs;
            out.points[out.count++] = ScreenPoint { toPixel(x0), toPixel(y0) };
            ++out.runLength[out.runs - 1];
            lastX = x0;
            lastY = y0;
            inRun = true;
        }

        const bool last = leave || i + 1 == count;
        const float dx  = x1 - lastX;
        const float dy  = y1 - lastY;
        if (last || dx * dx + dy * dy >= kMinStepPx * kMinStepPx) {
            if (out.count >= kMaxPoints) {
                out.truncated = true;
                break;
            }
            out.points[out.count++] = ScreenPoint { toPixel(x1), toPixel(y1) };
            ++out.runLength[out.runs - 1];
            lastX = x1;
            lastY = y1;
        }
        if (leave) {
            inRun = false;
        }
    }

    // A run of one point is nothing to draw: drop it (only the last can be).
    if (out.runs > 0 && out.runLength[out.runs - 1] < 2) {
        out.count = out.runStart[out.runs - 1];
        --out.runs;
    }
}

MapView::View MapView::fit(const GeoPoint* points, uint16_t count, int16_t width, int16_t height, int16_t marginPx)
{
    View v;
    v.width  = width;
    v.height = height;
    v.cx     = static_cast<int16_t>(width / 2);
    v.cy     = static_cast<int16_t>(height / 2);
    if (points == nullptr || count == 0) {
        return v;
    }
    int32_t minLat = points[0].latE7, maxLat = points[0].latE7;
    int32_t minLon = points[0].lonE7, maxLon = points[0].lonE7;
    for (uint16_t i = 1; i < count; ++i) {
        minLat = points[i].latE7 < minLat ? points[i].latE7 : minLat;
        maxLat = points[i].latE7 > maxLat ? points[i].latE7 : maxLat;
        minLon = points[i].lonE7 < minLon ? points[i].lonE7 : minLon;
        maxLon = points[i].lonE7 > maxLon ? points[i].lonE7 : maxLon;
    }
    v.centre.latE7 = static_cast<int32_t>((static_cast<int64_t>(minLat) + maxLat) / 2);
    v.centre.lonE7 = static_cast<int32_t>((static_cast<int64_t>(minLon) + maxLon) / 2);

    float maxR2 = 0.0f;
    for (uint16_t i = 0; i < count; ++i) {
        float e = 0.0f;
        float n = 0.0f;
        Geo::offsetM(v.centre, points[i], e, n);
        maxR2 = e * e + n * n > maxR2 ? e * e + n * n : maxR2;
    }
    const int16_t radiusPx = static_cast<int16_t>((width < height ? width : height) / 2 - marginPx);
    const float   maxR     = std::sqrt(maxR2);
    v.metresPerPx          = (radiusPx > 0 && maxR > 0.0f) ? maxR / static_cast<float>(radiusPx) : 1.0f;
    return v;
}

float MapView::zoomScale(uint8_t level, int16_t radiusPx)
{
    const uint8_t l = level < kZoomLevels ? level : static_cast<uint8_t>(kZoomLevels - 1);
    return radiusPx > 0 ? static_cast<float>(kZoomRadiiM[l]) / static_cast<float>(radiusPx) : 1.0f;
}

} // namespace Trail
