/**
 ******************************************************************************
 * @file    GlanceLayout.cpp
 * @brief   What the glance draws (see the header).
 ******************************************************************************
 */

#include "GlanceLayout.hpp"

#include <cstdio>

#include "GraphScale.hpp"
#include "WeighText.hpp"

namespace XWeighIn
{
namespace Glance
{

namespace
{
// Text box heights: the glance fonts' line heights (the SDK's generated
// Poppins tables give SemiBold 25 as 32 px, SemiBold 20 as 25 and every 18
// as 23).
constexpr int16_t kBigHead = 32;   ///< SEMIBOLD_25, the full layout's trend
constexpr int16_t kHead    = 25;   ///< SEMIBOLD_20, the compact layout's
constexpr int16_t kLine    = 23;   ///< REGULAR_18 / MEDIUM_18

// The full layout, for a 240 px wide area: 4 px in from each edge (a 60 px
// strip across the middle of the round display is 232 px wide at its top and
// bottom rows), the words in a 132 px column, a 4 px gap, then the bars.
constexpr int16_t kEdge      = 4;
constexpr int16_t kWordsW    = 132;   ///< "-10.4 kg/week" is 128 px in Regular 18
constexpr int16_t kBarW      = 5;
constexpr int16_t kBarGap    = 2;
constexpr int16_t kBarsW     = kGlanceDays * kBarW + (kGlanceDays - 1) * kBarGap;   ///< 96
constexpr int16_t kFullWidth = kEdge + kWordsW + 4 + kBarsW + kEdge;               ///< 240
constexpr int16_t kFullHeight    = kBigHead + kLine;   ///< 55
constexpr int16_t kTwoLineHeight = kHead + kLine;      ///< 48
// The bars stand on a 2 px baseline; the tallest is kBarsH.
constexpr int16_t kBarsTop  = 4;
constexpr int16_t kBarsH    = 48;
constexpr int16_t kBaseH    = 2;
constexpr int16_t kMinBarH  = 2;

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

/// The two lines of words: "81.9 kg" and "-0.4 kg/week", or the empty state.
struct Words {
    char head[GLANCE_TEXT_SIZE + 1] {};
    char line[GLANCE_TEXT_SIZE + 1] {};
};

void words(const View& v, Words& w)
{
    if (!v.hasData) {
        std::snprintf(w.head, sizeof(w.head), "X Weigh-in");
        std::snprintf(w.line, sizeof(w.line), "Open it to weigh in");
        return;
    }
    char kg[16];
    formatKg(v.trend, 1, kg, sizeof(kg));
    std::snprintf(w.head, sizeof(w.head), "%s kg", kg);
    char change[16];
    if (v.hasWeekChange) {
        // A change of 100 kg or more (only a mistyped log could hold one)
        // drops its decimal, so the line still fits its column.
        const bool huge = v.weekChange >= 10000 || v.weekChange <= -10000;
        formatChange(v.weekChange, huge ? 0 : 1, change, sizeof(change));
    } else {
        std::snprintf(change, sizeof(change), "--");
    }
    std::snprintf(w.line, sizeof(w.line), "%s kg/week", change);
}

/// The bars, right of the words: one per day with a weigh-in, scaled to the
/// days' own range (GraphScale: at least 2 kg tall), on a baseline.
void bars(const View& v, int16_t x0, int16_t oy, Layout& out)
{
    const int16_t base = static_cast<int16_t>(oy + kBarsTop + kBarsH);
    rect(out, x0, base, kBarsW, kBaseH, GLANCE_COLOR_GRAY_DARK);

    Range r;
    if (!rangeFor(v.weight, nullptr, kGlanceDays, r)) {
        return;   // no weigh-in in the last 14 days: the baseline alone
    }
    int newest = -1;
    for (int i = kGlanceDays - 1; i >= 0; --i) {
        if (v.weight[i] != 0) {
            newest = i;
            break;
        }
    }
    for (uint8_t i = 0; i < kGlanceDays; ++i) {
        if (v.weight[i] == 0) {
            continue;   // a missed day: a gap
        }
        int32_t top = yFor(v.weight[i], r, oy + kBarsTop, kBarsH);
        if (base - top < kMinBarH) {
            top = base - kMinBarH;
        }
        rect(out, static_cast<int16_t>(x0 + i * (kBarW + kBarGap)), static_cast<int16_t>(top), kBarW,
             static_cast<int16_t>(base - top), i == newest ? GLANCE_COLOR_WHITE : GLANCE_COLOR_GRAY);
    }
}
}  // namespace

void layout(const View& v, int16_t width, int16_t height, uint32_t maxControls, Layout& out)
{
    out = Layout {};
    Words w;
    words(v, w);

    if (height < kTwoLineHeight || maxControls < 2) {
        // Tiny: one line, as much as fits.
        out.kind        = Layout::Kind::Tiny;
        const int16_t h = height < kLine ? height : kLine;
        text(out, 0, static_cast<int16_t>((height - h) / 2), width, h, v.hasData ? w.head : "Open X Weigh-in",
             GLANCE_FONT_POPPINS_MEDIUM_18, GLANCE_COLOR_WHITE, GLANCE_ALIGN_H_CENTER);
        return;
    }
    if (!v.hasData || maxControls < kFullControls || width < kFullWidth || height < kFullHeight) {
        // Compact: the two lines of words, centred.
        out.kind          = Layout::Kind::Compact;
        const int16_t top = static_cast<int16_t>((height - kTwoLineHeight) / 2);
        text(out, 0, top, width, kHead, w.head, GLANCE_FONT_POPPINS_SEMIBOLD_20, GLANCE_COLOR_WHITE,
             GLANCE_ALIGN_H_CENTER);
        text(out, 0, static_cast<int16_t>(top + kHead), width, kLine, w.line, GLANCE_FONT_POPPINS_REGULAR_18,
             GLANCE_COLOR_GRAY, GLANCE_ALIGN_H_CENTER);
        return;
    }

    // Full: the words on the left, the bars on the right, the block centred
    // in the area.
    out.kind         = Layout::Kind::Full;
    const int16_t ox = static_cast<int16_t>((width - kFullWidth) / 2);
    const int16_t oy = static_cast<int16_t>((height - (kBarsTop + kBarsH + kBaseH)) / 2);
    const int16_t ty = static_cast<int16_t>((height - kFullHeight) / 2);
    text(out, static_cast<int16_t>(ox + kEdge), ty, kWordsW, kBigHead, w.head, GLANCE_FONT_POPPINS_SEMIBOLD_25,
         GLANCE_COLOR_WHITE, GLANCE_ALIGN_H_LEFT);
    text(out, static_cast<int16_t>(ox + kEdge), static_cast<int16_t>(ty + kBigHead), kWordsW, kLine, w.line,
         GLANCE_FONT_POPPINS_REGULAR_18, GLANCE_COLOR_GRAY, GLANCE_ALIGN_H_LEFT);
    bars(v, static_cast<int16_t>(ox + kFullWidth - kEdge - kBarsW), oy, out);
}

}  // namespace Glance
}  // namespace XWeighIn
