/**
 ******************************************************************************
 * @file    StateCodec.hpp
 * @brief   The counter as text, for the state file. Pure C++, no kernel.
 *
 * The file is plain text so it can be read over USB:
 *
 *     knitting 1
 *     rows=8
 *     set=2
 *     total=16
 *
 * Writing it is integer formatting only, into a caller's buffer. Reading is
 * strict: the version line, then exactly the three keys in that order, each a
 * plain run of digits. Anything else is rejected, and the caller falls back to
 * the backup or to a fresh counter. A value that parses but is out of range is
 * not an error: it is pulled inside the limits, as RowCounter does.
 *
 * Torn writes are not this file's problem: SafeFile swaps the file in whole.
 ******************************************************************************
 */

#ifndef KNITTING_STATE_CODEC_HPP
#define KNITTING_STATE_CODEC_HPP

#include <cstddef>

#include "RowCounter.hpp"

namespace Knitting::StateCodec
{

/// The most bytes an encoded state takes, with its terminator.
constexpr size_t kMaxBytes = 64;

/// Write @p state to @p out as text. Returns the length (without the NUL), or
/// 0 if @p cap is too small.
size_t encode(const CounterState& state, char* out, size_t cap);

/// Read @p len bytes of @p text. False if it is not a state file; @p out is
/// untouched then. The values are clamped to the counter's limits.
bool decode(const char* text, size_t len, CounterState& out);

} // namespace Knitting::StateCodec

#endif // KNITTING_STATE_CODEC_HPP
