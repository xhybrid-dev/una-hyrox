/**
 ******************************************************************************
 * @file    GlanceLayout.hpp
 * @brief   What X Training's glance draws, as control specs: pure, host-tested
 *          (GlanceLayoutTest).
 *
 * The area and the control budget come from the watch at run time
 * (RequestGlanceConfig); Jon's watch gives 240x60 and 32 controls (HybridX Streak
 * NOTES, Gate 0). What the platform has been seen to do (Streak NOTES S4.2-S4.4):
 * only text and filled rectangles draw (no lines); words need the 18 or 20 point
 * faces (the 10 point face holds digits alone); an 18 point line is 23 px tall,
 * SemiBold 20 is 25, so two lines and a strip of pips fill the 60 px. The layout
 * adapts:
 *   full     line 1 (SemiBold 20): "Today: Intervals"; line 2 (Regular 18, grey):
 *            "4 x 4 min @ 4:18/km"; under them the week's seven pips (done green,
 *            today white, to come grey, rest dark, missed amber) and a bar of
 *            how far through the plan it is (11 controls);
 *   compact  the two lines alone, centred (too few controls, or a short area);
 *   tiny     line 1 alone (a very short area).
 *
 * The words come in decreasing length ("Skipped: Intervals", "Intervals"; "4 x 4 min @
 * 4:18/km", "4 x 4 min") and the longest that fits the band is used, measured with the
 * face's own advance widths (GlanceFontWidths.hpp), so a long name or a mile pace
 * never runs off the face.
 ******************************************************************************
 */

#ifndef TRAINING_GLANCE_LAYOUT_HPP
#define TRAINING_GLANCE_LAYOUT_HPP

#include <cstdint>

#include "SDK/Glance/GlanceControl.h"

#include "GlanceView.hpp"

namespace TrainingGlance
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
    static constexpr uint8_t kMax = 12;   ///< the full layout uses 11
    Spec    items[kMax] {};
    uint8_t count = 0;
    enum class Kind : uint8_t { Full, Compact, Tiny } kind = Kind::Full;
};

/// Controls the full layout needs: two lines, seven pips, the bar's track and its fill.
constexpr uint32_t kFullControls = 2 + 7 + 2;

/// The width words may use, centred: held in from the list's scroll arcs on both edges
/// (Streak NOTES S4.3 kept 14 px in on the left and 8 on the right).
constexpr int16_t kBandW = 208;

/// Lay out @p v in a @p width x @p height area with at most @p maxControls controls.
void layout(const View& v, int16_t width, int16_t height, uint32_t maxControls, Layout& out);

/// The words for @p v that the full layout would draw, for tests and the preview tool: line 1,
/// line 2 (each at most GLANCE_TEXT_SIZE characters).
struct Words {
    char    head[GLANCE_TEXT_SIZE + 1] {};
    char    line[GLANCE_TEXT_SIZE + 1] {};
    uint8_t headColour = GLANCE_COLOR_WHITE;
};
void words(const View& v, Words& out);

}  // namespace TrainingGlance

#endif  // TRAINING_GLANCE_LAYOUT_HPP
