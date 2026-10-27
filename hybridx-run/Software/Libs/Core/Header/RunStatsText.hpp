/**
 ******************************************************************************
 * @file    RunStatsText.hpp
 * @brief   The run detail page's drift as text for the screens.
 *
 * Header-only, because the GUI links none of the Libs sources. Integer
 * arithmetic only (no "%f"). Times and paces are formatted by the GUI's own
 * helpers (Fmt), as every other screen does; only what is new is here.
 ******************************************************************************
 */

#ifndef RUN_RUN_STATS_TEXT_HPP
#define RUN_RUN_STATS_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "RunStats.hpp"

namespace RunVo2::StatsText
{

inline constexpr const char* kNone = "---";

/// Drift as a percentage to one decimal: "4.8%", "-1.2%"; "---" when there is none.
inline size_t drift(char* out, size_t cap, const RunStatsResult& r)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    int n = 0;
    if (r.driftStatus != DriftStatus::Ok) {
        n = std::snprintf(out, cap, "%s", kNone);
    } else {
        const int  x   = r.driftX10;
        const int  mag = x < 0 ? -x : x;
        n = std::snprintf(out, cap, "%s%d.%d%%", x < 0 ? "-" : "", mag / 10, mag % 10);
    }
    return n < 0 ? 0 : (static_cast<size_t>(n) < cap ? static_cast<size_t>(n) : cap - 1);
}

/// Why there is no drift, in at most 16 characters; "" when there is one.
inline const char* driftReason(DriftStatus s)
{
    switch (s) {
        case DriftStatus::Ok:          return "";
        case DriftStatus::TooShort:    return "Needs 30+ min";
        case DriftStatus::NoHeartRate: return "No steady HR";
        case DriftStatus::NotSteady:   return "Pace not steady";
        case DriftStatus::Treadmill:   return "Not on treadmill";
    }
    return "";
}

}  // namespace RunVo2::StatsText

#endif  // RUN_RUN_STATS_TEXT_HPP
