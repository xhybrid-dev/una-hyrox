/**
 ******************************************************************************
 * @file    SaveCodec.hpp
 * @brief   A game in progress as text, and back.
 *
 * One "key=value" per line, readable over USB and easy to diff:
 *
 *   tq=1                    the format's version
 *   rng=…  lv=…  xp=…  st=…
 *   pc=…  p0=…  p1=…  p2=…
 *   bag=gel,water,salt,poles,headtorch
 *   stage=…  node=…  hut=…  min=…  wx=…  lap=…
 *   boss=…  cache=…  met=…  beat=…  won=…  lost=…  perf=…
 *
 * decode() is strict: every key must be there and in range, and the whole must
 * make sense (a partner that is a boss, a node that is not a save point, more
 * consumables than the bag holds: all refused). A refused file makes the store
 * fall back to the backup copy, then to a new game. Unknown keys are ignored,
 * so a later version can add some.
 ******************************************************************************
 */

#ifndef TRAILQUEST_SAVE_CODEC_HPP
#define TRAILQUEST_SAVE_CODEC_HPP

#include <cstddef>
#include <cstdint>

#include "RunState.hpp"

namespace TrailQuest::SaveCodec
{

constexpr uint32_t kVersion = 1;

/// The most bytes an encoded save takes, with its terminator.
constexpr size_t kMaxBytes = 384;

/// Write @p s into @p out (NUL-terminated). Returns the length, or 0 if
/// @p cap is too small.
size_t encode(const RunState& s, char* out, size_t cap);

/// Read @p len bytes of @p text into @p out. False (and @p out untouched) if
/// anything is missing, out of range or inconsistent.
bool decode(const char* text, size_t len, RunState& out);

/// True if @p s is a state the game can carry on from (decode's checks).
bool valid(const RunState& s);

} // namespace TrailQuest::SaveCodec

#endif // TRAILQUEST_SAVE_CODEC_HPP
