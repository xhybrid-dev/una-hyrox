/**
 ******************************************************************************
 * @file    ActivityScanner.cpp
 * @brief   Finds new activities in every app's Activity folder (see the header).
 ******************************************************************************
 */

#include "ActivityScanner.hpp"

#include <cstdio>
#include <cstring>

#include "Classifier.hpp"
#include "WeekMath.hpp"

namespace Streak
{

namespace
{
constexpr const char* kShared      = "SharedData";
constexpr const char* kMarker      = ".recording";   // SDK RecordingMarker::kFileName
constexpr int32_t     kSecondsDay  = 86400;
/// The session's UTC start and the name's local start may differ by the zone
/// offset (at most 14 h) and no more; a day is the plan's tolerance (PLAN 5.4).
constexpr int64_t     kMaxSkewS    = kSecondsDay;

bool digits(const char* s, int n, uint32_t& out)
{
    out = 0;
    for (int i = 0; i < n; ++i) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
        out = out * 10u + static_cast<uint32_t>(s[i] - '0');
    }
    return true;
}

char lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

void copy(char* out, size_t n, const char* in)
{
    std::strncpy(out, in, n - 1);
    out[n - 1] = '\0';
}

/// The last path component.
const char* baseName(const char* path)
{
    const char* slash = std::strrchr(path, '/');
    return slash ? slash + 1 : path;
}
} // namespace

uint32_t ActivityScanner::appKey(const char* folder)
{
    uint32_t h = 2166136261u;
    for (const char* p = folder; p && *p; ++p) {
        h ^= static_cast<uint8_t>(*p);
        h *= 16777619u;
    }
    return h;
}

bool ActivityScanner::parseName(const char* name, uint32_t& localStart)
{
    // activity_YYYYMMDDTHHMMSS.fit (Examples/*/ActivityWriter.cpp).
    if (!name || std::strlen(name) != 28 || std::strncmp(name, "activity_", 9) != 0 || name[17] != 'T'
        || name[24] != '.' || lower(name[25]) != 'f' || lower(name[26]) != 'i' || lower(name[27]) != 't') {
        return false;
    }
    uint32_t y, mo, d, h, mi, s;
    if (!digits(name + 9, 4, y) || !digits(name + 13, 2, mo) || !digits(name + 15, 2, d) || !digits(name + 18, 2, h)
        || !digits(name + 20, 2, mi) || !digits(name + 22, 2, s)) {
        return false;
    }
    if (y < 2000 || y > 2100 || mo < 1 || mo > 12 || d < 1 || d > 31 || h > 23 || mi > 59 || s > 60) {
        return false;
    }
    const int32_t day = WeekMath::daysFromCivil(static_cast<int32_t>(y), mo, d);
    localStart = static_cast<uint32_t>(day) * static_cast<uint32_t>(kSecondsDay) + h * 3600u + mi * 60u + s;
    return true;
}

void ActivityScanner::listApps(SDK::Interface::IFileSystem& fs, const char* ownFolder)
{
    mAppCount = 0;
    auto dir  = fs.dir("..");
    if (!dir || !dir->open()) {
        return;
    }
    mStats.listed = true;
    uint16_t seen = 0;
    while (seen < kMaxEntries && mAppCount < kMaxApps && dir->readNext(mInfo)) {
        ++seen;
        if (!mInfo.isDir || mInfo.name[0] == '.' || std::strcmp(mInfo.name, kShared) == 0
            || (ownFolder && std::strcmp(mInfo.name, ownFolder) == 0)
            || std::strlen(mInfo.name) >= kMaxFolderChars) {
            continue;
        }
        copy(mApps[mAppCount++], kMaxFolderChars, mInfo.name);
    }
    dir->close();
}

void ActivityScanner::readMarker(SDK::Interface::IFileSystem& fs, uint8_t app, char* out, size_t n)
{
    out[0] = '\0';
    char path[SDK::Interface::IFileSystem::skMaxPathLen];
    if (snprintf(path, sizeof(path), "../%s/Activity/%s", mApps[app], kMarker) >= static_cast<int>(sizeof(path))) {
        return;
    }
    auto file = fs.file(path);
    if (!file || !file->open(false, false)) {
        return;
    }
    // Line 1 is the .fit being written (RecordingMarker.hpp).
    char   line[SDK::Interface::IFileSystem::skMaxPathLen] {};
    size_t got = 0;
    file->read(line, sizeof(line) - 1, got);
    file->close();
    line[got < sizeof(line) ? got : sizeof(line) - 1] = '\0';
    char* nl = std::strchr(line, '\n');
    if (nl) {
        *nl = '\0';
    }
    copy(out, n, baseName(line));
}

void ActivityScanner::addCandidate(const Candidate& c)
{
    if (mCandidateCount < kMaxCandidates) {
        mCandidates[mCandidateCount++] = c;
        return;
    }
    // Full: keep the oldest, so nothing old is starved by a burst of new files.
    uint8_t newest = 0;
    for (uint8_t i = 1; i < mCandidateCount; ++i) {
        if (mCandidates[i].localStart > mCandidates[newest].localStart) {
            newest = i;
        }
    }
    if (c.localStart < mCandidates[newest].localStart) {
        mCandidates[newest] = c;
    }
    ++mStats.deferred;
}

size_t ActivityScanner::scan(SDK::Interface::IFileSystem& fs, const char* ownFolder, int32_t fromDay, int32_t toDay,
                             const SeenSet& seen, Found* out, size_t cap)
{
    mStats          = ScanStats {};
    mCandidateCount = 0;
    if (fromDay > toDay || cap == 0) {
        return 0;
    }
    listApps(fs, ownFolder);
    mStats.apps = mAppCount;

    // The month folders the window touches: at most two for a 14-day window,
    // but walk month by month so any window works.
    char    months[4][7] {};
    uint8_t monthCount = 0;
    for (int32_t day = fromDay; day <= toDay && monthCount < 4;) {
        const WeekMath::Civil c = WeekMath::civilFromDays(day);
        const unsigned year = static_cast<unsigned>(c.year < 0 ? 0 : (c.year > 9999 ? 9999 : c.year));
        snprintf(months[monthCount++], sizeof(months[0]), "%04u%02u", year % 10000u,
                 static_cast<unsigned>(c.month % 100u));
        // First day of the next month.
        const uint32_t nm = c.month == 12u ? 1u : c.month + 1u;
        const int32_t  ny = c.month == 12u ? c.year + 1 : c.year;
        day               = WeekMath::daysFromCivil(ny, nm, 1);
    }

    for (uint8_t app = 0; app < mAppCount; ++app) {
        char recording[32];
        readMarker(fs, app, recording, sizeof(recording));
        const uint32_t key = appKey(mApps[app]);

        for (uint8_t m = 0; m < monthCount; ++m) {
            char path[SDK::Interface::IFileSystem::skMaxPathLen];
            if (snprintf(path, sizeof(path), "../%s/Activity/%s", mApps[app], months[m])
                >= static_cast<int>(sizeof(path))) {
                continue;
            }
            auto dir = fs.dir(path);
            if (!dir || !dir->open()) {
                continue;   // no activity that month, or no Activity folder at all
            }
            uint16_t entries = 0;
            while (entries < kMaxEntries && dir->readNext(mInfo)) {
                ++entries;
                uint32_t start = 0;
                if (mInfo.isDir || !parseName(mInfo.name, start)) {
                    continue;
                }
                const int32_t day = static_cast<int32_t>(start / static_cast<uint32_t>(kSecondsDay));
                if (day < fromDay || day > toDay || seen.seen(key, start)) {
                    continue;
                }
                if (recording[0] != '\0' && std::strcmp(recording, mInfo.name) == 0) {
                    ++mStats.recording;
                    continue;
                }
                Candidate c;
                c.app        = app;
                c.localStart = start;
                copy(c.month, sizeof(c.month), months[m]);
                copy(c.name, sizeof(c.name), mInfo.name);
                ++mStats.candidates;
                addCandidate(c);
            }
            dir->close();
        }
    }

    // Oldest first (insertion sort: at most kMaxCandidates).
    for (uint8_t i = 1; i < mCandidateCount; ++i) {
        const Candidate c = mCandidates[i];
        int             j = i - 1;
        while (j >= 0 && mCandidates[j].localStart > c.localStart) {
            mCandidates[j + 1] = mCandidates[j];
            --j;
        }
        mCandidates[j + 1] = c;
    }

    size_t  count = 0;
    uint8_t reads = 0;
    for (uint8_t i = 0; i < mCandidateCount && count < cap; ++i) {
        if (reads >= kMaxNew) {
            mStats.deferred = static_cast<uint16_t>(mStats.deferred + (mCandidateCount - i));
            break;
        }
        const Candidate& c = mCandidates[i];
        char path[SDK::Interface::IFileSystem::skMaxPathLen];
        snprintf(path, sizeof(path), "../%s/Activity/%s/%s", mApps[c.app], c.month, c.name);
        ++reads;
        const FitSession s = mReader.read(fs, path);
        if (!s.ok()) {
            ++mStats.rejected;
            continue;
        }
        if (s.startFit != 0) {
            const int64_t utc  = static_cast<int64_t>(s.startFit) + kFitEpochOffset;
            const int64_t skew = static_cast<int64_t>(c.localStart) - utc;
            if (skew > kMaxSkewS || skew < -kMaxSkewS) {
                ++mStats.mismatched;
                continue;
            }
        }
        Found& f     = out[count++];
        f            = Found {};
        f.appKey     = appKey(mApps[c.app]);
        f.localStart = c.localStart;
        f.localDay   = static_cast<int32_t>(c.localStart / static_cast<uint32_t>(kSecondsDay));
        f.kind       = classify(s.sport, s.subSport, mApps[c.app]);
        f.minutes    = static_cast<uint16_t>(s.timerSeconds / 60u > 0xFFFFu ? 0xFFFFu : s.timerSeconds / 60u);
        copy(f.app, sizeof(f.app), mApps[c.app]);
        ++mStats.read;
    }
    return count;
}

} // namespace Streak
