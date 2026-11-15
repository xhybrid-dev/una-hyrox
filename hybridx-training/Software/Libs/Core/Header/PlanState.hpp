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
 *      "vo2s":452}
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
    static constexpr size_t  kMaxJsonBytes = 2560;

    PlanSetup setup;
    Calendar  calendar;
    /// X Run's VO2max x 10 as first seen on or after the plan's start; 0 = not yet (TP4).
    /// Shown beside the figure now; never used for paces.
    uint16_t  vo2StartX10 = 0;

    void clear() { *this = PlanState{}; }

    bool active() const { return setup.plan != PlanId::None && calendar.usable(); }

    /// Record @p e, replacing any entry for its date. When full, the oldest goes.
    void record(const LogEntry& e);

    /// The entry for @p date, or null.
    const LogEntry* find(Ymd date) const;

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
