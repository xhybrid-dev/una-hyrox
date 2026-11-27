/**
 ******************************************************************************
 * @file    ProbeState.cpp
 * @brief   See the header.
 ******************************************************************************
 */

#include "ProbeState.hpp"

#include <cstdio>

namespace ClockProbe
{

size_t formatState(const ProbeState& s, char* out, size_t cap)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    const int n = std::snprintf(out, cap, "starts=%lu\nfirst=%lu\n", static_cast<unsigned long>(s.starts),
                                static_cast<unsigned long>(s.firstUtc));
    if (n < 0) {
        out[0] = '\0';
        return 0;
    }
    return static_cast<size_t>(n) < cap ? static_cast<size_t>(n) : cap - 1;
}

bool parseState(const char* text, size_t len, ProbeState& out)
{
    out = ProbeState {};
    if (text == nullptr) {
        return false;
    }
    bool any = false;
    size_t i = 0;
    while (i < len) {
        // One line: key=digits.
        size_t eq = i;
        while (eq < len && text[eq] != '=' && text[eq] != '\n') {
            ++eq;
        }
        const size_t keyLen = eq - i;
        if (eq < len && text[eq] == '=') {
            size_t v = eq + 1;
            uint64_t n = 0;
            bool digits = false;
            while (v < len && text[v] >= '0' && text[v] <= '9') {
                n = n * 10u + static_cast<uint64_t>(text[v] - '0');
                if (n > 0xFFFFFFFFull) {
                    n = 0xFFFFFFFFull;
                }
                digits = true;
                ++v;
            }
            if (digits) {
                if (keyLen == 6 && text[i] == 's' && text[i + 1] == 't') {   // "starts"
                    out.starts = static_cast<uint32_t>(n);
                    any = true;
                } else if (keyLen == 5 && text[i] == 'f' && text[i + 1] == 'i') {   // "first"
                    out.firstUtc = static_cast<uint32_t>(n);
                    any = true;
                }
            }
        }
        while (i < len && text[i] != '\n') {
            ++i;
        }
        ++i;
    }
    return any;
}

} // namespace ClockProbe
