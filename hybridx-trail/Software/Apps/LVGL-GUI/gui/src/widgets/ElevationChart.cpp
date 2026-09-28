/**
 ******************************************************************************
 * @file    ElevationChart.cpp
 * @brief   The elevation chart widget (see ElevationChart.hpp).
 ******************************************************************************
 */

#include "gui/widgets/ElevationChart.hpp"

#include <cmath>
#include <initializer_list>

#include "SDK/GUI/Color.hpp"
#include "gui/theme/Theme.hpp"

namespace Widgets
{

namespace
{
constexpr uint32_t kDoneColour = 0xE040FF;   ///< the route's magenta
constexpr uint32_t kRestColour = 0x8A2AA0;   ///< dimmer: still to come
constexpr int32_t  kPadPx      = 6;
constexpr int32_t  kDotRadius  = 5;
} // namespace

ElevationChart::ElevationChart(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
    : mW(w)
    , mH(h)
{
    mRoot = Theme::container(parent, x, y, w, h);
    lv_obj_set_style_clip_corner(mRoot, false, LV_PART_MAIN);
    mRest = lv_line_create(mRoot);
    mDone = lv_line_create(mRoot);
    for (lv_obj_t* l : { mRest, mDone }) {
        lv_obj_set_pos(l, 0, 0);
        lv_obj_set_style_line_rounded(l, true, LV_PART_MAIN);
        lv_obj_add_flag(l, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_line_width(mRest, 3, LV_PART_MAIN);
    lv_obj_set_style_line_color(mRest, Theme::rgb(kRestColour), LV_PART_MAIN);
    lv_obj_set_style_line_width(mDone, 4, LV_PART_MAIN);
    lv_obj_set_style_line_color(mDone, Theme::rgb(kDoneColour), LV_PART_MAIN);
    mMarker = Theme::dot(mRoot, 0, 0, kDotRadius, SDK::GUI::Color::WHITE);
    lv_obj_add_flag(mMarker, LV_OBJ_FLAG_HIDDEN);
}

void ElevationChart::set(const Trail::ElevationProfile& profile, float alongM, bool showRunner)
{
    using Profile = Trail::ElevationProfile;
    if (!profile.valid()) {
        lv_obj_add_flag(mRest, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mDone, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mMarker, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    float lo = profile.minM();
    float hi = profile.maxM();
    if (hi - lo < kMinRangeM) {
        const float mid = (hi + lo) * 0.5f;
        lo              = mid - kMinRangeM * 0.5f;
        hi              = mid + kMinRangeM * 0.5f;
    }
    const float usable = static_cast<float>(mH - 2 * kPadPx);
    auto        yOf    = [&](float e) {
        return static_cast<lv_value_precise_t>(std::lround(static_cast<float>(mH - kPadPx) - (e - lo) / (hi - lo) * usable));
    };
    auto xOf = [&](float bin) {
        return static_cast<lv_value_precise_t>(std::lround(bin * static_cast<float>(mW - 1) / static_cast<float>(Profile::kBins - 1)));
    };

    // The runner's place: a fractional bin.
    float f = showRunner ? alongM / profile.lengthM() * static_cast<float>(Profile::kBins - 1) : 0.0f;
    f       = f < 0.0f ? 0.0f : (f > static_cast<float>(Profile::kBins - 1) ? static_cast<float>(Profile::kBins - 1) : f);
    int i   = static_cast<int>(f);
    if (i > Profile::kBins - 2) {
        i = Profile::kBins - 2;
    }
    const lv_point_precise_t here = { xOf(f), yOf(profile.elevationM(showRunner ? alongM : 0.0f)) };

    uint16_t nd = 0;
    uint16_t nr = 0;
    if (showRunner) {
        for (int b = 0; b <= i; ++b) {
            mDonePts[nd++] = { xOf(static_cast<float>(b)), yOf(profile.binM(static_cast<uint8_t>(b))) };
        }
        mDonePts[nd++] = here;
        mRestPts[nr++] = here;
        for (int b = i + 1; b < Profile::kBins; ++b) {
            mRestPts[nr++] = { xOf(static_cast<float>(b)), yOf(profile.binM(static_cast<uint8_t>(b))) };
        }
    } else {
        for (int b = 0; b < Profile::kBins; ++b) {
            mDonePts[nd++] = { xOf(static_cast<float>(b)), yOf(profile.binM(static_cast<uint8_t>(b))) };
        }
    }
    lv_line_set_points(mDone, mDonePts, nd);
    lv_obj_remove_flag(mDone, LV_OBJ_FLAG_HIDDEN);
    if (nr >= 2) {
        lv_line_set_points(mRest, mRestPts, nr);
        lv_obj_remove_flag(mRest, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mRest, LV_OBJ_FLAG_HIDDEN);
    }
    if (showRunner) {
        lv_obj_set_pos(mMarker, here.x - kDotRadius, here.y - kDotRadius);
        lv_obj_remove_flag(mMarker, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mMarker, LV_OBJ_FLAG_HIDDEN);
    }
}

} // namespace Widgets
