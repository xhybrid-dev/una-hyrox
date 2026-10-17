/**
 ******************************************************************************
 * @file    GlanceLayout.hpp
 * @brief   What the glance draws, as control specs: pure, host-tested
 *          (GlanceLayoutTest). The shape of HybridX Streak's GlanceLayout.
 *
 * The area and the control budget come from the watch at run time
 * (RequestGlanceConfig); Jon's watch gives 240x60 and 32 controls (HybridX
 * Streak NOTES, Gate 0). The glances list draws its separators only across
 * the middle 172 px, and a scroll arc over both edges, so everything keeps to
 * that middle band (NOTES, Gate W3). The layout adapts:
 *   full     on the left the trend, "81.9 kg", and under it the weekly change,
 *            "-0.4 kg/wk"; on the right the last 14 days as bars, one per
 *            weigh-in, the newest in white, on a baseline (17 controls);
 *   compact  the two lines alone, centred (too few controls, or narrow);
 *   tiny     "81.9 kg" alone (a very short area).
 * With no weigh-ins yet, or no app: "X Weigh-in" and "No weigh-ins yet".
 *
 * Only text and filled rectangles: line controls do not draw on the watch
 * (Streak NOTES S4.3). Words are plain ASCII in 18-25 point faces (the 10
 * point face holds digits alone, S4.2). Bars are grey and white, without
 * judgement: the app is a tracker. GlanceLayoutTest checks that every control
 * lies inside the area, the budget is kept, and every text fits its box in
 * the real Poppins widths.
 ******************************************************************************
 */

#ifndef XWEIGHIN_GLANCE_LAYOUT_HPP
#define XWEIGHIN_GLANCE_LAYOUT_HPP

#include <cstdint>

#include "SDK/Glance/GlanceControl.h"

#include "GlanceView.hpp"

namespace XWeighIn
{
namespace Glance
{

struct Spec {
    enum class Type : uint8_t { Text, Rect };
    Type    type   = Type::Text;
    int16_t x      = 0, y = 0;
    int16_t w      = 0, h = 0;
    uint8_t font   = GLANCE_FONT_POPPINS_REGULAR_18;
    uint8_t colour = GLANCE_COLOR_WHITE;
    uint8_t align  = GLANCE_ALIGN_H_LEFT;
    char    text[GLANCE_TEXT_SIZE + 1] {};
};

struct Layout {
    static constexpr uint8_t kMax = 20;   ///< the full layout uses 17
    Spec    items[kMax] {};
    uint8_t count = 0;
    enum class Kind : uint8_t { Full, Compact, Tiny } kind = Kind::Full;
};

/// Controls the full layout needs: two texts, the baseline and a bar a day.
constexpr uint32_t kFullControls = 2 + 1 + kGlanceDays;

/// Lay out @p v in a @p width x @p height area with at most @p maxControls
/// controls.
void layout(const View& v, int16_t width, int16_t height, uint32_t maxControls, Layout& out);

}  // namespace Glance
}  // namespace XWeighIn

#endif  // XWEIGHIN_GLANCE_LAYOUT_HPP
