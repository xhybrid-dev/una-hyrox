/**
 ******************************************************************************
 * @file    WorkoutFormat.hpp
 * @brief   HybridX Intervals: fitting workout text to the screen.
 *
 * The words come from Core's WorkoutText (host-tested). This adds what needs
 * LVGL: the watch fonts are ASCII only, so a name (UTF-8) is folded
 * (TextFold, from Trail), and fitted to its room by stepping the font down,
 * then cut with "..". Copied from HybridX Trail's RouteFormat.
 ******************************************************************************
 */

#ifndef WORKOUT_FORMAT_HPP
#define WORKOUT_FORMAT_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "lvgl.h"

#include "TextFold.hpp"

namespace WorkoutFmt
{

/// @p in (UTF-8) as printable ASCII in @p out.
inline void name(char* out, size_t cap, const char* in)
{
    Intervals::Text::asciiFold(out, cap, in);
}

/// Width of ASCII @p text in @p font, in pixels (with kerning).
inline int32_t textWidth(const char* text, const lv_font_t* font)
{
    int32_t w = 0;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        w += lv_font_get_glyph_width(font, static_cast<uint8_t>(text[i]), static_cast<uint8_t>(text[i + 1]));
    }
    return w;
}

/// The first of @p faces (largest first) that @p text fits in @p maxW. If it
/// fits none, @p text is cut short, with "..", to fit the last.
inline const lv_font_t* fit(char* text, size_t cap, const lv_font_t* const* faces, size_t count, int32_t maxW)
{
    for (size_t i = 0; i < count; ++i) {
        if (textWidth(text, faces[i]) <= maxW) {
            return faces[i];
        }
    }
    const lv_font_t* f   = faces[count - 1];
    size_t           len = std::strlen(text);
    while (len > 0) {
        while (len > 0 && text[len - 1] == ' ') {
            --len;
        }
        if (len + 3 <= cap) {
            text[len]     = '.';
            text[len + 1] = '.';
            text[len + 2] = '\0';
            if (textWidth(text, f) <= maxW) {
                return f;
            }
        }
        --len;
    }
    text[0] = '\0';
    return f;
}

} // namespace WorkoutFmt

#endif // WORKOUT_FORMAT_HPP
