/**
 ******************************************************************************
 * @file    GlanceLayout.hpp
 * @brief   What the VO2max glance draws, as control specs: pure, host-tested
 *          (GlanceLayoutTest). X Weigh-in's glance layout, for VO2max.
 *
 * The area and the control budget come from the watch at run time
 * (RequestGlanceConfig); Jon's watch gives 240x60 and 32 controls (HybridX
 * Streak NOTES, Gate 0). The glances list draws its separators only across
 * the middle 172 px, and a scroll arc over both edges, so everything keeps to
 * that middle band (X Weigh-in NOTES, Gate W3). The layout adapts:
 *   full     on the left the shown figure, "52.7" with a small "VO2", and
 *            under it the change at the latest run, "+0.4 last run"; on the
 *            right the shown figure as it stood after each of the last 10
 *            runs as bars, the newest in white, on a baseline (14 controls);
 *   compact  the two lines alone, centred (too few controls, or narrow);
 *   tiny     "52.7 VO2" alone (a very short area).
 * With no runs yet, or no Run app: "VO2max" and "No runs yet".
 *
 * Only text and filled rectangles: line controls do not draw on the watch
 * (Streak NOTES S4.3). Words are plain ASCII in 18-25 point faces (the 10
 * point face holds digits alone, S4.2). The bars use their own range, at
 * least kMinSpanX10 tall, so a steady run of figures looks steady.
 ******************************************************************************
 */

#ifndef RUN_GLANCE_LAYOUT_HPP
#define RUN_GLANCE_LAYOUT_HPP

#include <cstdint>

#include "SDK/Glance/GlanceControl.h"

#include "GlanceView.hpp"

namespace RunVo2
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
    static constexpr uint8_t kMax = 16;   ///< the full layout uses 14
    Spec    items[kMax] {};
    uint8_t count = 0;
    enum class Kind : uint8_t { Full, Compact, Tiny } kind = Kind::Full;
};

/// Controls the full layout needs: three texts, the baseline and a bar a run.
constexpr uint32_t kFullControls = 3 + 1 + kGlanceRuns;

/// The bars' scale is never shorter than this, x 10: 4.0 ml/kg/min.
constexpr int32_t kMinSpanX10 = 40;
/// Room below the lowest bar and above the highest, x 10: 0.5 ml/kg/min.
constexpr int32_t kPadX10 = 5;

/// Lay out @p v in a @p width x @p height area with at most @p maxControls
/// controls.
void layout(const View& v, int16_t width, int16_t height, uint32_t maxControls, Layout& out);

}  // namespace Glance
}  // namespace RunVo2

#endif  // RUN_GLANCE_LAYOUT_HPP
