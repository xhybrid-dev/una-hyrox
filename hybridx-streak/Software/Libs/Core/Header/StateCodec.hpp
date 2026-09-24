/**
 ******************************************************************************
 * @file    StateCodec.hpp
 * @brief   The streak's State as JSON, saved crash-safely (PLAN 6.5).
 *
 * The SDK's own JSON classes (JsonStreamWriter into a fixed buffer,
 * JsonStreamReader over coreJSON) and SafeFile's tmp/bak sequence. Records are
 * compact arrays to stay well inside the buffer (a full state is about 3 KB).
 *
 * Decoding trusts nothing: every count is clamped to its array, every enum to
 * its range, and a missing required key fails the whole file, so the loader
 * falls back to ".bak" and then to a fresh start.
 *
 * The same encoding is the private state.json and the public
 * ../SharedData/HybridX/streak.json the glance reads (PLAN 4).
 ******************************************************************************
 */

#ifndef STREAK_STATE_CODEC_HPP
#define STREAK_STATE_CODEC_HPP

#include <cstddef>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "StreakModel.hpp"

namespace Streak::StateCodec
{

constexpr uint16_t kVersion  = 1;
constexpr size_t   kMaxBytes = 6144;

/// JSON for @p s into @p out (NUL-terminated). Returns its length, 0 if it did not fit.
size_t encode(const State& s, char* out, size_t cap);

/// Parse @p json into @p s. False (and @p s untouched) if anything is wrong.
bool decode(const char* json, size_t len, State& s);

/// Encode and save crash-safely. @p scratch is a kMaxBytes work buffer.
bool save(SDK::Interface::IFileSystem& fs, const char* path, const State& s, char* scratch, size_t cap);

/// Where load() found the state.
enum class Source : uint8_t { None, Primary, Backup };

/// Load @p path, falling back to its ".bak". @p s is untouched on None.
Source load(SDK::Interface::IFileSystem& fs, const char* path, State& s, char* scratch, size_t cap);

} // namespace Streak::StateCodec

#endif // STREAK_STATE_CODEC_HPP
