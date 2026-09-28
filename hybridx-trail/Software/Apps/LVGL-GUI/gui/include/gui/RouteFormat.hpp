/**
 ******************************************************************************
 * @file    RouteFormat.hpp
 * @brief   HybridX Trail's text for routes: names, distances, climb, status.
 *
 * The watch fonts are ASCII at most (assets/gen_assets.py: range 32-126; the
 * big numeric faces digits only), so a route name from a GPX, which is UTF-8,
 * is folded to ASCII (TextFold.hpp: accents dropped, dashes and quotes made
 * plain), and fitted to the room it has by stepping the font down. Integer
 * formatting only, via Fmt::fixed (no %f), per the embedded rules.
 ******************************************************************************
 */

#ifndef ROUTE_FORMAT_HPP
#define ROUTE_FORMAT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "lvgl.h"

#include "TextFold.hpp"
#include "gui/Format.hpp"

namespace RouteFmt
{

/// @p in (UTF-8) as printable ASCII in @p out (Trail::Text::asciiFold).
inline void name(char* out, size_t cap, const char* in)
{
    Trail::Text::asciiFold(out, cap, in);
}

/// The number and its unit apart ("8.25" and "km", "650" and "m"), for the
/// big numeric faces, which carry digits only (assets/gen_assets.py NUMERIC).
inline void distanceParts(char* num, size_t n, const char*& unit, float metres, bool imperial)
{
    if (metres < 0.0f) {
        metres = 0.0f;
    }
    if (!imperial && metres < 1000.0f) {
        std::snprintf(num, n, "%lu", static_cast<unsigned long>(metres + 0.5f));
        unit = "m";
        return;
    }
    const float v = Fmt::distUnits(metres, imperial);
    Fmt::fixed(num, n, v, v < 10.0f ? 2 : 1);
    unit = Fmt::units(imperial);
}

/// "12.4 km", "8.25 km", "650 m" (or miles).
inline void distance(char* buf, size_t n, float metres, bool imperial)
{
    char        num[16];
    const char* unit = "";
    distanceParts(num, sizeof(num), unit, metres, imperial);
    std::snprintf(buf, n, "%.8s %s", num, unit);   // "12345.6 km" at most
}

/// A height or a climb: "180 m", or "590 ft".
inline void height(char* buf, size_t n, float metres, bool imperial)
{
    if (imperial) {
        std::snprintf(buf, n, "%lu ft", static_cast<unsigned long>(metres * 3.28084f + 0.5f));
    } else {
        std::snprintf(buf, n, "%lu m", static_cast<unsigned long>(metres + 0.5f));
    }
}

/// "350 m up" (or feet).
inline void climb(char* buf, size_t n, uint32_t metres, bool imperial)
{
    if (imperial) {
        std::snprintf(buf, n, "%lu ft up", static_cast<unsigned long>(static_cast<float>(metres) * 3.28084f + 0.5f));
    } else {
        std::snprintf(buf, n, "%lu m up", static_cast<unsigned long>(metres));
    }
}

/// The route list's hint line: "12.4 km, 350 m up".
inline void summary(char* buf, size_t n, uint32_t lengthM, uint32_t ascentM, bool imperial)
{
    char d[16];
    char c[16];
    distance(d, sizeof(d), static_cast<float>(lengthM), imperial);
    climb(c, sizeof(c), ascentM, imperial);
    std::snprintf(buf, n, "%.12s, %.12s", d, c);   // fits the 32-byte tips
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

} // namespace RouteFmt

#endif // ROUTE_FORMAT_HPP
