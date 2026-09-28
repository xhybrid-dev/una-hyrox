/**
 ******************************************************************************
 * @file    ElevationChart.hpp
 * @brief   The route's elevation profile as a line, with the runner on it.
 *
 * Draws Trail::ElevationProfile (96 bins) as two lv_lines that meet at the
 * runner: the part run in bright magenta, the part to come dimmer, and a
 * white dot where the runner is. Vertical scale fits the route's own range
 * (never tighter than kMinRangeM, so a flat route does not look like a
 * mountain). Fixed point buffers; nothing is allocated per update.
 ******************************************************************************
 */

#ifndef ELEVATION_CHART_HPP
#define ELEVATION_CHART_HPP

#include <cstdint>

#include "lvgl.h"

#include "ElevationProfile.hpp"

namespace Widgets
{

class ElevationChart
{
public:
    static constexpr float kMinRangeM = 40.0f;

    ElevationChart(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h);

    ElevationChart(const ElevationChart&)            = delete;
    ElevationChart& operator=(const ElevationChart&) = delete;

    /// Draw @p profile with the runner @p alongM along the route; without a
    /// runner (@p showRunner false) the whole line is the bright one.
    void set(const Trail::ElevationProfile& profile, float alongM, bool showRunner);

private:
    lv_obj_t* mRoot   = nullptr;
    lv_obj_t* mRest   = nullptr;
    lv_obj_t* mDone   = nullptr;
    lv_obj_t* mMarker = nullptr;

    lv_point_precise_t mDonePts[Trail::ElevationProfile::kBins + 1] {};
    lv_point_precise_t mRestPts[Trail::ElevationProfile::kBins + 1] {};
    int32_t            mW = 0;
    int32_t            mH = 0;
};

} // namespace Widgets

#endif // ELEVATION_CHART_HPP
