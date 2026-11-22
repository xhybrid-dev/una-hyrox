/**
 ******************************************************************************
 * @file    GraphMath.hpp
 * @brief   The training load graphs' scales and pixel mapping (NOTES "S8").
 *
 * As X Weigh-in's GraphScale: the scale is chosen from the data, its ends are
 * round numbers for the labels, and a value or a day maps to a pixel inside
 * the plot. Header-only (the GUI uses it), integers only, host-tested.
 *
 * Values arrive in tenths (fitness and fatigue, LoadTrend) or whole points
 * (daily load); a range is in the same unit as its values.
 ******************************************************************************
 */

#ifndef STREAK_GRAPH_MATH_HPP
#define STREAK_GRAPH_MATH_HPP

#include <cstdint>

namespace Streak::GraphMath
{

/// A value range, lo < hi.
struct Range {
    int32_t lo = 0;
    int32_t hi = 0;
};

/// The smallest round number at or above @p v: 1, 2, 3, 4, 5, 6, 8, 10, 15, 20, 30, 40, 50, 60,
/// 80, 100, 150, ... (at least 1).
constexpr int32_t niceCeil(int32_t v)
{
    constexpr int32_t kMantissa[] = {10, 15, 20, 30, 40, 50, 60, 80};   // tenths of a decade
    if (v <= 1) {
        return 1;
    }
    for (int64_t p = 1; p <= 100000000; p *= 10) {
        for (int32_t m : kMantissa) {
            const int64_t c10 = m * p;   // ten times the candidate
            if (c10 % 10 != 0) {
                continue;                // 1.5 is not a whole number
            }
            if (c10 / 10 >= v) {
                return static_cast<int32_t>(c10 / 10);
            }
        }
    }
    return v;
}

/// From 0 to a round number above the largest of @p a and @p b (either may be null), in units of
/// @p unit (10 for tenths, so the labels are whole points), with some room above the top value
/// and at least @p minTop units.
inline Range fromZero(const uint16_t* a, const uint16_t* b, uint8_t n, int32_t unit, int32_t minTop)
{
    int32_t top = 0;
    for (uint8_t i = 0; i < n; ++i) {
        if (a != nullptr && a[i] > top) top = a[i];
        if (b != nullptr && b[i] > top) top = b[i];
    }
    const int32_t u      = unit < 1 ? 1 : unit;
    const int32_t whole  = (top + top / 20 + u - 1) / u;   // 5 % headroom, in whole units, rounded up
    const int32_t minW   = (minTop + u - 1) / u;
    Range r;
    r.lo = 0;
    r.hi = niceCeil(whole > minW ? whole : minW) * u;
    return r;
}

/// About zero, the same distance each way: a round number past the furthest of @p a - @p b (form,
/// from fitness and fatigue), at least @p minHalf units.
inline Range aroundZero(const uint16_t* a, const uint16_t* b, uint8_t n, int32_t unit, int32_t minHalf)
{
    int32_t far = 0;
    for (uint8_t i = 0; i < n; ++i) {
        const int32_t f = static_cast<int32_t>(a[i]) - static_cast<int32_t>(b[i]);
        const int32_t m = f < 0 ? -f : f;
        far = m > far ? m : far;
    }
    const int32_t u     = unit < 1 ? 1 : unit;
    const int32_t whole = (far + far / 20 + u - 1) / u;
    const int32_t minW  = (minHalf + u - 1) / u;
    const int32_t half  = niceCeil(whole > minW ? whole : minW) * u;
    Range r;
    r.lo = -half;
    r.hi = half;
    return r;
}

/// The pixel row for @p value: range.hi at @p top, range.lo at top + height - 1. Clamped inside
/// the plot.
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

/// The pixel column for slot @p index of @p slots: slot 0 at @p left, the last at left + width - 1.
inline int32_t xFor(uint8_t index, uint8_t slots, int32_t left, int32_t width)
{
    if (slots <= 1 || width <= 1) {
        return left + width / 2;
    }
    const uint8_t i = index < slots ? index : static_cast<uint8_t>(slots - 1);
    return left + static_cast<int32_t>(i) * (width - 1) / (slots - 1);
}

} // namespace Streak::GraphMath

#endif // STREAK_GRAPH_MATH_HPP
