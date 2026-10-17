/**
 ******************************************************************************
 * @file    GlanceLayout.cpp
 * @brief   What the VO2max glance draws (see the header).
 ******************************************************************************
 */

#include "GlanceLayout.hpp"

#include <cstdio>

#include "Vo2Text.hpp"

namespace RunVo2
{
namespace Glance
{

namespace
{
// Text box heights: the glance fonts' line heights (the SDK's generated
// Poppins tables give SemiBold 25 as 32 px, SemiBold 20 as 25 and every 18
// as 23).
constexpr int16_t kBigHead = 32;   ///< SEMIBOLD_25, the full layout's figure
constexpr int16_t kHead    = 25;   ///< SEMIBOLD_20, the compact layout's
constexpr int16_t kLine    = 23;   ///< REGULAR_18 / MEDIUM_18

// The middle band the glances list lines up with (see the header).
constexpr int16_t kSafeW = 172;
// The full layout in that band: the words in a column on the left, the bars
// on the right, a 4 px gap between.
constexpr int16_t kBarW   = 4;
constexpr int16_t kBarGap = 1;
constexpr int16_t kBarsW  = kGlanceRuns * kBarW + (kGlanceRuns - 1) * kBarGap;   ///< 49
constexpr int16_t kWordsW = kSafeW - 4 - kBarsW;   ///< 119: "-9.9 last run" is 109 px in Regular 18
// The figure and, after it, a small "VO2" (MEDIUM_18, 34 px) in the column.
constexpr int16_t kFigureW = 64;   ///< "88.8" is 61 px in SemiBold 25
constexpr int16_t kUnitW   = kWordsW - kFigureW;
// MEDIUM_18 is 9 px shorter than SEMIBOLD_25; this sits the two on about the
// same baseline.
constexpr int16_t kUnitDrop = 6;
constexpr int16_t kFullWidth     = 240;   ///< the watch's area; anything narrower is compact
constexpr int16_t kFullHeight    = kBigHead + kLine;   ///< 55
constexpr int16_t kTwoLineHeight = kHead + kLine;      ///< 48
// The bars stand on a 2 px baseline; the tallest is kBarsH.
constexpr int16_t kBarsTop = 4;
constexpr int16_t kBarsH   = 48;
constexpr int16_t kBaseH   = 2;
constexpr int16_t kMinBarH = 2;

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

void rect(Layout& out, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t colour)
{
    Spec& s  = add(out);
    s.type   = Spec::Type::Rect;
    s.x      = x;
    s.y      = y;
    s.w      = w;
    s.h      = h;
    s.colour = colour;
}

/// The words: "52.7", "52.7 VO2" and "+0.4 last run", or the empty state.
struct Words {
    char figure[8] {};   // "88.8"
    char head[GLANCE_TEXT_SIZE + 1] {};
    char line[GLANCE_TEXT_SIZE + 1] {};
};

void words(const View& v, Words& w)
{
    if (!v.hasData) {
        std::snprintf(w.head, sizeof(w.head), "VO2max");
        std::snprintf(w.line, sizeof(w.line), "No runs yet");
        return;
    }
    Text::formatX10(v.vo2x10, w.figure, sizeof(w.figure));
    std::snprintf(w.head, sizeof(w.head), "%s VO2", w.figure);
    if (v.beforeX10 == 0) {
        std::snprintf(w.line, sizeof(w.line), "First run");
        return;
    }
    const bool     up = v.vo2x10 > v.beforeX10;
    const unsigned d  = up ? v.vo2x10 - v.beforeX10 : v.beforeX10 - v.vo2x10;
    const char     sign = d == 0 ? '\0' : (up ? '+' : '-');
    char           sigil[2] = { sign, '\0' };
    if (d >= 100) {
        // 10 or more (only the first runs move it that far): no decimal, so
        // the line still fits its column.
        std::snprintf(w.line, sizeof(w.line), "%s%u last run", sigil, (d + 5) / 10);
    } else {
        std::snprintf(w.line, sizeof(w.line), "%s%u.%u last run", sigil, d / 10, d % 10);
    }
}

/// The bars, right of the words: the trend, newest on the right, scaled to
/// its own range (at least kMinSpanX10 tall), on a baseline.
void bars(const View& v, int16_t x0, int16_t oy, Layout& out)
{
    const int16_t base = static_cast<int16_t>(oy + kBarsTop + kBarsH);
    rect(out, x0, base, kBarsW, kBaseH, GLANCE_COLOR_GRAY_DARK);
    if (v.count == 0) {
        return;
    }
    int32_t lo = v.trend[0];
    int32_t hi = v.trend[0];
    for (uint8_t i = 1; i < v.count; ++i) {
        if (v.trend[i] < lo) lo = v.trend[i];
        if (v.trend[i] > hi) hi = v.trend[i];
    }
    lo -= kPadX10;
    hi += kPadX10;
    if (hi - lo < kMinSpanX10) {
        // Grow evenly about the middle.
        const int32_t mid = (lo + hi) / 2;
        lo = mid - kMinSpanX10 / 2;
        hi = lo + kMinSpanX10;
    }
    const int32_t span = hi - lo;   // >= kMinSpanX10 > 0
    const uint8_t first = static_cast<uint8_t>(kGlanceRuns - v.count);
    for (uint8_t i = 0; i < v.count; ++i) {
        int32_t h = (static_cast<int32_t>(v.trend[i]) - lo) * kBarsH / span;
        if (h < kMinBarH) h = kMinBarH;
        if (h > kBarsH) h = kBarsH;
        rect(out, static_cast<int16_t>(x0 + (first + i) * (kBarW + kBarGap)), static_cast<int16_t>(base - h), kBarW,
             static_cast<int16_t>(h), i == v.count - 1 ? GLANCE_COLOR_WHITE : GLANCE_COLOR_GRAY);
    }
}
}  // namespace

void layout(const View& v, int16_t width, int16_t height, uint32_t maxControls, Layout& out)
{
    out = Layout {};
    Words w;
    words(v, w);

    // The band everything keeps to, centred; an area narrower than the band
    // uses its full width.
    const int16_t bw = width < kSafeW ? width : kSafeW;
    const int16_t bx = static_cast<int16_t>((width - bw) / 2);

    if (height < kTwoLineHeight || maxControls < 2) {
        // Tiny: one line, as much as fits.
        out.kind        = Layout::Kind::Tiny;
        const int16_t h = height < kLine ? height : kLine;
        text(out, bx, static_cast<int16_t>((height - h) / 2), bw, h, v.hasData ? w.head : "Open HybridX Run",
             GLANCE_FONT_POPPINS_MEDIUM_18, GLANCE_COLOR_WHITE, GLANCE_ALIGN_H_CENTER);
        return;
    }
    if (!v.hasData || maxControls < kFullControls || width < kFullWidth || height < kFullHeight) {
        // Compact: the two lines of words, centred.
        out.kind          = Layout::Kind::Compact;
        const int16_t top = static_cast<int16_t>((height - kTwoLineHeight) / 2);
        text(out, bx, top, bw, kHead, w.head, GLANCE_FONT_POPPINS_SEMIBOLD_20, GLANCE_COLOR_WHITE,
             GLANCE_ALIGN_H_CENTER);
        text(out, bx, static_cast<int16_t>(top + kHead), bw, kLine, w.line, GLANCE_FONT_POPPINS_REGULAR_18,
             GLANCE_COLOR_GRAY, GLANCE_ALIGN_H_CENTER);
        return;
    }

    // Full: the words on the left of the band, the bars on the right,
    // centred top to bottom.
    out.kind         = Layout::Kind::Full;
    const int16_t oy = static_cast<int16_t>((height - (kBarsTop + kBarsH + kBaseH)) / 2);
    const int16_t ty = static_cast<int16_t>((height - kFullHeight) / 2);
    text(out, bx, ty, kFigureW, kBigHead, w.figure, GLANCE_FONT_POPPINS_SEMIBOLD_25, GLANCE_COLOR_WHITE,
         GLANCE_ALIGN_H_LEFT);
    text(out, static_cast<int16_t>(bx + kFigureW), static_cast<int16_t>(ty + kUnitDrop), kUnitW, kLine, "VO2",
         GLANCE_FONT_POPPINS_MEDIUM_18, GLANCE_COLOR_GRAY, GLANCE_ALIGN_H_LEFT);
    text(out, bx, static_cast<int16_t>(ty + kBigHead), kWordsW, kLine, w.line, GLANCE_FONT_POPPINS_REGULAR_18,
         GLANCE_COLOR_GRAY, GLANCE_ALIGN_H_LEFT);
    bars(v, static_cast<int16_t>(bx + bw - kBarsW), oy, out);
}

}  // namespace Glance
}  // namespace RunVo2
