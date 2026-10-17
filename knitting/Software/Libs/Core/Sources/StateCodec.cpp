/**
 ******************************************************************************
 * @file    StateCodec.cpp
 * @brief   The counter as text, for the state file (see the header).
 ******************************************************************************
 */

#include "StateCodec.hpp"

#include <cstdint>

namespace Knitting::StateCodec
{

namespace
{

constexpr char kHeader[] = "knitting 1\n";

/// Append @p text (NUL-terminated) at @p pos. False if it does not fit.
bool putText(char* out, size_t cap, size_t& pos, const char* text)
{
    while (*text != '\0') {
        if (pos + 1 >= cap) {
            return false;
        }
        out[pos++] = *text++;
    }
    return true;
}

/// Append @p v in decimal, then a newline.
bool putNumberLine(char* out, size_t cap, size_t& pos, uint32_t v)
{
    char   digits[11];
    size_t n = 0;
    do {
        digits[n++] = static_cast<char>('0' + (v % 10));
        v /= 10;
    } while (v != 0 && n < sizeof(digits));
    if (pos + n + 1 >= cap) {
        return false;
    }
    while (n > 0) {
        out[pos++] = digits[--n];
    }
    out[pos++] = '\n';
    return true;
}

/// If the text at @p pos starts with @p prefix, step over it.
bool take(const char* text, size_t len, size_t& pos, const char* prefix)
{
    size_t p = pos;
    while (*prefix != '\0') {
        if (p >= len || text[p] != *prefix) {
            return false;
        }
        ++p;
        ++prefix;
    }
    pos = p;
    return true;
}

/// Read a run of digits then a newline. Rejects an empty run. A run too long
/// for 32 bits reads as the largest value, which the caller then clamps.
bool takeNumberLine(const char* text, size_t len, size_t& pos, uint32_t& value)
{
    size_t   p      = pos;
    uint64_t v      = 0;
    size_t   digits = 0;
    while (p < len && text[p] >= '0' && text[p] <= '9') {
        v = v * 10 + static_cast<uint64_t>(text[p] - '0');
        if (v > UINT32_MAX) {
            v = UINT32_MAX;
        }
        ++p;
        ++digits;
    }
    if (digits == 0 || p >= len || text[p] != '\n') {
        return false;
    }
    value = static_cast<uint32_t>(v);
    pos   = p + 1;
    return true;
}

} // namespace

size_t encode(const CounterState& state, char* out, size_t cap)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    size_t pos = 0;
    if (!putText(out, cap, pos, kHeader)
        || !putText(out, cap, pos, "rows=") || !putNumberLine(out, cap, pos, state.rows)
        || !putText(out, cap, pos, "set=")   || !putNumberLine(out, cap, pos, state.set)
        || !putText(out, cap, pos, "total=") || !putNumberLine(out, cap, pos, state.total)) {
        out[0] = '\0';
        return 0;
    }
    out[pos] = '\0';
    return pos;
}

bool decode(const char* text, size_t len, CounterState& out)
{
    if (text == nullptr) {
        return false;
    }
    size_t   pos = 0;
    uint32_t rows = 0;
    uint32_t set = 0;
    uint32_t total = 0;
    if (!take(text, len, pos, kHeader)
        || !take(text, len, pos, "rows=")  || !takeNumberLine(text, len, pos, rows)
        || !take(text, len, pos, "set=")   || !takeNumberLine(text, len, pos, set)
        || !take(text, len, pos, "total=") || !takeNumberLine(text, len, pos, total)
        || pos != len) {
        return false;
    }
    CounterState s;
    s.rows  = rows;
    s.set   = set;
    s.total = total;
    out = RowCounter(s).state();     // RowCounter clamps what it is given
    return true;
}

} // namespace Knitting::StateCodec
