/**
 ******************************************************************************
 * @file    ReviewLog.hpp
 * @brief   The last sessions' reviews and how the runner felt (TP8): the
 *          file reviews.json, in the app's own folder, beside plan.json (which
 *          is full to its cap and stays as it is). Pure; the service writes the
 *          bytes with SafeFile.
 *
 * One entry per planned session that counted (a day has one: a rerun replaces
 * it), oldest first, at most kMax. A session that was not judged (a treadmill, a
 * heart-rate target, too little time) is still an entry, so that its feel is
 * kept; its judgedSec is 0.
 *
 *   {"v":1,"eh":20261120,"e":[[date,kind,in,fast,gap,fade,sec,feel], ...]}
 *
 * The gap and the fade are stored plus 600 and plus 100, so that the file has no negative number
 * (the SDK's writer prints a negative 32-bit integer wrongly on a host build).
 * "eh" is the day the last suggestion was answered (taken or declined): the
 * next one needs kOffPaceRun new sessions after it. The log is cleared when the
 * plan ends or is replaced.
 ******************************************************************************
 */

#ifndef XTRAINING_REVIEW_LOG_HPP
#define XTRAINING_REVIEW_LOG_HPP

#include "Date.hpp"
#include "PlanTypes.hpp"
#include "SessionReview.hpp"

#include <cstddef>
#include <cstdint>

namespace XTraining {

struct ReviewEntry {
    Ymd      date      = 0;
    Kind     kind      = Kind::Rest;
    uint8_t  inPct     = 0;     ///< The judged shares; slow is the rest.
    uint8_t  fastPct   = 0;
    int16_t  gapSecKm  = 0;
    int8_t   fadeSecKm = 0;
    uint16_t judgedSec = 0;     ///< 0 = not judged.
    uint8_t  feel      = 0;     ///< Review::Feel; 0 = not asked.

    bool    judged() const { return judgedSec != 0; }
    uint8_t slowPct() const
    {
        const int s = 100 - inPct - fastPct;
        return judged() && s > 0 ? static_cast<uint8_t>(s) : 0;
    }
    bool offPace() const { return judged() && inPct < Review::kOffPacePct; }
    Review::Dir dir() const
    {
        if (!judged() || (fastPct == 0 && slowPct() == 0)) {
            return Review::Dir::None;
        }
        return fastPct > slowPct() ? Review::Dir::Fast : Review::Dir::Slow;
    }

    /// The entry for a session's @p result (an unjudged result gives the zeros).
    static ReviewEntry from(Ymd date, Kind kind, const Review::Result& result, uint8_t feel = 0);
};

class ReviewLog {
public:
    static constexpr uint8_t kMax = 24;
    static constexpr size_t  kMaxJsonBytes = 1536;

    /// The day the last suggestion was answered; 0 = never.
    Ymd handled = 0;

    void    clear();
    uint8_t count() const { return mCount; }
    /// 0 is the oldest. Out of range gives an empty entry.
    const ReviewEntry& at(uint8_t i) const;
    const ReviewEntry* find(Ymd date) const;

    /// Add the session of @p e.date, or replace that day's; keeps date order, drops the oldest when full.
    void record(const ReviewEntry& e);
    /// The feel of the session on @p date; false if there is no entry (or the feel is out of range).
    bool setFeel(Ymd date, uint8_t feel);

    /// JSON into @p buf; the length, or 0 if it does not fit.
    size_t toJson(char* buf, size_t cap) const;
    /// Replace the log with @p text's; false (and the log empty) if it is not a valid reviews.json.
    bool fromJson(const char* text, size_t len);

private:
    ReviewEntry mEntry[kMax];
    uint8_t     mCount = 0;
};

}  // namespace XTraining

#endif  // XTRAINING_REVIEW_LOG_HPP
