/**
 ******************************************************************************
 * @file    TextFold.hpp
 * @brief   A name (UTF-8) as printable ASCII. Copied from HybridX Trail.
 *
 * The watch's fonts carry ASCII only (32-126), so a name like "Llyn y Fan
 * Fach – Pen y Fan" or "Cwm Llwch, Ŵyl" would show boxes. This folds:
 *
 *   - Latin letters with accents (U+00C0-U+017F) to the plain letter, which
 *     covers Welsh (ŵ, ŷ, â), Scots Gaelic, Irish and most of Europe; ß, æ and
 *     œ become "ss", "ae" and "oe";
 *   - dashes to '-', curly quotes to straight ones, the ellipsis to "...",
 *     the no-break space to a space;
 *   - control characters to a space;
 *   - anything else (an emoji, a malformed byte) to '?'.
 *
 * A sequence cut short at the end of @p in (a name truncated to its buffer
 * mid-character) is dropped. Pure, no allocation; host-tested.
 ******************************************************************************
 */

#ifndef INTERVALS_TEXT_FOLD_HPP
#define INTERVALS_TEXT_FOLD_HPP

#include <cstddef>

namespace Intervals::Text
{

/// @p in (UTF-8, NUL-terminated; nullptr reads as "") folded to printable
/// ASCII in @p out, NUL-terminated and cut to @p cap - 1 characters.
/// Returns the length written.
size_t asciiFold(char* out, size_t cap, const char* in);

} // namespace Intervals::Text

#endif // INTERVALS_TEXT_FOLD_HPP
