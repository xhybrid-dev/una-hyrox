/**
 ******************************************************************************
 * @file    GlanceFontWidths.hpp
 * @brief   Advance widths (px) of printable ASCII, 0x20..0x7E, in the two glance
 *          faces X Training's words use, so the layout can pick the longest
 *          wording that fits the band.
 *
 * Copied from the SDK's generated TouchGFX tables (una-sdk Docs/Tutorials/Buttons/
 * .../generated/fonts/src/Table_Poppins_*_2bpp.cpp), which list the same Poppins
 * faces as GlanceFont_t, by HybridX Streak's glance (its NOTES, S4.2). Kerning is
 * ignored, so a sum is a slight over-estimate: safe for "does it fit". The 10 point
 * face holds digits alone and is never used for words.
 ******************************************************************************
 */

#ifndef TRAINING_GLANCE_FONT_WIDTHS_HPP
#define TRAINING_GLANCE_FONT_WIDTHS_HPP

#include <cstddef>
#include <cstdint>

namespace TrainingGlance
{
namespace Fonts
{

/// Poppins Regular 18.
inline constexpr uint8_t kRegular18[95] = {
     5,  5,  5, 15, 11, 14, 13,  3,  8,  8,  9, 12,  4, 10,  4,  9,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11,  4,  5, 10, 13, 10,  9,
    18, 12, 11, 14, 13,  9,  9, 14, 12,  4, 10, 11,  8, 16, 13, 14,
    10, 14, 11, 11, 10, 12, 12, 18, 11, 11, 10,  8, 12,  8, 11, 13,
     5, 12, 12, 11, 12, 11,  6, 12, 12,  4,  4,  9,  4, 19, 12, 12,
    12, 12,  7,  9,  7, 12, 10, 15,  9, 10,  8,  8,  5,  8,  9,
};

/// Poppins SemiBold 20.
inline constexpr uint8_t kSemiBold20[95] = {
     5,  7,  7, 18, 13, 17, 16,  4, 10, 10, 10, 13,  5, 12,  5, 10,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13,  5,  6, 12, 15, 11, 11,
    21, 14, 13, 15, 14, 11, 11, 15, 14,  6, 11, 13,  9, 18, 15, 16,
    12, 16, 13, 12, 12, 14, 14, 20, 14, 13, 12, 10, 15, 10, 14, 16,
     5, 14, 14, 12, 14, 12,  7, 14, 13,  6,  6, 12,  6, 21, 13, 13,
    14, 14,  8, 11,  8, 13, 12, 17, 11, 12, 10, 10,  6, 10, 12,
};

/// The width of @p s in @p table (a character outside 0x20..0x7E counts as the widest, 21).
inline int width(const char* s, const uint8_t (&table)[95])
{
    int w = 0;
    for (; s != nullptr && *s != '\0'; ++s) {
        const unsigned char c = static_cast<unsigned char>(*s);
        w += (c >= 0x20 && c <= 0x7E) ? table[c - 0x20] : 21;
    }
    return w;
}

}  // namespace Fonts
}  // namespace TrainingGlance

#endif  // TRAINING_GLANCE_FONT_WIDTHS_HPP
