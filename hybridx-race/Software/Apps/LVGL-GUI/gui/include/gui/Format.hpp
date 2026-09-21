/**
 ******************************************************************************
 * @file    Format.hpp
 * @brief   Value formatting shared by the race screens.
 *
 * Every decimal is produced with integer arithmetic, which keeps the output
 * independent of the float formatting in the kernel's C library export table
 * (brief 14.4). A race has no pace and no distance, so the Run app's pace and
 * distance formatters are gone.
 ******************************************************************************
 */

#ifndef FORMAT_HPP
#define FORMAT_HPP

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "SDK/Utils/Utils.hpp"

#include "gui/Strings.hpp"
#include "gui/model/Model.hpp"

namespace Fmt
{

/// Format @p value with @p decimals decimal places, using integer arithmetic
/// only: the kernel's exported snprintf has no guaranteed float support
/// (brief 14.4).
inline void fixed(char* buf, size_t n, float value, int decimals)
{
    if (n == 0) {
        return;
    }

    uint32_t scale = 1;
    for (int i = 0; i < decimals; ++i) {
        scale *= 10;
    }

    const bool negative = value < 0.0f;
    if (negative) {
        value = -value;
    }

    const uint32_t scaled = static_cast<uint32_t>(value * static_cast<float>(scale) + 0.5f);
    const uint32_t whole = scaled / scale;
    const uint32_t frac  = scaled % scale;

    if (decimals > 0) {
        snprintf(buf, n, "%s%u.%0*u", negative ? "-" : "", static_cast<unsigned>(whole),
                 decimals, static_cast<unsigned>(frac));
    } else {
        snprintf(buf, n, "%s%u", negative ? "-" : "", static_cast<unsigned>(whole));
    }
}


/// "h:mm:ss".
inline void hms(char* buf, size_t n, std::time_t sec)
{
    const auto t = SDK::Utils::toHMS(sec);
    snprintf(buf, n, "%u:%02u:%02u", t.h, t.m, t.s);
}

/// "m:ss", or "h:mm" from one hour.
inline void shortTime(char* buf, size_t n, std::time_t sec)
{
    const auto t = SDK::Utils::toHMS(sec);
    if (t.h > 0) {
        snprintf(buf, n, "%u:%02u", t.h, t.m);
    } else {
        snprintf(buf, n, "%u:%02u", t.m, t.s);
    }
}

/// Whole beats per minute, or "---" below the physiological minimum.
inline void heartRate(char* buf, size_t n, float bpm)
{
    if (bpm < App::Display::kMinHR) {
        snprintf(buf, n, "---");
    } else {
        fixed(buf, n, bpm, 0);
    }
}
/// fixed(), then left-padded with zeros to at least @p width characters
/// (the TouchGFX "%05.02f" style used by the interval distance readout).
inline void fixedPadded(char* buf, size_t n, float value, int decimals, size_t width)
{
    if (n == 0) {
        return;
    }
    char tmp[16];
    fixed(tmp, sizeof(tmp), value, decimals);
    size_t len = strlen(tmp);
    size_t pad = len < width ? width - len : 0;
    // Fit pad + text + terminator in n: drop the padding first, then the text.
    if (pad + len + 1 > n) {
        pad = (n > len + 1) ? n - len - 1 : 0;
        len = n - pad - 1;
    }
    memset(buf, '0', pad);
    memcpy(buf + pad, tmp, len);
    buf[pad + len] = '\0';
}

/// Milliseconds to whole seconds, for the race clocks.
inline std::time_t msToSec(uint32_t ms)
{
    return static_cast<std::time_t>(ms / 1000u);
}

} // namespace Fmt

#endif // FORMAT_HPP
