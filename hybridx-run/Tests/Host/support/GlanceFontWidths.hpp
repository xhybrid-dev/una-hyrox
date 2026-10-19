/**
 * Advance widths (px) of printable ASCII, 0x20..0x7E, in the glance fonts the
 * layout uses with words. Copied from X Weigh-in (HybridX Streak's, with
 * SemiBold 25 added). All come from the SDK's generated TouchGFX tables
 * (una-sdk Docs/Tutorials/Buttons/.../generated/fonts/src/Table_Poppins_*_2bpp.cpp,
 * SDK a7a995a: each glyph's advance), which list the same Poppins faces as
 * GlanceFont_t. Kerning is ignored, so a sum is a slight over-estimate: safe
 * for "does it fit".
 *
 * The same tables show POPPINS_MEDIUM_10 holds only '0'..'9' and '?': any other
 * character draws as '?' (HybridX Streak NOTES, "S4.2 The glance's third line").
 */

#ifndef RUN_TESTS_GLANCE_FONT_WIDTHS_HPP
#define RUN_TESTS_GLANCE_FONT_WIDTHS_HPP

#include <cstdint>

namespace GlanceFontWidths
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

/// Poppins Medium 18.
inline constexpr uint8_t kMedium18[95] = {
     5,  6,  6, 16, 12, 14, 14,  3,  9,  9,  9, 13,  4, 11,  4,  9,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12,  4,  5, 11, 14, 11, 10,
    18, 13, 11, 14, 13,  9,  9, 14, 13,  5, 10, 11,  8, 16, 13, 14,
    11, 14, 11, 11, 10, 12, 12, 18, 12, 11, 10,  9, 13,  9, 12, 14,
     5, 12, 12, 11, 12, 11,  6, 12, 12,  5,  5, 10,  5, 19, 12, 11,
    12, 12,  7, 10,  7, 12, 10, 15,  9, 10,  8,  9,  6,  9, 10,
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

/// Poppins SemiBold 25.
inline constexpr uint8_t kSemiBold25[95] = {
     6,  9,  9, 22, 16, 21, 19,  5, 12, 12, 13, 17,  6, 15,  7, 12,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17,  7,  8, 15, 18, 14, 13,
    26, 18, 16, 19, 18, 13, 13, 19, 18,  7, 14, 17, 11, 22, 18, 20,
    15, 20, 16, 15, 14, 17, 18, 26, 17, 16, 14, 12, 19, 12, 17, 20,
     7, 17, 17, 15, 17, 15,  9, 17, 17,  7,  7, 15,  7, 26, 17, 16,
    17, 17, 10, 14, 10, 17, 15, 21, 14, 15, 12, 13,  8, 13, 15,
};

} // namespace GlanceFontWidths

#endif // RUN_TESTS_GLANCE_FONT_WIDTHS_HPP
