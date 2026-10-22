/**
 ******************************************************************************
 * @file    SessionText.hpp
 * @brief   Text for the session screens: clocks, heart rate, recovery and the
 *          relative temperature. Pure C++, header-only, integer only. Every
 *          writer fills the caller's buffer, never past it, always terminates
 *          it (when cap > 0), and returns the characters written.
 ******************************************************************************
 */

#ifndef SAUNA_SESSION_TEXT_HPP
#define SAUNA_SESSION_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "FixedPoint.hpp"
#include "HrRecovery.hpp"

namespace Sauna {

namespace detail {
inline size_t copyOut(char *out, size_t cap, const char *text)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    size_t n = 0;
    while (text[n] != '\0' && n + 1 < cap) {
        out[n] = text[n];
        ++n;
    }
    out[n] = '\0';
    return n;
}
} // namespace detail

/// "mm:ss", or "h:mm:ss" from an hour on; capped at 99:59:59.
inline size_t formatClock(char *out, size_t cap, uint32_t ms)
{
    uint32_t s = ms / 1000u;
    constexpr uint32_t kMaxS = 99u * 3600u + 59u * 60u + 59u;
    if (s > kMaxS) {
        s = kMaxS;
    }
    const unsigned long h   = s / 3600u;
    const unsigned long m   = (s / 60u) % 60u;
    const unsigned long sec = s % 60u;
    char tmp[16];
    if (h > 0) {
        std::snprintf(tmp, sizeof(tmp), "%lu:%02lu:%02lu", h, m, sec);
    } else {
        std::snprintf(tmp, sizeof(tmp), "%02lu:%02lu", m, sec);
    }
    return detail::copyOut(out, cap, tmp);
}

/// Heart rate: the number, or "--" when there is none (0).
inline size_t formatBpm(char *out, size_t cap, uint16_t bpm)
{
    if (bpm == 0) {
        return detail::copyOut(out, cap, "--");
    }
    char tmp[8];
    std::snprintf(tmp, sizeof(tmp), "%u", static_cast<unsigned>(bpm));
    return detail::copyOut(out, cap, tmp);
}

/// Recovery as the heart rate's change: "-23" when it fell by 23, "+4" when it
/// rose, "0", and ".." while still waiting, "--" with no result.
inline size_t formatRecovery(char *out, size_t cap, const Recovery &r)
{
    switch (r.state) {
    case Recovery::State::Pending:
        return detail::copyOut(out, cap, "..");
    case Recovery::State::Done: {
        char tmp[12];
        if (r.drop > 0) {
            std::snprintf(tmp, sizeof(tmp), "-%d", static_cast<int>(r.drop));
        } else if (r.drop < 0) {
            std::snprintf(tmp, sizeof(tmp), "+%d", -static_cast<int>(r.drop));
        } else {
            std::snprintf(tmp, sizeof(tmp), "0");
        }
        return detail::copyOut(out, cap, tmp);
    }
    default:
        return detail::copyOut(out, cap, "--");
    }
}

/// A change in the watch's temperature, one decimal and always signed:
/// "+3.2", "-0.5", "0.0"; "--" for no value. (A hint, not a measurement.)
inline size_t formatRelTemp(char *out, size_t cap, int32_t deltaX100)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    char num[24];
    formatX100(num, sizeof(num), deltaX100, 1);
    if (deltaX100 == kX100Invalid || num[0] == '-' || (num[0] == '0' && num[1] == '.' && num[2] == '0')) {
        return detail::copyOut(out, cap, num);
    }
    char tmp[26];
    tmp[0] = '+';
    size_t i = 0;
    while (num[i] != '\0' && i + 2 < sizeof(tmp)) {
        tmp[i + 1] = num[i];
        ++i;
    }
    tmp[i + 1] = '\0';
    return detail::copyOut(out, cap, tmp);
}

} // namespace Sauna

#endif // SAUNA_SESSION_TEXT_HPP
