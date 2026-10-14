/**
 ******************************************************************************
 * @file    Vo2Text.hpp
 * @brief   What the screens say about VO2max (British English, no %f).
 ******************************************************************************
 */

#ifndef RUN_VO2_TEXT_HPP
#define RUN_VO2_TEXT_HPP

#include <cstddef>
#include <cstdint>

#include "Vo2Run.hpp"

namespace RunVo2::Text
{

/// "52.3" from 523; "---" for 0. Returns buf.
const char* formatX10(uint16_t x10, char* buf, size_t cap);

/// The one-line reason a run has no estimate, or "" when it has one.
const char* reason(const RunResult& r);

/// The same reason for a profile alone (the VO2max page), "" when complete.
const char* profileReason(ProfileStatus s);

/// "12 Oct"; "" for an impossible date. Returns buf.
const char* shortDate(uint8_t day, uint8_t month, char* buf, size_t cap);

/// "+0.8 since last run", "-1.2 since last run", "Same as last run", or ""
/// with no earlier figure. Returns buf.
const char* trend(uint16_t nowX10, uint16_t beforeX10, char* buf, size_t cap);

/// Where max and resting HR come from, for "Max HR 184 (age)": "age",
/// "runs", "entered", "watch"; "" when unknown.
const char* maxSourceLabel(MaxHrSource s);
const char* restSourceLabel(RestHrSource s);

}  // namespace RunVo2::Text

#endif  // RUN_VO2_TEXT_HPP
