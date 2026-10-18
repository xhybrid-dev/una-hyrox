/**
 ******************************************************************************
 * @file    TextFit.hpp
 * @brief   What fits on the round face: the visible width at a row, how many
 *          characters of the game's font fit there, and word-wrapping into
 *          fixed buffers.
 *
 * Header-only (the GUI lays text out with it; the host tests prove every
 * string the game shows fits where it is drawn).
 *
 * The face is a 240 px circle, so a row's visible width is the chord at that
 * height: about 210 px a quarter of the way down, 150 px near the top. All the
 * player's text is Press Start 2P at 16 px, which is monospace (every glyph 16
 * px wide), so fitting is arithmetic: (chord - margins) / 16 characters.
 ******************************************************************************
 */

#ifndef TRAILQUEST_TEXT_FIT_HPP
#define TRAILQUEST_TEXT_FIT_HPP

#include <cstddef>
#include <cstdint>

namespace TrailQuest::TextFit
{

constexpr int32_t kScreen = 240;
constexpr int32_t kRadius = kScreen / 2;
constexpr int32_t kGlyph  = 16;   ///< Press Start 2P at 16 px: each glyph 16 x 16.
constexpr int32_t kMargin = 4;    ///< Kept clear of the edge on each side, px.

/// The most characters any row holds ((240 - 2 x 4) / 16), and the most lines a
/// wrapped block keeps.
constexpr uint8_t kMaxCols  = 14;
constexpr uint8_t kMaxLines = 6;

/// Integer square root (floor).
inline uint32_t isqrt(uint32_t v)
{
    uint32_t r = 0;
    uint32_t bit = 1u << 30;
    while (bit > v) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

/// The visible width of pixel row @p y (0 at the top), px. Measured at the
/// row's centre: the chord is sqrt((2R)^2 - (2y + 1 - 2R)^2), all doubled so
/// it stays in integers.
inline int32_t chordAt(int32_t y)
{
    if (y < 0 || y >= kScreen) {
        return 0;
    }
    const int32_t d  = 2 * y + 1 - kScreen;
    const int32_t dd = d * d;
    const int32_t rr = kScreen * kScreen;
    return dd >= rr ? 0 : static_cast<int32_t>(isqrt(static_cast<uint32_t>(rr - dd)));
}

/// The narrowest visible width over rows [@p top, @p top + @p height).
inline int32_t widthOver(int32_t top, int32_t height)
{
    // The chord narrows away from the middle, so the band's narrowest row is
    // one of its two ends.
    const int32_t a = chordAt(top);
    const int32_t b = chordAt(top + height - 1);
    return a < b ? a : b;
}

/// Characters that fit on a 16 px line whose top is at @p top, centred.
inline uint8_t colsAt(int32_t top)
{
    const int32_t w = widthOver(top, kGlyph) - 2 * kMargin;
    if (w <= 0) {
        return 0;
    }
    const int32_t n = w / kGlyph;
    return static_cast<uint8_t>(n > kMaxCols ? kMaxCols : n);
}

/// Characters that fit in a block of @p lines lines @p pitch px apart, its
/// first line's top at @p top.
inline uint8_t colsOver(int32_t top, uint8_t lines, int32_t pitch)
{
    uint8_t n = kMaxCols;
    for (uint8_t i = 0; i < lines; ++i) {
        const uint8_t c = colsAt(top + i * pitch);
        if (c < n) {
            n = c;
        }
    }
    return n;
}

/// Characters that fit on one side of a row split down the middle (a label in
/// a corner by its button's arc), leaving @p gap px clear at the centre.
inline uint8_t halfColsAt(int32_t top, int32_t gap = 8)
{
    const int32_t w = widthOver(top, kGlyph) / 2 - kMargin - gap / 2;
    if (w <= 0) {
        return 0;
    }
    const int32_t n = w / kGlyph;
    return static_cast<uint8_t>(n > kMaxCols ? kMaxCols : n);
}

/// A block of wrapped text, in fixed buffers.
struct Lines {
    char    text[kMaxLines][kMaxCols + 1] = {};
    uint8_t count = 0;
    bool    fits  = true;   ///< False if words were cut or lines dropped.

    void clear()
    {
        for (auto& l : text) {
            l[0] = '\0';
        }
        count = 0;
        fits  = true;
    }
};

/// Append one line (cut to @p cols). False if the block is full.
///
/// Not inlined: inlined against a short string literal, GCC's -Warray-bounds
/// reports reads past its end on paths the loop never takes (it stops at the
/// terminator first).
[[gnu::noinline]] inline bool addLine(Lines& out, const char* s, uint8_t cols)
{
    if (out.count >= kMaxLines) {
        out.fits = false;
        return false;
    }
    if (cols > kMaxCols) {
        cols = kMaxCols;
    }
    char*  d = out.text[out.count];
    size_t n = 0;
    bool   ended = s == nullptr;
    for (; !ended && n < cols; ++n) {
        if (s[n] == '\0') {
            ended = true;
            break;
        }
        d[n] = s[n];
    }
    d[n] = '\0';
    if (!ended && s[n] != '\0') {   // reached only after cols characters: s[n] is in the string
        out.fits = false;
    }
    ++out.count;
    return true;
}

/**
 * @brief   Word-wrap @p s into lines of at most @p cols characters, appended to
 *          @p out, at most @p maxLines of them (and kMaxLines in all). Spaces
 *          separate words; a word longer than a line is cut across lines (and
 *          @p out.fits goes false, as it does if lines run out). A '\n' forces
 *          a new line. Not inlined, for the reason addLine() gives.
 */
[[gnu::noinline]] inline void wrap(Lines& out, const char* s, uint8_t cols, uint8_t maxLines = kMaxLines)
{
    if (s == nullptr) {
        return;
    }
    if (cols == 0) {
        out.fits = false;
        return;
    }
    if (cols > kMaxCols) {
        cols = kMaxCols;
    }
    const uint8_t limit = static_cast<uint8_t>(out.count + maxLines > kMaxLines ? kMaxLines : out.count + maxLines);
    char   line[kMaxCols + 1] = {};
    size_t len = 0;
    size_t i   = 0;

    auto flush = [&]() -> bool {
        if (out.count >= limit) {
            out.fits = false;
            return false;
        }
        line[len] = '\0';
        addLine(out, line, cols);
        len = 0;
        return true;
    };

    while (s[i] != '\0') {
        if (s[i] == ' ') {
            ++i;
            continue;
        }
        if (s[i] == '\n') {
            if (!flush()) {
                return;
            }
            ++i;
            continue;
        }
        size_t w = 0;
        while (s[i + w] != '\0' && s[i + w] != ' ' && s[i + w] != '\n') {
            ++w;
        }
        const size_t need = len == 0 ? w : len + 1 + w;
        if (need <= cols) {
            if (len != 0) {
                line[len++] = ' ';
            }
            for (size_t k = 0; k < w; ++k) {
                line[len++] = s[i + k];
            }
            i += w;
        } else if (len != 0) {
            if (!flush()) {
                return;
            }
        } else {
            // A word longer than a whole line: cut it.
            for (size_t k = 0; k < cols; ++k) {
                line[len++] = s[i + k];
            }
            i += cols;
            out.fits = false;
            if (!flush()) {
                return;
            }
        }
    }
    if (len != 0) {
        flush();
    }
}

/// Length of a NUL-terminated string, at most @p cap.
inline size_t length(const char* s, size_t cap = 64)
{
    size_t n = 0;
    while (s != nullptr && n < cap && s[n] != '\0') {
        ++n;
    }
    return n;
}

} // namespace TrailQuest::TextFit

#endif // TRAILQUEST_TEXT_FIT_HPP
