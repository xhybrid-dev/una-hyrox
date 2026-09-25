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
// Line heights of the glance faces used (Poppins; as the LVGL fonts' 23/22/13).
constexpr int16_t kHead = 24;   ///< SEMIBOLD_20
constexpr int16_t kWeek = 22;   ///< MEDIUM_18
constexpr int16_t kCoach = 13;  ///< MEDIUM_10
constexpr int16_t kFullHeight    = kHead + kWeek + kCoach;   ///< 59
constexpr int16_t kTwoLineHeight = kHead + kWeek;            ///< 46
constexpr uint32_t kFullControls = 8;    ///< 4 lines + 1 rect + 3 texts
constexpr int16_t  kFullWidth    = 170;  ///< mountain + the longest line

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

/// The three lines of words for a view.
struct Words {
    char    head[GLANCE_TEXT_SIZE + 1] {};
    char    week[GLANCE_TEXT_SIZE + 1] {};
    char    coach[GLANCE_TEXT_SIZE + 1] {};
    char    tiny[GLANCE_TEXT_SIZE + 1] {};
    uint8_t headColour  = GLANCE_COLOR_GREEN;
    uint8_t weekColour  = GLANCE_COLOR_WHITE;
    uint8_t coachColour = GLANCE_COLOR_GRAY;
};

void words(const Streak::HomeView& v, State state, Words& w)
{
    if (state == State::NoStreak) {
        std::snprintf(w.head, sizeof(w.head), "HybridX Streak");
        std::snprintf(w.week, sizeof(w.week), "Open it to start");
        std::snprintf(w.coach, sizeof(w.coach), "Every activity counts");
        std::snprintf(w.tiny, sizeof(w.tiny), "Open HybridX Streak");
        w.weekColour = GLANCE_COLOR_GRAY;
        return;
    }
    if (v.flags & Streak::HomeView::kClockUnset) {
        std::snprintf(w.head, sizeof(w.head), "Set the time");
        std::snprintf(w.week, sizeof(w.week), "in the UNA app");
        std::snprintf(w.coach, sizeof(w.coach), "Your streak is safe");
        std::snprintf(w.tiny, sizeof(w.tiny), "Streak: set the time");
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
    std::snprintf(w.tiny, sizeof(w.tiny), "%u wk streak %s %u/%u", u(v.streakWeeks), "\xC2\xB7", u(v.sessions),
                  u(v.target));

    const unsigned left = met ? 0u : u(v.target - v.sessions);
    if (v.flags & Streak::HomeView::kDecisionPending) {
        std::snprintf(w.coach, sizeof(w.coach), "Open to use a shield");
        w.coachColour = GLANCE_COLOR_YELLOW_DARK;
    } else if (met) {
        std::snprintf(w.coach, sizeof(w.coach), "Week banked. Rest up.");
        w.coachColour = GLANCE_COLOR_GREEN;
    } else if (v.mood == Streak::Mood::AtRisk) {
        std::snprintf(w.coach, sizeof(w.coach), "%u more in %u days", left, u(v.daysLeft));
        w.coachColour = GLANCE_COLOR_YELLOW_DARK;
    } else {
        const Streak::ClimbPosition pos = Streak::climbFor(v.weeksAchieved);
        const unsigned weeks = u(pos.steps - pos.stepsClimbed);
        std::snprintf(w.coach, sizeof(w.coach), "%s: %u week%s to go", Streak::kClimbs[pos.climb].name, weeks,
                      weeks == 1 ? "" : "s");
    }
}
} // namespace

void layout(const Streak::HomeView& v, State state, int16_t width, int16_t height, uint32_t maxControls, Layout& out)
{
    out = Layout {};
    Words w;
    words(v, state, w);

    if (height < kTwoLineHeight || maxControls < 2) {
        // Tiny: one line, as much as fits.
        out.kind = Layout::Kind::Tiny;
        text(out, 0, 0, width, height, w.tiny, GLANCE_FONT_POPPINS_MEDIUM_18, w.headColour, GLANCE_ALIGN_H_CENTER);
        return;
    }
    if (maxControls < kFullControls || width < kFullWidth || height < kFullHeight) {
        // Compact: the two lines of words, centred.
        out.kind          = Layout::Kind::Compact;
        const int16_t top = static_cast<int16_t>((height - kTwoLineHeight) / 2);
        text(out, 0, top, width, kHead, w.head, GLANCE_FONT_POPPINS_SEMIBOLD_20, w.headColour, GLANCE_ALIGN_H_CENTER);
        text(out, 0, static_cast<int16_t>(top + kHead), width, kWeek, w.week, GLANCE_FONT_POPPINS_MEDIUM_18,
             w.weekColour, GLANCE_ALIGN_H_CENTER);
        return;
    }

    // Full: a mountain on the left, drawn in lines (the glance has no filled
    // triangles), with a green summit flag; the words on the right.
    out.kind          = Layout::Kind::Full;
    const int16_t m    = static_cast<int16_t>(height - 4 < 56 ? height - 4 : 56);   // the mountain's box
    const int16_t x0   = 4;
    const int16_t base = static_cast<int16_t>(height - 3);
    const int16_t top  = static_cast<int16_t>(base - m + 14);
    const int16_t apex = static_cast<int16_t>(x0 + m / 2);
    line(out, x0, base, apex, top, GLANCE_COLOR_TEAL);
    line(out, apex, top, static_cast<int16_t>(x0 + m), base, GLANCE_COLOR_TEAL);
    line(out, static_cast<int16_t>(apex - 5), static_cast<int16_t>(top + 8), static_cast<int16_t>(apex + 5),
         static_cast<int16_t>(top + 8), GLANCE_COLOR_WHITE);
    line(out, apex, top, apex, static_cast<int16_t>(top - 11), GLANCE_COLOR_WHITE);
    rect(out, static_cast<int16_t>(apex + 1), static_cast<int16_t>(top - 11), 9, 6, GLANCE_COLOR_GREEN);

    const int16_t tx = static_cast<int16_t>(x0 + m + 8);
    const int16_t tw = static_cast<int16_t>(width - tx);
    const int16_t ty = static_cast<int16_t>((height - kFullHeight) / 2);
    text(out, tx, ty, tw, kHead, w.head, GLANCE_FONT_POPPINS_SEMIBOLD_20, w.headColour, GLANCE_ALIGN_H_LEFT);
    text(out, tx, static_cast<int16_t>(ty + kHead), tw, kWeek, w.week, GLANCE_FONT_POPPINS_MEDIUM_18, w.weekColour,
         GLANCE_ALIGN_H_LEFT);
    text(out, tx, static_cast<int16_t>(ty + kHead + kWeek), tw, kCoach, w.coach, GLANCE_FONT_POPPINS_MEDIUM_10,
         w.coachColour, GLANCE_ALIGN_H_LEFT);
}

} // namespace Glance
