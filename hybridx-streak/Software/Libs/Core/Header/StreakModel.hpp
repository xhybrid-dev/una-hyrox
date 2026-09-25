/**
 ******************************************************************************
 * @file    StreakModel.hpp
 * @brief   The streak's rules: weeks, targets, streaks, shields, badges.
 *
 * Pure C++ with no SDK types; "now" is always an argument (PLAN 6). The
 * service and the glance both run it; only the service saves it (PLAN 4).
 *
 * THE ORDER OF WORK on every open is scan -> credit each session to its own
 * week -> close finished weeks (PLAN 6.2), done week by week inside update():
 * a Sunday run found on Tuesday counts for last week before last week is
 * judged.
 *
 * LIVE AND COMMITTED. A week is achieved the moment it meets its target -- the
 * climber steps up there and then (DESIGN 4, 6). So the streak, weeks achieved
 * and lifetime sessions the athlete sees are the committed figures plus the
 * current week's contribution; closing the week commits them. Undoing a
 * manual log below the target takes the live step back down.
 *
 * THE BOUNDARY RULES (PLAN 6.4, S3, S6, S9):
 *   - met                -> achieved: streak +1, weeks +1; a shield per 4
 *                           achieved weeks, at most 2;
 *   - a trial week, missed -> no harm: the streak stands;
 *   - missed, streak 0   -> missed, no prompt;
 *   - missed, streak > 0 -> one pending decision for the whole catch-up:
 *                           with enough shields, "use N to keep your streak?",
 *                           otherwise the streak resets and says why.
 *
 * GOALS. Target, scope, minimum and one-per-day apply from next week (a new
 * target or scope makes that week a trial). A week-start change closes the
 * current week now -- achieved if it met the target, otherwise void -- and a
 * new week begins today under the new start day (S8).
 *
 * DEDUP (PLAN 5.5). Every counted file's key (app hash, local start) goes in a
 * ring of kRing. When the ring evicts a key, every file at least that old is
 * treated as seen, so nothing inside the scan window can be counted twice.
 *
 * Bounded: fixed arrays everywhere, no heap.
 ******************************************************************************
 */

#ifndef STREAK_MODEL_HPP
#define STREAK_MODEL_HPP

#include <cstddef>
#include <cstdint>

#include "ActivityScanner.hpp"
#include "Goal.hpp"
#include "StreakEvents.hpp"
#include "StreakTypes.hpp"
#include "StreakView.hpp"

namespace Streak
{

/// One session this week.
struct Session {
    uint32_t appKey     = 0;       ///< 0 for a manual log
    uint32_t localStart = 0;       ///< local seconds since 1970 (orders the list)
    uint16_t minutes    = 0;       ///< 0 for a manual log
    Kind     kind       = Kind::Other;
    uint8_t  app        = 0xFF;    ///< index into State::apps; kManualApp for a manual log
    uint8_t  flags      = 0;       ///< SessionFlag bits

    static constexpr uint8_t kManual   = 0x01;
    static constexpr uint8_t kExcluded = 0x02;
};

constexpr uint8_t kManualApp = 0xFE;
constexpr uint8_t kNoApp     = 0xFF;

/// Why a session does or does not count this week.
enum class Status : uint8_t {
    Counts,
    Excluded,     ///< the athlete excluded it
    TooShort,     ///< under the minimum duration
    OutOfScope,   ///< the goal counts one kind, and this is another
    SameDay,      ///< one-per-day is on, and the day already has one
};

enum class Outcome : uint8_t { Achieved, Trial, Missed, Shielded, Void };

struct WeekRecord {
    uint8_t count   = 0;
    uint8_t target  = 0;
    Outcome outcome = Outcome::Void;
};

struct Key {
    uint32_t appKey     = 0;
    uint32_t localStart = 0;
};

struct State {
    static constexpr uint8_t kMaxSessions = 16;
    static constexpr uint8_t kRing        = 64;
    static constexpr uint8_t kApps        = 8;
    static constexpr uint8_t kHistory     = 52;
    static constexpr int32_t kNoPeriod    = INT32_MIN;

    Goal     goal {};                 ///< the goal this week is judged by
    Goal     pending {};              ///< takes effect next week, if hasPending
    bool     hasPending = false;

    int32_t  installDay       = 0;
    int32_t  period           = kNoPeriod;   ///< this week, under goal.weekStart
    int32_t  weekStartDay     = 0;           ///< first day of this week
    int32_t  lastDay          = 0;           ///< latest day evaluated; never goes back
    bool     trial            = true;

    // Committed figures (closed weeks only).
    uint16_t streak           = 0;
    uint16_t weeksAchieved    = 0;
    uint16_t longest          = 0;
    uint16_t lifetime         = 0;            ///< qualifying sessions
    uint8_t  shields          = 0;
    uint8_t  bestWeek         = 0;            ///< most qualifying sessions in a closed week
    uint8_t  badges           = 0;            ///< bit i = kSessionBadges[i] earned
    uint8_t  lastWeekCount    = 0;

    // A pending shield decision (PLAN 6.4 rule 4).
    uint8_t  pendingMissed    = 0;            ///< missed weeks awaiting a decision
    uint16_t pendingStreak    = 0;            ///< the streak before them, kept if shields are used
    uint16_t sinceLastMiss    = 0;            ///< weeks achieved since the last missed one

    // This week.
    Session  sessions[kMaxSessions] {};
    uint8_t  sessionCount     = 0;
    uint8_t  overflow         = 0;            ///< qualifying sessions beyond kMaxSessions
    bool     celebrated       = false;        ///< StepUp already played for this week
    bool     bestCelebrated   = false;

    // Dedup (PLAN 5.5).
    Key      ring[kRing] {};
    uint8_t  ringNext         = 0;
    uint8_t  ringCount        = 0;
    uint32_t ringFloor        = 0;            ///< files at or before this local second count as seen

    // App folder names for the week list.
    char     apps[kApps][kAppNameChars] {};

    // The last 52 closed weeks, oldest overwritten first.
    WeekRecord history[kHistory] {};
    uint8_t  historyNext      = 0;
    uint8_t  historyCount     = 0;
};


class StreakModel : public SeenSet
{
public:
    static constexpr int32_t kMaxCatchUpDays = 56;   ///< the longest scan window (8 weeks)

    StreakModel() = default;

    State&       state() { return mState; }
    const State& state() const { return mState; }

    /// A new athlete: no weeks yet; the first update() starts a trial week.
    void reset(const Goal& goal);

    /// The days the scanner should look at before update(nowDay): the
    /// previous and current week, stretched back to the last open's week
    /// (at most kMaxCatchUpDays). A first open looks at this week only (S15).
    void scanWindow(int32_t nowDay, int32_t& fromDay, int32_t& toDay) const;

    /// SeenSet: has this file been counted?
    bool seen(uint32_t appKey, uint32_t localStart) const override;

    /// Credit what the scan found, each to its own week, and close every week
    /// that has ended. @p found must be oldest first (as the scanner returns it).
    void update(int32_t nowDay, const Found* found, size_t count, Events& ev);

    /// A manual log, today or yesterday, within this week. False if refused.
    bool logManual(Kind kind, bool yesterday, int32_t nowDay, uint32_t secondOfDay, Events& ev);
    /// Remove a manual log (index into State::sessions).
    bool undo(uint8_t index, Events& ev);
    /// Exclude an automatic session, or include it again.
    bool toggleExclude(uint8_t index, Events& ev);
    /// Change the goal (see the file comment for when it applies).
    void setGoal(const Goal& goal, int32_t nowDay, Events& ev);
    /// Answer a ShieldOffer.
    void decideShields(bool use, Events& ev);

    // -- Views ----------------------------------------------------------------
    Status   status(uint8_t index) const;
    uint8_t  qualifying() const;                      ///< this week's counting sessions
    bool     weekMet() const { return qualifying() >= mState.goal.target; }
    uint16_t liveStreak() const;
    uint16_t liveWeeks() const;
    uint16_t liveLifetime() const;
    HomeView view(int32_t nowDay) const;

private:
    void    startWeek(int32_t period, int32_t startDay, bool trial);
    void    closeWeek(Events& ev, bool isLast);
    void    resolvePending(Events& ev);
    void    credit(const Found& f, Events& ev);
    void    remember(uint32_t appKey, uint32_t localStart);
    uint8_t appIndex(const char* name);
    void    afterChange(Events& ev);
    void    addHistory(uint8_t count, uint8_t target, Outcome outcome);
    void    commitAchieved(uint8_t count, Events& ev);

    State mState {};
};

} // namespace Streak

#endif // STREAK_MODEL_HPP
