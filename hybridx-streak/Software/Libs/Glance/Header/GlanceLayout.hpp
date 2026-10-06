/**
 ******************************************************************************
 * @file    GlanceLayout.hpp
 * @brief   What the glance shows, as control specs: pure, host-tested.
 *
 * The glance's area and control budget come from the watch at run time
 * (RequestGlanceConfig), and the S0 probe has yet to report the real ones, so
 * the layout adapts (PLAN 8):
 *   full     a mountain of stacked blocks with a flag, then "7 week streak", a row
 *            of beads (one per session of the week's target, done in green)
 *            and one line of what matters now: a shield waiting, the days
 *            left when at risk, "Banked. Rest up.", or the next summit
 *            ("Ben Nevis: 14 wks");
 *   compact  two lines of text alone, centred: "7 week streak" and
 *            "2 of 3 this week" (too few controls, or narrow);
 *   tiny     one line: "7 wk streak, 2/3" (a very short area).
 * Special states replace the words: no streak yet ("Open HybridX Streak"),
 * the clock unset ("Set the time"), a shield decision waiting.
 *
 * Colours are the glance's own 16 (GlanceControl.h), which have no lime:
 * GREEN stands in for it. Words use only 18 and 20 point faces and plain
 * ASCII: the 10 point face holds digits alone, so letters drew as '?'.
 * Every text is at most GLANCE_TEXT_SIZE bytes and fits its box in the real
 * Poppins widths, and every control lies inside the area -- GlanceLayoutTest
 * checks all three for a range of areas, budgets and states.
 ******************************************************************************
 */

#ifndef STREAK_GLANCE_LAYOUT_HPP
#define STREAK_GLANCE_LAYOUT_HPP

#include <cstdint>

#include "SDK/Glance/GlanceControl.h"

#include "StreakView.hpp"

namespace Glance
{

struct Spec {
    enum class Type : uint8_t { Text, Line, Rect };
    Type    type   = Type::Text;
    int16_t x      = 0, y = 0;   ///< text/rect position, or a line's start
    int16_t w      = 0, h = 0;   ///< text/rect size
    int16_t x2     = 0, y2 = 0;  ///< a line's end
    uint8_t font   = GLANCE_FONT_POPPINS_MEDIUM_18;
    uint8_t colour = GLANCE_COLOR_WHITE;
    uint8_t align  = GLANCE_ALIGN_H_LEFT;
    bool    fill   = false;
    char    text[GLANCE_TEXT_SIZE + 1] {};
};

struct Layout {
    static constexpr uint8_t kMax = 20;   ///< the full layout uses at most 17
    Spec    items[kMax] {};
    uint8_t count = 0;
    enum class Kind : uint8_t { Full, Compact, Tiny } kind = Kind::Full;
};

enum class State : uint8_t {
    Normal,     ///< the streak as the view says
    NoStreak,   ///< the app has never been opened: nothing saved yet
};

/// Lay out @p v (or @p state) in a @p width x @p height area with at most
/// @p maxControls controls.
void layout(const Streak::HomeView& v, State state, int16_t width, int16_t height, uint32_t maxControls, Layout& out);

} // namespace Glance

#endif // STREAK_GLANCE_LAYOUT_HPP
