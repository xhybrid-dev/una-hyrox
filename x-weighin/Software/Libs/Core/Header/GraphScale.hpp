/**
 ******************************************************************************
 * @file    GraphScale.hpp
 * @brief   The graph's vertical scale, chosen from the data, and the mapping
 *          from a weight or a day to a pixel.
 *
 * Header-only (the GUI uses it), integers only, host-tested.
 ******************************************************************************
 */

#ifndef XWEIGHIN_GRAPH_SCALE_HPP
#define XWEIGHIN_GRAPH_SCALE_HPP

#include <cstdint>

namespace XWeighIn
{

/// A weight range in 0.01 kg, lo < hi.
struct Range {
    int32_t lo = 0;
    int32_t hi = 0;
};

/// The scale is never shorter than this: a steady week should look steady,
/// not like a cliff. 2 kg.
constexpr int32_t kGraphMinSpan = 200;
/// Ends are snapped to this, so the labels read cleanly. 0.5 kg.
constexpr int32_t kGraphSnap    = 50;
/// Room above and below the data. 0.2 kg.
constexpr int32_t kGraphPad     = 20;

inline int32_t floorSnap(int32_t v)
{
    const int32_t q = v / kGraphSnap;
    return (v < 0 && q * kGraphSnap != v ? q - 1 : q) * kGraphSnap;
}

inline int32_t ceilSnap(int32_t v)
{
    const int32_t f = floorSnap(v);
    return f == v ? v : f + kGraphSnap;
}

/**
 * @brief   The range for the non-zero values of two arrays (the day weights
 *          and the trend), padded, snapped, and at least kGraphMinSpan tall.
 * @return  false if every value is 0 (nothing to draw).
 */
inline bool rangeFor(const uint16_t* a, const uint16_t* b, uint8_t n, Range& out)
{
    int32_t lo = 0;
    int32_t hi = 0;
    bool    any = false;
    for (uint8_t i = 0; i < n; ++i) {
        const uint16_t vs[2] = { a ? a[i] : uint16_t(0), b ? b[i] : uint16_t(0) };
        for (uint16_t v : vs) {
            if (v == 0) {
                continue;
            }
            if (!any || v < lo) lo = v;
            if (!any || v > hi) hi = v;
            any = true;
        }
    }
    if (!any) {
        return false;
    }
    int32_t top    = ceilSnap(hi + kGraphPad);
    int32_t bottom = floorSnap(lo - kGraphPad);
    if (top - bottom < kGraphMinSpan) {
        // Grow evenly about the middle, then snap the bottom and fix the span.
        const int32_t mid = (lo + hi) / 2;
        bottom = floorSnap(mid - kGraphMinSpan / 2);
        top    = bottom + kGraphMinSpan;
        if (top < hi + kGraphPad) {
            top = ceilSnap(hi + kGraphPad);
        }
    }
    if (bottom < 0) {
        top   -= bottom;
        bottom = 0;
    }
    out.lo = bottom;
    out.hi = top;
    return true;
}

/// The pixel row for @p value: @p top is @p range.hi, top + height - 1 is
/// range.lo. Clamped inside the plot.
inline int32_t yFor(int32_t value, const Range& range, int32_t top, int32_t height)
{
    if (height <= 1 || range.hi <= range.lo) {
        return top;
    }
    int64_t y = top + static_cast<int64_t>(range.hi - value) * (height - 1) / (range.hi - range.lo);
    if (y < top) y = top;
    if (y > top + height - 1) y = top + height - 1;
    return static_cast<int32_t>(y);
}

/// The pixel column for day @p index of @p slots (index 0 at @p left, the
/// last slot at left + width - 1).
inline int32_t xFor(uint8_t index, uint8_t slots, int32_t left, int32_t width)
{
    if (slots <= 1 || width <= 1) {
        return left + width / 2;
    }
    const uint8_t i = index < slots ? index : static_cast<uint8_t>(slots - 1);
    return left + static_cast<int32_t>(i) * (width - 1) / (slots - 1);
}

}  // namespace XWeighIn

#endif  // XWEIGHIN_GRAPH_SCALE_HPP
