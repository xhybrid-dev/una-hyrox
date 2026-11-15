/**
 ******************************************************************************
 * @file    Vo2Log.hpp
 * @brief   The tuning log's text: one CSV per run, one row per window.
 *
 * Written so the provisional limits in Vo2Config.hpp can be tuned from real
 * runs (docs/NOTES.md, R5). Integers only (no %f); decimals printed from x10
 * values. Layout:
 *   # HybridX Run VO2 log 1, start 2026-10-14T07:30:00Z (1791963000), max_hr 184 (age), rest_hr 52 (watch), age 40
 *   window,active_s,good_s,outcome,speed_m_min,spread_pct,grade_pct,hr_avg,hr_min,hr_max,cost
 *   1,60,58,warmup,201.4,2.1,0.4,148.2,141,153,43.8
 *   ...
 *   # result not_enough (profile ok), vo2max 0.0, windows 3, too_easy 0, above_max 0, implausible 0, max_hr 184 (age), rest_hr 52 (watch), run_max_hr 171
 * Each function returns the length written (a full line with '\n'), or 0 if
 * it did not fit (buf then holds "").
 ******************************************************************************
 */

#ifndef RUN_VO2_LOG_HPP
#define RUN_VO2_LOG_HPP

#include <cstddef>
#include <cstdint>
#include <ctime>

#include "Vo2Profile.hpp"
#include "Vo2Run.hpp"

namespace RunVo2::Log
{

/// Longest line any of these writes, for sizing a buffer.
inline constexpr size_t kMaxLineBytes = 256;

const char* outcomeName(WindowOutcome o);
const char* statusName(RunStatus s);

size_t contextLine(std::time_t startUtc, const Profile& profile, char* buf, size_t cap);
size_t headerLine(char* buf, size_t cap);
size_t windowLine(const WindowLog& w, char* buf, size_t cap);
size_t resultLine(const RunResult& r, const Profile& profile, uint8_t runMaxHr, char* buf, size_t cap);
/// P3, measured first: what the guards would make of the run, and the shown value with the age
/// limit (Config::kMaxRunAgeDays). A comment line, after the result line.
size_t whatIfLine(const RunResult& r, uint16_t shownX10, uint16_t shownWithinX10, char* buf, size_t cap);

}  // namespace RunVo2::Log

#endif  // RUN_VO2_LOG_HPP
