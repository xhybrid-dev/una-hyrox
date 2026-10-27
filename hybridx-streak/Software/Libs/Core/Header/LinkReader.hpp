/**
 ******************************************************************************
 * @file    LinkReader.hpp
 * @brief   Reads the activity summaries the HybridX apps leave in the shared
 *          folder (hybridx-link/docs/ACTIVITY_LINK.md).
 *
 * On a real watch Streak cannot read other apps' Activity folders (NOTES, Gate
 * 0), but it can open the fixed file names in HybridXLink::kSlotPaths: each
 * HybridX activity app leaves its last 16 activities in its own file. This
 * reads every file that exists (the primary if it decodes, else its .bak),
 * keeps the entries that fall in the scan window and that @p seen has not
 * counted, and adds them to the list the scanner returned, oldest first.
 *
 * It never lists the shared folder: a missing file is simply a slot nobody has
 * used. A file that does not decode, and any entry out of range, is skipped.
 *
 * The same activity can turn up twice (the scanner reads the .fit in the
 * simulator, and the app's summary has it too): both are added with the same
 * key, and StreakModel::update counts the first and skips the second.
 *
 * Holds a 1 KB buffer and one decoded file, so place it at start-up like the
 * scanner (it is far larger than the service stack).
 ******************************************************************************
 */

#ifndef STREAK_LINK_READER_HPP
#define STREAK_LINK_READER_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "ActivityLink.hpp"
#include "ActivityScanner.hpp"

namespace Streak
{

/// What a read did, for the log and the tests.
struct LinkStats {
    uint8_t  files     = 0;   ///< summary files that read
    uint16_t entries   = 0;   ///< entries in them
    uint16_t invalid   = 0;   ///< entries a file held that were out of range
    uint16_t outside   = 0;   ///< outside the scan window
    uint16_t counted   = 0;   ///< already counted (seen)
    uint16_t added     = 0;   ///< added to the list
    uint16_t deferred  = 0;   ///< no room: left for the next open
};

class LinkReader
{
public:
    /// Add the new activities in [fromDay, toDay] (local days) to @p found, which
    /// holds @p count entries oldest first and has room for @p cap. The list stays
    /// oldest first. If it is full, the newest entries are dropped (they are not
    /// remembered as counted, so the next open sees them). Returns the new count.
    size_t addTo(SDK::Interface::IFileSystem& fs, int32_t fromDay, int32_t toDay, const SeenSet& seen, Found* found,
                 size_t count, size_t cap);

    const LinkStats& stats() const { return mStats; }

    /// How a summary's sport reads as one of Streak's kinds.
    static Kind kindOf(HybridXLink::Sport sport);

private:
    /// The slot's file into mFile: the primary if it decodes, else the backup.
    bool load(SDK::Interface::IFileSystem& fs, const char* path);
    size_t insert(Found* found, size_t count, size_t cap, const Found& f);

    char                     mText[HybridXLink::kMaxBytes] {};
    HybridXLink::ActivityFile mFile {};
    LinkStats                mStats {};
};

} // namespace Streak

#endif // STREAK_LINK_READER_HPP
