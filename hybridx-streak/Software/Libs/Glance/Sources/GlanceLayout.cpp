/**
 ******************************************************************************
 * @file    GlanceLayout.cpp
 * @brief   What the glance shows (see the header).
 ******************************************************************************
 */

#include "GlanceLayout.hpp"

#include <cstdio>
#include <cstring>

#include "Summits.hpp"

namespace Glance
{

namespace
{
// Text box heights: the glance fonts' line heights (the SDK's generated
// Poppins tables give SemiBold 20 as 25 px and every 18 as 23 px).
constexpr int16_t kHead = 25;   ///< SEMIBOLD_20
constexpr int16_t kLine = 23;   ///< REGULAR_18 / MEDIUM_18
// The full layout's middle row: one bead per session of the weekly target.
constexpr int16_t kBeadTop  = kHead + 3;
constexpr int16_t kBeadW    = 14;
constexpr int16_t kBeadH    = 8;
constexpr int16_t kBeadGap  = 6;
constexpr int16_t kFullHeight    = kBeadTop + kBeadH + 1 + kLine;   ///< 60
constexpr int16_t kTwoLineHeight = kHead + kLine;                   ///< 48
constexpr uint32_t kMountainControls = 5;   ///< 4 lines + the flag
constexpr uint8_t  kMaxBeads = 7;            ///< targets are 1..7 (Goal)
// The mountain's box, and the gap between it and the words. 52 + 6 leaves
// 178 px of words in a 240 px area: the longest message, "Arthur's Seat:
// 4 wks", is 173 px in Regular 18 (GlanceLayoutTest measures every one).
constexpr int16_t kMountain = 52;
constexpr int16_t kWordsGap = 6;
constexpr int16_t kFullWidth = 4 + kMountain + kWordsGap + 178;   ///< 240

unsigned u(uint32_t v) { return static_cast<unsigned>(v); }

Spec& add(Layout& out)
{
    Spec& s = out.items[out.count < Layout::kMax ? out.count++ : Layout::kMax - 1];
    s       = Spec {};
    return s;
}

void text(Layout& out, int16_t x, int16_t y, int16_t w, int16_t h, const char* str, uint8_t font, uint8_t colour,
          uint8_t align)
{
    Spec& s  = add(out);
    s.type   = Spec::Type::Text;
    s.x      = x;
    s.y      = y;
    s.w      = w;
    s.h      = h;
    s.font   = font;
    s.colour = colour;
    s.align  = align;
    std::snprintf(s.text, sizeof(s.text), "%s", str);
}

void line(Layout& out, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t colour)
{
    Spec& s  = add(out);
    s.type   = Spec::Type::Line;
    s.x      = x1;
    s.y      = y1;
    s.x2     = x2;
    s.y2     = y2;
    s.colour = colour;
}

void rect(Layout& out, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t colour)
{
    Spec& s  = add(out);
    s.type   = Spec::Type::Rect;
    s.x      = x;
    s.y      = y;
    s.w      = w;
    s.h      = h;
    s.colour = colour;
    s.fill   = true;
}

/// The words for a view. Plain ASCII only: the glance fonts have nothing
/// beyond it (the middle dot drew as '?').
struct Words {
    char    head[GLANCE_TEXT_SIZE + 1] {};
    char    week[GLANCE_TEXT_SIZE + 1] {};    ///< compact layout's second line
    char    coach[GLANCE_TEXT_SIZE + 1] {};   ///< full layout's bottom line
    char    tiny[GLANCE_TEXT_SIZE + 1] {};
    bool    beads       = true;               ///< the week's beads mean something
    uint8_t headColour  = GLANCE_COLOR_GREEN;
    uint8_t weekColour  = GLANCE_COLOR_WHITE;
    uint8_t coachColour = GLANCE_COLOR_GRAY;
};

void words(const Streak::HomeView& v, State state, Words& w)
{
    if (state == State::NoStreak) {
        std::snprintf(w.head, sizeof(w.head), "HybridX Streak");
        std::snprintf(w.week, sizeof(w.week), "Open it to start");
        std::snprintf(w.coach, sizeof(w.coach), "Open it to start");
        std::snprintf(w.tiny, sizeof(w.tiny), "Open HybridX Streak");
        w.beads      = false;
        w.weekColour = GLANCE_COLOR_GRAY;
        return;
    }
    if (v.flags & Streak::HomeView::kClockUnset) {
        std::snprintf(w.head, sizeof(w.head), "Set the time");
        std::snprintf(w.week, sizeof(w.week), "in the UNA app");
        std::snprintf(w.coach, sizeof(w.coach), "in the UNA app");
        std::snprintf(w.tiny, sizeof(w.tiny), "Streak: set the time");
        w.beads      = false;
        w.headColour = GLANCE_COLOR_YELLOW_DARK;
        w.weekColour = GLANCE_COLOR_GRAY;
        return;
    }

    const bool met = v.sessions >= v.target;
    if (v.streakWeeks > 0) {
        std::snprintf(w.head, sizeof(w.head), "%u week streak", u(v.streakWeeks));
    } else if (v.flags & Streak::HomeView::kTrialWeek) {
        std::snprintf(w.head, sizeof(w.head), "First week");
    } else {
        std::snprintf(w.head, sizeof(w.head), "New streak");
        w.headColour = GLANCE_COLOR_WHITE;
    }
    std::snprintf(w.week, sizeof(w.week), "%u of %u this week", u(v.sessions), u(v.target));
    w.weekColour = met ? GLANCE_COLOR_GREEN : GLANCE_COLOR_WHITE;
    std::snprintf(w.tiny, sizeof(w.tiny), "%u wk streak, %u/%u", u(v.streakWeeks), u(v.sessions), u(v.target));

    // The bottom line: what matters most now, first.
    const unsigned left = met ? 0u : u(v.target - v.sessions);
    if (v.flags & Streak::HomeView::kDecisionPending) {
        std::snprintf(w.coach, sizeof(w.coach), "Shield? Open app");
        w.coachColour = GLANCE_COLOR_YELLOW_DARK;
    } else if (met) {
        std::snprintf(w.coach, sizeof(w.coach), "Banked. Rest up.");
        w.coachColour = GLANCE_COLOR_GREEN;
    } else if (v.mood == Streak::Mood::AtRisk) {
        if (v.daysLeft <= 1) {
            std::snprintf(w.coach, sizeof(w.coach), "Last day: %u more", left);
        } else {
            std::snprintf(w.coach, sizeof(w.coach), "%u more, %u days left", left, u(v.daysLeft));
        }
        w.coachColour = GLANCE_COLOR_YELLOW_DARK;
    } else {
        // On track: the next summit.
        const Streak::ClimbPosition pos = Streak::climbFor(v.weeksAchieved);
        const unsigned weeks = u(pos.steps - pos.stepsClimbed);
        std::snprintf(w.coach, sizeof(w.coach), "%s: %u wk%s", Streak::kClimbs[pos.climb].name, weeks,
                      weeks == 1 ? "" : "s");
    }
}
} // namespace

void layout(const Streak::HomeView& v, State state, int16_t width, int16_t height, uint32_t maxControls, Layout& out)
{
    out = Layout {};
    Words w;
    words(v, state, w);

    const uint8_t  beads    = !w.beads ? 0 : (v.target < 1 ? 1 : (v.target > kMaxBeads ? kMaxBeads : v.target));
    const uint32_t controls = kMountainControls + 2 + beads;

    if (height < kTwoLineHeight || maxControls < 2) {
        // Tiny: one line, as much as fits.
        out.kind = Layout::Kind::Tiny;
        const int16_t h = height < kLine ? height : kLine;
        text(out, 0, static_cast<int16_t>((height - h) / 2), width, h, w.tiny, GLANCE_FONT_POPPINS_MEDIUM_18,
             w.headColour, GLANCE_ALIGN_H_CENTER);
        return;
    }
    if (maxControls < controls || width < kFullWidth || height < kFullHeight) {
        // Compact: the two lines of words, centred.
        out.kind          = Layout::Kind::Compact;
        const int16_t top = static_cast<int16_t>((height - kTwoLineHeight) / 2);
        text(out, 0, top, width, kHead, w.head, GLANCE_FONT_POPPINS_SEMIBOLD_20, w.headColour, GLANCE_ALIGN_H_CENTER);
        text(out, 0, static_cast<int16_t>(top + kHead), width, kLine, w.week, GLANCE_FONT_POPPINS_MEDIUM_18,
             w.weekColour, GLANCE_ALIGN_H_CENTER);
        return;
    }

    // Full: a mountain on the left, drawn in lines (the glance has no filled
    // triangles), with a green summit flag. On the right: the streak, a bead
    // per session of the week's target, and one line of what matters now.
    out.kind           = Layout::Kind::Full;
    const int16_t oy   = static_cast<int16_t>((height - kFullHeight) / 2);
    const int16_t x0   = 4;
    const int16_t base = static_cast<int16_t>(oy + kFullHeight - 3);
    const int16_t top  = static_cast<int16_t>(base - kMountain + 14);
    const int16_t apex = static_cast<int16_t>(x0 + kMountain / 2);
    line(out, x0, base, apex, top, GLANCE_COLOR_TEAL);
    line(out, apex, top, static_cast<int16_t>(x0 + kMountain), base, GLANCE_COLOR_TEAL);
    line(out, static_cast<int16_t>(apex - 5), static_cast<int16_t>(top + 8), static_cast<int16_t>(apex + 5),
         static_cast<int16_t>(top + 8), GLANCE_COLOR_WHITE);
    line(out, apex, top, apex, static_cast<int16_t>(top - 11), GLANCE_COLOR_WHITE);
    rect(out, static_cast<int16_t>(apex + 1), static_cast<int16_t>(top - 11), 9, 6, GLANCE_COLOR_GREEN);

    const int16_t tx = static_cast<int16_t>(x0 + kMountain + kWordsGap);
    const int16_t tw = static_cast<int16_t>(width - tx);
    text(out, tx, oy, tw, kHead, w.head, GLANCE_FONT_POPPINS_SEMIBOLD_20, w.headColour, GLANCE_ALIGN_H_LEFT);
    // Beads: done in green, still to do in grey. Both filled: a filled
    // rect is what the flag already proves on the watch.
    for (uint8_t i = 0; i < beads; ++i) {
        rect(out, static_cast<int16_t>(tx + 1 + i * (kBeadW + kBeadGap)), static_cast<int16_t>(oy + kBeadTop), kBeadW,
             kBeadH, i < v.sessions ? GLANCE_COLOR_GREEN : GLANCE_COLOR_GRAY);
    }
    const int16_t ly = static_cast<int16_t>(oy + (beads > 0 ? kFullHeight - kLine : kHead + 2));
    text(out, tx, ly, tw, kLine, w.coach, GLANCE_FONT_POPPINS_REGULAR_18, w.coachColour, GLANCE_ALIGN_H_LEFT);
}

} // namespace Glance
