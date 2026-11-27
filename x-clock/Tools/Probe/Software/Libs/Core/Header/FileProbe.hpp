/**
 ******************************************************************************
 * @file    FileProbe.hpp
 * @brief   Two questions about the file system, asked without trusting it:
 *          what is this file (probeFile), and what is in this folder (listDir).
 *
 * What Streak's Gate 0 found on the watch (hybridx-streak NOTES) shapes both:
 *   - exist() comes first. IDirectory::open() on a missing path may report
 *     success, so a folder that exist() denies is opened only to see whether
 *     that happens (ListResult::openedWithoutExist), never to read from;
 *   - a failed open() cannot tell "missing" from "refused" from "busy", so the
 *     result keeps each step apart: exists, info, opened, read.
 * Pure of the kernel: it talks to SDK::Interface::IFileSystem, which the host
 * tests fill with an in-memory tree.
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_FILE_PROBE_HPP
#define CLOCK_PROBE_FILE_PROBE_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

namespace ClockProbe
{

struct FileResult {
    bool     exists    = false;   ///< fs.exist()
    bool     infoOk    = false;   ///< fs.objectInfo() answered
    bool     opened    = false;   ///< open for reading succeeded
    bool     readOk    = false;   ///< the read succeeded
    bool     truncated = false;   ///< the file is bigger than the buffer
    uint32_t size      = 0;       ///< bytes, from objectInfo or else from the read
    uint32_t utc       = 0;       ///< the file's stamp, UTC seconds (0 if unknown)
    uint16_t got       = 0;       ///< bytes in the buffer
};

/// Ask about @p path. With @p readContent, also read up to cap - 1 bytes into @p buf (always
/// NUL-terminated). Without it the file is never opened.
FileResult probeFile(SDK::Interface::IFileSystem& fs, const char* path, char* buf, size_t cap, bool readContent);

struct ListEntry {
    char     name[32] = {};   ///< cut to fit; the full name is rarely longer
    bool     isDir    = false;
    uint32_t size     = 0;
    uint32_t utc      = 0;
};

struct ListResult {
    bool     exists            = false;   ///< fs.exist() on the folder
    bool     opened            = false;   ///< the folder opened and was read
    bool     openedWithoutExist = false;  ///< exist() said no, and open() said yes anyway
    bool     full              = false;   ///< stopped at the limit: there may be more
    uint16_t entries           = 0;
    uint16_t files             = 0;
    uint16_t dirs              = 0;
};

using ListVisitor = void (*)(void* ctx, const ListEntry& entry);

/// List @p path (not "." or ".."), calling @p visit for each entry, at most @p maxEntries.
ListResult listDir(SDK::Interface::IFileSystem& fs, const char* path, ListVisitor visit, void* ctx,
                   uint16_t maxEntries = 64);

} // namespace ClockProbe

#endif // CLOCK_PROBE_FILE_PROBE_HPP
