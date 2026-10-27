/**
 ******************************************************************************
 * @file    FormText.hpp
 * @brief   Cadence and step length as text for the screens.
 *
 * Header-only, because the GUI links none of the Libs sources. Integer
 * arithmetic only (no "%f"). The number and its unit are separate strings: the
 * screens draw the unit smaller. A value of 0 means no data and reads "---", as
 * the other fields do.
 ******************************************************************************
 */

#ifndef RUN_FORM_TEXT_HPP
#define RUN_FORM_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace RunVo2::FormText
{

inline constexpr const char* kNone         = "---";
inline constexpr const char* kCadenceUnit  = "spm";   ///< steps per minute
inline constexpr const char* kStepUnit     = "m";

/// Steps per minute, whole ("172"). @p spmX10 is tenths of a step per minute; 0 = none.
inline size_t cadence(char* out, size_t cap, uint16_t spmX10)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    const int n = spmX10 == 0 ? std::snprintf(out, cap, "%s", kNone)
                              : std::snprintf(out, cap, "%u", static_cast<unsigned>((spmX10 + 5u) / 10u));
    return n < 0 ? 0 : (static_cast<size_t>(n) < cap ? static_cast<size_t>(n) : cap - 1);
}

/// Step length in metres to the centimetre ("1.08"). @p mm is millimetres; 0 = none.
inline size_t stepLength(char* out, size_t cap, uint16_t mm)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    int n = 0;
    if (mm == 0) {
        n = std::snprintf(out, cap, "%s", kNone);
    } else {
        const unsigned cm = (static_cast<unsigned>(mm) + 5u) / 10u;
        n = std::snprintf(out, cap, "%u.%02u", cm / 100u, cm % 100u);
    }
    return n < 0 ? 0 : (static_cast<size_t>(n) < cap ? static_cast<size_t>(n) : cap - 1);
}

}  // namespace RunVo2::FormText

#endif  // RUN_FORM_TEXT_HPP
