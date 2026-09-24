/**
 ******************************************************************************
 * @file    SafeFile.hpp
 * @brief   Crash-safe whole-file saves, the SDK's own way.
 *
 * The sequence is una-sdk RecordingMarker::write() (RecordingMarker.cpp:70-110),
 * itself a copy of Settings::ManagerBase: write "<path>.tmp", flush, close;
 * then, if a primary exists, remove any stale "<path>.bak", rename the
 * primary to ".bak", and rename the temp into place (FatFs rename refuses an
 * existing destination, NOTES S0.5). A crash at any point leaves either the new
 * primary or the previous ".bak" intact, and readers fall back to ".bak".
 ******************************************************************************
 */

#ifndef STREAK_SAFE_FILE_HPP
#define STREAK_SAFE_FILE_HPP

#include <cstddef>

#include "SDK/Interfaces/IFileSystem.hpp"

namespace Streak::SafeFile
{

/// Save @p len bytes as @p path. False if any step failed (the old copy stands).
bool write(SDK::Interface::IFileSystem& fs, const char* path, const char* data, size_t len);

/// Read up to @p cap - 1 bytes of @p path (".bak" when @p backup) into @p out,
/// NUL-terminated. Returns the length, or 0 if absent or unreadable.
size_t read(SDK::Interface::IFileSystem& fs, const char* path, bool backup, char* out, size_t cap);

} // namespace Streak::SafeFile

#endif // STREAK_SAFE_FILE_HPP
