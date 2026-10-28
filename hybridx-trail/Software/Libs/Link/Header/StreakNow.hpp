/**
 ******************************************************************************
 * @file    StreakNow.hpp
 * @brief   The Streak line: a small summary HybridX Streak leaves of this
 *          week, and the line an activity app works out from it after a save
 *          ("3 of 4 this week", "8-week streak").
 *
 * Streak writes ../SharedData/HybridX/streak_now.json each time it saves
 * (its only writer). Integers only, none negative:
 *   {"v":1,"day":20755,"ws":20754,"t":4,"min":10,"sc":255,"opd":0,"pend":0,
 *    "n":2,"days":3,"st":7,"k":[[2688328718,1793135407],...]}
 *   day  the local day it was written (days since 1970-01-01)
 *   ws   the first day of this week
 *   t, min, sc, opd   this week's goal: target, minimum minutes (0 = off),
 *        scope (255 = any, else Streak's Kind code), one per day (0/1)
 *   pend a goal change waits for next week (0/1)
 *   n    sessions counting this week
 *   days bit d set: day ws + d has a counting session
 *   st   the streak before this week (closed weeks; Streak's live streak
 *        minus this week, if met)
 *   k    [appKey, localStart] of every automatic session in this week, at
 *        most kMaxKeys: what Streak has already counted
 *
 * An app, after writing its own activity summary, reads this file and every
 * act_*.json (kSlotPaths) and projects the line with Streak's own rules
 * (StreakModel::status): a session not in k, in this week, of at least the
 * minimum whole minutes, in scope, and on a new day if one-per-day is on.
 * Streak alone decides weeks it has not seen (misses, shields), so:
 *   - this week: "N of T this week" and the streak (st, plus one if met);
 *   - the next week, last week met: the same, the streak carried on;
 *   - otherwise: "N this week" with no target if one waits, and no streak.
 * Pure C++, no file I/O.
 ******************************************************************************
 */

#ifndef HYBRIDX_LINK_STREAK_NOW_HPP
#define HYBRIDX_LINK_STREAK_NOW_HPP

#include <cstddef>
#include <cstdint>

#include "ActivityLink.hpp"

namespace HybridXLink
{

inline constexpr const char* kStreakNowPath = "../SharedData/HybridX/streak_now.json";

/// The key Streak gives an app folder (FNV-1a), shared so the apps and Streak agree.
uint32_t appKey(const char* folder);

/// A Sport as Streak's Kind code (Streak::Kind's order: Run 0, Ride 1, Walk 2, Strength 3,
/// Workout 4, Hybrid 5, Row 6, Other 7), the mapping Streak's LinkReader uses.
uint8_t kindCode(Sport s);

inline constexpr uint8_t kScopeAny = 0xFF;
inline constexpr uint8_t kMaxKeys  = 16;

struct StreakKey {
    uint32_t appKey     = 0;
    uint32_t localStart = 0;
};

struct StreakNow {
    int32_t   day        = 0;
    int32_t   weekStart  = 0;
    uint8_t   target     = 0;
    uint8_t   minMinutes = 0;
    uint8_t   scope      = kScopeAny;
    bool      onePerDay  = false;
    bool      pending    = false;
    uint8_t   counted    = 0;
    uint8_t   daysMask   = 0;
    uint16_t  streak     = 0;
    uint8_t   keyCount   = 0;
    StreakKey keys[kMaxKeys] {};
};

/// A buffer big enough for any summary.
inline constexpr size_t kStreakNowMaxBytes = 640;

/// Write the summary. Returns the length, or 0 (buf emptied) if a value is out of range or it did not fit.
size_t encodeStreakNow(const StreakNow& s, char* buf, size_t cap);
/// Read it. False (and @p out cleared) if it is not a version 1 summary with every value in range.
bool decodeStreakNow(const char* text, size_t len, StreakNow& out);

/// The line to show.
struct StreakLine {
    bool     show       = false;   ///< false: Streak has never saved (not installed, or never opened)
    uint8_t  done       = 0;       ///< sessions counting this week
    uint8_t  target     = 0;       ///< 0: not known (a goal change waits): "N this week"
    bool     hasStreak  = false;
    uint16_t streak     = 0;       ///< weeks, this one included if met
};

/// Works the line out as sessions are offered: construct it from Streak's summary and today's local
/// day, add() every entry of every app's file, then result().
class StreakProjector
{
public:
    StreakProjector(const StreakNow& now, int32_t today);

    /// Offer one app's file (as decoded from its act_*.json).
    void add(const ActivityFile& file);

    StreakLine result() const;

private:
    void offer(uint32_t appKey, const Entry& e);

    StreakNow mNow;
    bool      mValid     = false;
    bool      mSameWeek  = false;
    bool      mNextWeek  = false;   ///< exactly one week on
    int32_t   mWeekStart = 0;       ///< the week being counted
    uint8_t   mDone      = 0;
    uint8_t   mDays      = 0;
    // Sessions added here, so one offered twice counts once.
    StreakKey mAdded[kMaxKeys * kSlotCount] {};
    uint8_t   mAddedCount = 0;
};

/// "3 of 4 this week" (or "3 this week"), and a second line "8-week streak" when there is one, as
/// two strings. Returns false when there is nothing to show.
bool formatStreakLine(const StreakLine& l, char* line1, size_t cap1, char* line2, size_t cap2);

}  // namespace HybridXLink

#endif  // HYBRIDX_LINK_STREAK_NOW_HPP
