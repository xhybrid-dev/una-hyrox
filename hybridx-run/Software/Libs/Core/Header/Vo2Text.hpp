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

}  // namespace RunVo2::Text

#endif  // RUN_VO2_TEXT_HPP
