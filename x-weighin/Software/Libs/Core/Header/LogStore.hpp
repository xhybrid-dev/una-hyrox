/**
 ******************************************************************************
 * @file    LogStore.hpp
 * @brief   The weigh-in log on the watch: append, read back, and the copy in
 *          SharedData that survives an uninstall.
 *
 * Two files, the same content:
 *   - weighins.csv in the app's own folder (Apps/XWeighIn/), the one the app
 *     reads and the one to copy off over USB;
 *   - ../SharedData/HybridX/weighins.csv (Apps/SharedData/HybridX/), the copy.
 *     Uninstalling an app deletes its folder (una-sdk Docs/app-config-fields.md
 *     7.4); SharedData is the one folder no app owns (hybridx-streak NOTES,
 *     Gate 0), and Streak and Run already keep their public files there.
 *
 * Append-only: a weigh-in is never rewritten in place, so a reset mid-write can
 * at worst leave a torn last line, which the reader skips and the next append
 * steps over. No heap: fixed buffers, streamed in chunks.
 *
 * On SDK::Interface::IFileSystem, as HybridX Streak's SafeFile is, so the host
 * tests run it against an in-memory tree.
 ******************************************************************************
 */

#ifndef XWEIGHIN_LOG_STORE_HPP
#define XWEIGHIN_LOG_STORE_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "WeighLog.hpp"

namespace XWeighIn::LogStore
{

constexpr const char* kLogFile    = "weighins.csv";
constexpr const char* kSharedDir  = "../SharedData/HybridX";
constexpr const char* kSharedFile = "../SharedData/HybridX/weighins.csv";

struct ScanResult {
    bool     opened   = false;   ///< The file existed and opened.
    uint32_t entries  = 0;       ///< Valid entries passed to the visitor.
    uint32_t rejected = 0;       ///< Non-blank lines, other than the header, that did not parse.
};

/**
 * @brief   Append one weigh-in to @p path, creating the file with its header
 *          if it does not exist. If the file's last byte is not a newline (a
 *          torn earlier write, or a hand edit), a newline goes first.
 * @return  false if the entry is invalid or any step failed.
 */
bool append(SDK::Interface::IFileSystem& fs, const char* path, const Entry& e);

/// Append to the app's own log, then to the SharedData copy (creating its
/// folder). Returns whether the app's own log took it; a failed copy is
/// mended by the next reconcile().
bool appendBoth(SDK::Interface::IFileSystem& fs, const Entry& e);

/// Stream @p path, passing each valid entry to @p visitor.
ScanResult scan(SDK::Interface::IFileSystem& fs, const char* path, EntryVisitor& visitor);

/**
 * @brief   Copy @p from to @p to: into "<to>.tmp" first, then swapped in, so a
 *          failure part-way leaves the old @p to untouched. (FatFs rename
 *          refuses an existing destination, so the old one is removed first.)
 */
bool copyFile(SDK::Interface::IFileSystem& fs, const char* from, const char* to);

enum class Reconcile : uint8_t {
    Nothing,     ///< Both present and the same size, or neither present.
    Restored,    ///< The app's own log was missing: restored from the copy.
    Refreshed,   ///< The copy was missing or differed: rewritten from our own log.
    Failed,      ///< A copy was needed and failed.
};

/**
 * @brief   Bring the two files back in step, once when the app starts.
 *
 * The app's own log is the source of truth whenever it exists. Only when it is
 * missing (the app was reinstalled) does the copy flow the other way. So to
 * clear the history by hand, delete both files.
 */
Reconcile reconcile(SDK::Interface::IFileSystem& fs,
                    const char* own       = kLogFile,
                    const char* shared    = kSharedFile,
                    const char* sharedDir = kSharedDir);

}  // namespace XWeighIn::LogStore

#endif  // XWEIGHIN_LOG_STORE_HPP
