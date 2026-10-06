/**
 ******************************************************************************
 * @file    TextFold.cpp
 * @brief   UTF-8 to printable ASCII (see TextFold.hpp). Copied from HybridX Trail.
 ******************************************************************************
 */

#include "TextFold.hpp"

#include <cstdint>

namespace Intervals::Text
{

namespace
{

/// U+00C0-U+017F, each to its plain letter (from Unicode's decompositions;
/// letters with none, such as Ø, Ł and ı, by hand). The two-letter ones
/// (ß, æ, œ) are handled in fold().
constexpr char kLatin[] =
    "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTs"   // U+00C0
    "aaaaaaaceeeeiiiidnooooo/ouuuuyty"   // U+00E0
    "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGg"   // U+0100
    "GgGgHhHhIiIiIiIiIiIiJjKkkLlLlLlL"   // U+0120
    "lLlNnNnNnnNnOoOoOoOoRrRrRrSsSsSs"   // U+0140
    "SsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs";  // U+0160
static_assert(sizeof(kLatin) - 1 == 0x180 - 0xC0, "one letter per code point");

/// The ASCII for code point @p cp (never empty).
const char* fold(uint32_t cp, char (&one)[2])
{
    one[1] = '\0';
    if (cp < 0x20 || cp == 0x7F || (cp >= 0x80 && cp < 0xA0)) {
        return " ";   // control characters
    }
    if (cp < 0x7F) {
        one[0] = static_cast<char>(cp);
        return one;
    }
    switch (cp) {
        case 0x00A0: return " ";     // no-break space
        case 0x00C6: return "AE";
        case 0x00DF: return "ss";
        case 0x00E6: return "ae";
        case 0x0152: return "OE";
        case 0x0153: return "oe";
        case 0x2026: return "...";
        default: break;
    }
    if (cp >= 0xC0 && cp < 0x180) {
        one[0] = kLatin[cp - 0xC0];
        return one;
    }
    if (cp >= 0x2010 && cp <= 0x2015) {
        return "-";   // hyphens and dashes
    }
    if ((cp >= 0x2018 && cp <= 0x201B) || cp == 0x2032) {
        return "'";
    }
    if ((cp >= 0x201C && cp <= 0x201F) || cp == 0x2033) {
        return "\"";
    }
    return "?";
}

} // namespace

size_t asciiFold(char* out, size_t cap, const char* in)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    size_t o = 0;
    size_t i = 0;
    while (in != nullptr && in[i] != '\0') {
        const auto lead = static_cast<uint8_t>(in[i]);
        uint32_t   cp   = 0;
        size_t     more = 0;
        if (lead < 0x80) {
            cp = lead;
        } else if ((lead & 0xE0) == 0xC0) {
            cp   = lead & 0x1Fu;
            more = 1;
        } else if ((lead & 0xF0) == 0xE0) {
            cp   = lead & 0x0Fu;
            more = 2;
        } else if ((lead & 0xF8) == 0xF0) {
            cp   = lead & 0x07u;
            more = 3;
        } else {
            cp = 0xFFFD;   // a stray continuation byte, or not UTF-8
        }
        ++i;

        bool truncated = false;
        bool malformed = false;
        for (size_t k = 0; k < more; ++k) {
            const auto c = static_cast<uint8_t>(in[i]);
            if (c == 0) {
                truncated = true;
                break;
            }
            if ((c & 0xC0) != 0x80) {
                malformed = true;   // leave this byte to start the next character
                break;
            }
            cp = (cp << 6) | (c & 0x3Fu);
            ++i;
        }
        if (truncated) {
            break;
        }
        if (malformed) {
            cp = 0xFFFD;
        }

        char        one[2];
        const char* text = fold(cp, one);
        for (size_t k = 0; text[k] != '\0'; ++k) {
            if (o + 1 >= cap) {
                out[o] = '\0';
                return o;
            }
            out[o++] = text[k];
        }
    }
    out[o] = '\0';
    return o;
}

} // namespace Intervals::Text
