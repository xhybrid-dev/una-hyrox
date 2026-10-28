/**
 ******************************************************************************
 * @file    ActivityLink.hpp
 * @brief   The HybridX activity summary: what an activity app leaves in the
 *          shared folder for HybridX Streak to count.
 *
 * Each activity app owns one file, ../SharedData/HybridX/act_<slot>.json
 * (kSlotPaths), and is its only writer. The file holds that app's last
 * kMaxEntries activities, oldest first:
 *
 *   {"v":1,"app":"HybridXRun","a":[[1791980037,1500,2700,5400,152,171,1],...]}
 *
 * app = the app's folder name (the name Streak's own folder scan would use, so
 * the same activity found both ways is counted once); each entry is
 * [startUtc, tzOffsetMin + 1440, durationSec, distanceM, avgHr, maxHr, sport].
 * Integers only, none negative: the SDK writer prints signed 32-bit values
 * with "%ld", which is wrong where int32_t is an int (the host tests), so the
 * time-zone offset is stored 1440 higher. Distance and heart rate are 0 when
 * the app has none. Sport codes are ours (Sport), not FIT's.
 *
 * The reader bounds every value (the SDK's reader does no range check). A file
 * that is missing, malformed, from another version or holds too many entries
 * reads as nothing; a single bad entry is skipped and counted, the rest kept.
 *
 * This code does no file I/O: the app reads and writes with its own SafeFile
 * (primary, then .bak). Nothing depends on listing the shared folder (not
 * shown to work on the watch), so the reader opens the fixed names in
 * kSlotPaths.
 *
 * Canonical source: hybridx-link/. Each app keeps a byte-identical copy in its
 * Software/Libs/Link/; Utilities/check-link-copies.sh keeps them so.
 ******************************************************************************
 */

#ifndef HYBRIDX_LINK_ACTIVITY_LINK_HPP
#define HYBRIDX_LINK_ACTIVITY_LINK_HPP

#include <cstddef>
#include <cstdint>

namespace HybridXLink
{

inline constexpr uint8_t kVersion = 1;

/// Activities kept per file (the oldest are dropped).
inline constexpr uint8_t kMaxEntries = 16;

/// The app's folder name: 1 to 15 characters of A-Z a-z 0-9 _ -, plus the NUL.
inline constexpr size_t kAppNameChars = 16;

/// A buffer big enough for any file the writer makes (the worst case is about 710 bytes).
inline constexpr size_t kMaxBytes = 1024;

/// What an entry may hold. Anything else is not an activity.
inline constexpr uint32_t kMinStartUtc    = 1577836800u;   ///< 1 January 2020: below it the clock was not set
inline constexpr int16_t  kMinTzOffsetMin = -720;          ///< UTC-12
inline constexpr int16_t  kMaxTzOffsetMin = 840;           ///< UTC+14
inline constexpr uint32_t kMaxDurationSec = 172800;        ///< 48 hours
inline constexpr uint32_t kMaxDistanceM   = 1000000;       ///< 1000 km
inline constexpr int16_t  kTzOffsetBias   = 1440;          ///< stored, so the file holds no negative number

/// What kind of activity. The codes are ours and mirror HybridX Streak's kinds;
/// they are not FIT sport numbers. A code this reader does not know reads as Other.
enum class Sport : uint8_t {
    Run      = 1,
    Ride     = 2,
    Walk     = 3,
    Strength = 4,
    Workout  = 5,
    Hybrid   = 6,
    Row      = 7,
    Other    = 9,
};

Sport sportFromCode(int64_t code);

/// One activity.
struct Entry {
    uint32_t startUtc    = 0;   ///< start, UTC seconds
    int16_t  tzOffsetMin = 0;   ///< local time minus UTC, minutes, at the start
    uint32_t durationSec = 0;   ///< timer time
    uint32_t distanceM   = 0;   ///< 0 = none
    uint8_t  avgHr       = 0;   ///< 0 = none
    uint8_t  maxHr       = 0;   ///< 0 = none
    Sport    sport       = Sport::Other;

    /// Local start, seconds since 1970-01-01 on the local calendar: the
    /// "localStart" HybridX Streak keys its dedupe on.
    uint32_t localStart() const
    {
        return static_cast<uint32_t>(static_cast<int64_t>(startUtc) + static_cast<int64_t>(tzOffsetMin) * 60);
    }

    bool operator==(const Entry& o) const
    {
        return startUtc == o.startUtc && tzOffsetMin == o.tzOffsetMin && durationSec == o.durationSec &&
               distanceM == o.distanceM && avgHr == o.avgHr && maxHr == o.maxHr && sport == o.sport;
    }
};

/// Every value in range, and the average no higher than the maximum.
bool valid(const Entry& e);

/// An entry from an app's totals, which are floats and may be anything (0 when
/// there is no heart rate, NaN if a counter went wrong): rounded to whole
/// numbers and held inside the limits, a negative or NaN becoming 0, the average
/// held to no more than the maximum. @p startUtc and @p tzOffsetMin are not
/// changed, so a bad start still fails valid().
Entry makeEntry(uint32_t startUtc, int16_t tzOffsetMin, int64_t durationSec, float distanceM, float avgHr,
                float maxHr, Sport sport);

/// A calendar date and time as seconds since 1970-01-01 on that same calendar
/// (no zone applied): what an app has after the kernel's local-time call gives it
/// a broken-down time (std::tm: pass tm_year + 1900, tm_mon + 1, and so on).
int64_t calendarSeconds(int year, int month, int day, int hour, int min, int sec);

/// The time-zone offset in minutes from a UTC time and the same moment on the
/// local calendar (both in seconds, the second as calendarSeconds gives it),
/// rounded to the nearest minute and clamped to the range an entry may hold.
int16_t tzOffsetMinutes(uint32_t utc, int64_t localSeconds);

/// A file's contents.
struct ActivityFile {
    char    app[kAppNameChars] {};
    uint8_t count   = 0;    ///< entries in @c entries
    uint8_t skipped = 0;    ///< entries in the file that were out of range and left out
    Entry   entries[kMaxEntries] {};
};

/// True for an app folder name the file may carry.
bool validAppName(const char* name);

/// Read a file. False if it is not a version 1 activity file (see the top of
/// this file); @p out is then empty.
bool decode(const char* text, size_t len, ActivityFile& out);

/// Write a file. Returns the length, or 0 (with @p buf emptied) if the
/// arguments are bad, an entry is not valid, or it did not fit.
size_t encode(const ActivityFile& file, char* buf, size_t cap);

/// The file after an app adds one activity: parse @p oldText (none, or a file
/// that does not read, or one from another app, starts a fresh list), insert
/// the entry by start time (an entry with the same start is replaced), keep the
/// newest kMaxEntries, and write it to @p buf. Returns the length, or 0 on
/// failure (a bad app name or entry, or it did not fit).
size_t append(const char* oldText, size_t oldLen, const char* app, const Entry& entry, char* buf, size_t cap);

/// ---- Where each app's file is -------------------------------------------------------------

/// The apps that have a file. HybridX Streak opens every one that exists, so an
/// app listed here that is not built yet costs a failed open and nothing more.
/// A new app takes the next free slot (and a Streak release); ride and workout
/// are reserved now so those apps work with a Streak already released.
enum class Slot : uint8_t {
    Run     = 0,
    Race    = 1,
    Trail   = 2,
    Ride    = 3,
    Workout = 4,
};

inline constexpr uint8_t kSlotCount = 5;
inline constexpr uint8_t kMaxSlots  = 8;   ///< room to grow without a bigger table in the reader

inline constexpr const char* kSlotPaths[kSlotCount] = {
    "../SharedData/HybridX/act_run.json",
    "../SharedData/HybridX/act_race.json",
    "../SharedData/HybridX/act_trail.json",
    "../SharedData/HybridX/act_ride.json",
    "../SharedData/HybridX/act_workout.json",
};

/// The folder that holds them (for mkdir).
inline constexpr const char* kSharedDir = "../SharedData/HybridX";

inline const char* slotPath(Slot s)
{
    const uint8_t i = static_cast<uint8_t>(s);
    return i < kSlotCount ? kSlotPaths[i] : nullptr;
}

}  // namespace HybridXLink

#endif  // HYBRIDX_LINK_ACTIVITY_LINK_HPP
