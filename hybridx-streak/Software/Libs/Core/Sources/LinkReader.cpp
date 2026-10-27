/**
 ******************************************************************************
 * @file    LinkReader.cpp
 * @brief   Reads the activity summaries the HybridX apps leave (see the header).
 ******************************************************************************
 */

#include "LinkReader.hpp"

#include <cstdio>
#include <cstring>

#include "SafeFile.hpp"

namespace Streak
{

namespace
{
constexpr uint32_t kSecondsDay = 86400u;
} // namespace

Kind LinkReader::kindOf(HybridXLink::Sport sport)
{
    switch (sport) {
        case HybridXLink::Sport::Run: return Kind::Run;
        case HybridXLink::Sport::Ride: return Kind::Ride;
        case HybridXLink::Sport::Walk: return Kind::Walk;
        case HybridXLink::Sport::Strength: return Kind::Strength;
        case HybridXLink::Sport::Workout: return Kind::Workout;
        case HybridXLink::Sport::Hybrid: return Kind::Hybrid;
        case HybridXLink::Sport::Row: return Kind::Row;
        case HybridXLink::Sport::Other: break;
    }
    return Kind::Other;
}

bool LinkReader::load(SDK::Interface::IFileSystem& fs, const char* path)
{
    for (int backup = 0; backup < 2; ++backup) {
        const size_t len = SafeFile::read(fs, path, backup != 0, mText, sizeof(mText));
        if (len > 0 && HybridXLink::decode(mText, len, mFile)) {
            return true;
        }
    }
    return false;
}

size_t LinkReader::insert(Found* found, size_t count, size_t cap, const Found& f)
{
    size_t at = count;
    while (at > 0 && found[at - 1].localStart > f.localStart) {
        --at;
    }
    if (count == cap) {
        ++mStats.deferred;
        if (at == count) {
            return count;   // the newest of all: it waits
        }
        --count;            // the newest in the list waits instead
    }
    std::memmove(&found[at + 1], &found[at], (count - at) * sizeof(Found));
    found[at] = f;
    ++mStats.added;
    return count + 1;
}

size_t LinkReader::addTo(SDK::Interface::IFileSystem& fs, int32_t fromDay, int32_t toDay, const SeenSet& seen,
                         Found* found, size_t count, size_t cap)
{
    mStats = LinkStats {};
    if (found == nullptr || count > cap) {
        return count;
    }
    for (uint8_t slot = 0; slot < HybridXLink::kSlotCount; ++slot) {
        if (!load(fs, HybridXLink::kSlotPaths[slot])) {
            continue;   // nobody has used this slot, or its file does not read
        }
        ++mStats.files;
        mStats.entries   = static_cast<uint16_t>(mStats.entries + mFile.count + mFile.skipped);
        mStats.invalid   = static_cast<uint16_t>(mStats.invalid + mFile.skipped);
        const uint32_t key = ActivityScanner::appKey(mFile.app);
        for (uint8_t i = 0; i < mFile.count; ++i) {
            const HybridXLink::Entry& e = mFile.entries[i];
            Found                     f;
            f.appKey     = key;
            f.localStart = e.localStart();
            f.localDay   = static_cast<int32_t>(f.localStart / kSecondsDay);
            if (f.localDay < fromDay || f.localDay > toDay) {
                ++mStats.outside;
                continue;
            }
            if (seen.seen(f.appKey, f.localStart)) {
                ++mStats.counted;
                continue;
            }
            f.kind    = kindOf(e.sport);
            const uint32_t minutes = e.durationSec / 60u;   // whole minutes, as the FIT reader does
            f.minutes = static_cast<uint16_t>(minutes > 0xFFFFu ? 0xFFFFu : minutes);
            std::snprintf(f.app, sizeof(f.app), "%s", mFile.app);
            count = insert(found, count, cap, f);
        }
    }
    return count;
}

} // namespace Streak
