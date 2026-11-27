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
 * and, optional (Streak NOTES "S10", 23 November 2026; a file without them reads
 * as before, and a reader that predates them ignores them):
 *   at   the local second of the save: Streak had counted what started before
 *   lw   Streak's training load window (LoadHistory.hpp's LoadState): [first day,
 *        first open day, fitness x100, fatigue x100, seeded 0/1, [14 daily loads]]
 *
 * With them, the app also works out where form stands after its session: Streak's
 * window, plus the load of every session Streak has not counted (one not in k, or
 * started after "at"), replayed to today by Streak's own model (formatFormNow). A
 * session Streak counted in its week beyond the 16 in k, or one recording across
 * Streak's save, can be counted twice or missed here; Streak's own figures, on its
 * next open, are always right.
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
#include "LoadHistory.hpp"

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
    // Training load (optional; see the file comment).
    bool              hasLoad = false;
    uint32_t          at      = 0;   ///< local seconds of the save
    Streak::LoadState load {};       ///< load.started is true whenever hasLoad is
};

/// A buffer big enough for any summary, the load window included (it fits the apps' kMaxBytes
/// buffers, which they read it with).
inline constexpr size_t kStreakNowMaxBytes = 896;
static_assert(kStreakNowMaxBytes <= kMaxBytes, "the apps read streak_now.json into an activity file's buffer");

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
    // Form after the sessions Streak has not counted, this one included (optional; S10).
    bool     hasForm    = false;   ///< false: no load window, or still learning (two weeks)
    int16_t  form       = 0;       ///< whole points
    uint8_t  formWord   = 0;       ///< Streak::FormWord
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

    void offerLoad(uint32_t appKey, uint32_t localStart, uint16_t load);

    StreakNow mNow;
    int32_t   mToday     = 0;
    bool      mValid     = false;
    bool      mSameWeek  = false;
    bool      mNextWeek  = false;   ///< exactly one week on
    int32_t   mWeekStart = 0;       ///< the week being counted
    uint8_t   mDone      = 0;
    uint8_t   mDays      = 0;
    // Sessions added here, so one offered twice counts once.
    StreakKey mAdded[kMaxKeys * kSlotCount] {};
    uint8_t   mAddedCount = 0;
    // Loads Streak has not counted, for form (S10), once each. Apps build the projector on their
    // service stack, so the list is short: more than 32 sessions since Streak last opened, all
    // with a load, is far past any real fortnight (the rest would be left out of the form shown).
    static constexpr uint8_t kMaxNewLoads = 32;
    struct NewLoad {
        StreakKey key {};
        uint16_t  load = 0;
    };
    NewLoad   mLoads[kMaxNewLoads] {};
    uint8_t   mLoadCount = 0;
};

/// "3 of 4 this week" (or "3 this week"), and a second line "8-week streak" when there is one, as
/// two strings. Returns false when there is nothing to show.
bool formatStreakLine(const StreakLine& l, char* line1, size_t cap1, char* line2, size_t cap2);

/// "Form now +12, fresh" (Streak::formatFormNow, at most @p maxChars), or empty when the line has
/// no form. Returns the length.
size_t formatStreakForm(const StreakLine& l, char* out, size_t cap, size_t maxChars = 21);

}  // namespace HybridXLink

#endif  // HYBRIDX_LINK_STREAK_NOW_HPP
