/**
 ******************************************************************************
 * @file    PlanState.hpp
 * @brief   What the app keeps about the runner's plan, as plan.json in its
 *          folder (brief section 9): the set-up, the calendar, and a log of
 *          what happened to each planned day.
 *
 *     {"v":1,
 *      "s":[plan,level,days,longDay,easyByHr,repsByDistance,raceDate,
 *           fiveKSec,fiveKDate,fiveKRecent,halfSec,halfDate],
 *      "c":[status,start,firstWeek,weeks],
 *      "log":[[date,kind,status,startUtc],...],
 *      "vo2s":452,
 *      "adj":[[runWeek,delta+8],...],"ov":[[date,from,mode],...],
 *      "ps":date,"rt":date,"rw":planWeek+1,"st":date,"ak":seconds,"od":date}
 *
 * The keys from "adj" on are the plan's memory (TP5, PlanRules.hpp), each written only when it
 * has something to say and read as none when absent: "adj" the weeks repeated or dropped
 * (delta stored +8, so none is negative), "ov" the days moved, made easy or rested, "ps" the
 * day the plan was paused, "rt" the day a retest began (easy running until a time trial), "rw"
 * the plan week (+1) it resumes at once the trial is run, "st"
 * the day to which missed sessions are settled, "ak" a new 5K time waiting for the runner's
 * answer, and "od" a missed session whose offer was declined.
 *
 * "vo2s" (TP4, optional): X Run's VO2max x 10 as first seen in this plan, the
 * VO2max block's second measure (RunVo2Max.hpp); absent until there is one.
 * Older files read without it.
 *
 * Integers only, none negative (the family's rule: the SDK's JSON writer
 * misprints signed values on the host). Every value is range-checked on
 * reading; anything wrong and the file reads as no plan. The app writes it
 * with the family's SafeFile (TP2).
 ******************************************************************************
 */

#ifndef XTRAINING_PLAN_STATE_HPP
#define XTRAINING_PLAN_STATE_HPP

#include "Date.hpp"
#include "PlanCalendar.hpp"
#include "PlanTypes.hpp"
#include "WeekLayout.hpp"

#include <cstddef>
#include <cstdint>

namespace XTraining {

struct PlanSetup {
    PlanId   plan     = PlanId::None;
    uint8_t  level    = 2;
    DayMask  days     = WeekLayout::kDefaultDays;
    uint8_t  longDay  = WeekLayout::kDefaultLongDay;
    bool     easyByHr = false;
    bool     repsByDistance = false;
    Ymd      raceDate = 0;
    uint32_t fiveKSec = 0;
    Ymd      fiveKDate = 0;     ///< The day the app first saw this 5K.
    bool     fiveKRecent = true;
    uint32_t halfSec  = 0;
    Ymd      halfDate = 0;
};

/// A day that does not run the plan's own session (TP5).
enum class OverrideMode : uint8_t {
    Session = 1,  ///< Runs the session planned for date @p from.
    Easy    = 2,  ///< An easy run instead (a hard day made easy).
    Rest    = 3,  ///< A rest day instead.
};

struct Override {
    Ymd          date = 0;
    Ymd          from = 0;  ///< For Session: the day whose session this day runs.
    OverrideMode mode = OverrideMode::Easy;
};
constexpr uint8_t kMaxOverrides = 8;

enum class DayStatus : uint8_t {
    Done     = 1,  ///< Reached the end, and saved.
    PartDone = 2,  ///< Saved, at least half the planned time.
    Missed   = 3,  ///< The day passed with neither.
    Skipped  = 4,  ///< The runner chose to skip it.
};

struct LogEntry {
    Ymd       date     = 0;
    Kind      kind     = Kind::Rest;
    DayStatus status   = DayStatus::Missed;
    uint32_t  startUtc = 0;  ///< The activity's start, 0 if none.
};

/// How a saved planned session counts (brief section 9). Done: it reached its end and ran
/// at least half the planned time; part done: at least half the planned time without the
/// end. Anything shorter does not count (false), so skipping through the steps with the
/// lap button is not a session done.
inline bool judgeSession(bool reachedEnd, uint32_t activeSec, uint32_t plannedSec, DayStatus& out)
{
    if (plannedSec == 0 || static_cast<uint64_t>(activeSec) * 2u < plannedSec) {
        return false;
    }
    out = reachedEnd ? DayStatus::Done : DayStatus::PartDone;
    return true;
}

class PlanState {
public:
    static constexpr uint8_t kMaxLog = 84;  ///< 12 weeks of days.
    static constexpr size_t  kMaxJsonBytes = 3584;

    PlanSetup setup;
    Calendar  calendar;
    /// X Run's VO2max x 10 as first seen on or after the plan's start; 0 = not yet (TP4).
    /// Shown beside the figure now; never used for paces.
    uint16_t  vo2StartX10 = 0;

    // The plan's memory (TP5; PlanRules.hpp). Each reads as none from an older file.
    Ymd      pausedFrom   = 0;  ///< The day the plan was paused; 0 = not paused.
    Ymd      retestFrom   = 0;  ///< After a long pause: easy running until a time trial; 0 = no.
    uint8_t  resumeWeek   = 0;  ///< With a retest: the plan week (+1) the plan resumes at after the trial; 0 = none.
    Ymd      settledTo    = 0;  ///< Days before this are settled (missed ones logged).
    uint32_t askFiveKSec  = 0;  ///< A 5K time from a trial, waiting for "keep or update"; 0 = none.
    Ymd      offerDeclined = 0; ///< The missed session whose offer the runner declined.
    Override overrides[kMaxOverrides];
    uint8_t  overrideCount = 0;

    void clear() { *this = PlanState{}; }

    bool active() const { return setup.plan != PlanId::None && calendar.usable(); }

    /// Record @p e, replacing any entry for its date. When full, the oldest goes.
    void record(const LogEntry& e);

    /// The entry for @p date, or null.
    const LogEntry* find(Ymd date) const;

    /// Make @p date run something other than the plan's own session, replacing any earlier
    /// override for it. When full, the oldest goes.
    void setOverride(const Override& o);
    const Override* findOverride(Ymd date) const;
    /// Forget overrides for days before @p date (they are in the log).
    void pruneOverrides(Ymd date);

    uint8_t logCount() const { return mCount; }
    const LogEntry& log(uint8_t i) const { return mLog[i]; }

    /// Write plan.json into @p buf; the length, or 0 if it did not fit.
    size_t toJson(char* buf, size_t cap) const;

    /// Read plan.json. On any error the state is cleared and false returned.
    bool fromJson(const char* text, size_t len);

private:
    LogEntry mLog[kMaxLog] = {};
    uint8_t  mCount = 0;
};

}  // namespace XTraining

#endif  // XTRAINING_PLAN_STATE_HPP
