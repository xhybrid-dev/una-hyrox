/**
 ******************************************************************************
 * @file    ReviewInsight.hpp
 * @brief   What the reviews say together (TP8): sessions off pace in a row,
 *          which way they missed, and how the runner felt on them. Pure.
 *
 * The rule (Jon, 20 November 2026): nothing is suggested after one bad run.
 * A run of kOffPaceRun judged sessions in a row below kOffPacePct in band
 * is what the app speaks of, and:
 *   - missing slow (too slow, under the target) offers to hold the plan: run
 *     this week's plan week again next week;
 *   - missing fast (too fast, over the target) only says so: pace discipline,
 *     not fitness, so the plan is not changed.
 * A session not judged (a treadmill, a heart-rate target, too little time) is
 * skipped, and does not break a run; a judged session on pace ends it. A
 * suggestion that was answered (taken or declined) is not made again until
 * kOffPaceRun new sessions after the answer (ReviewLog::handled).
 ******************************************************************************
 */

#ifndef XTRAINING_REVIEW_INSIGHT_HPP
#define XTRAINING_REVIEW_INSIGHT_HPP

#include "ReviewLog.hpp"
#include "SessionReview.hpp"

#include <cstdint>

namespace XTraining {
namespace Review {

struct Insight {
    uint8_t judged    = 0;   ///< Judged sessions in the log.
    uint8_t offRun    = 0;   ///< The newest judged sessions in a row off pace, after the last answer.
    Dir     dir       = Dir::None;   ///< Which way those missed.
    uint8_t asked     = 0;   ///< Of them, how many have a feel.
    uint8_t tired     = 0;   ///< ... Tired or Spent.
    uint8_t fresh     = 0;   ///< ... Fresh or Good.
    Ymd     newest    = 0;   ///< The newest of the run.
    bool    latestOn  = false;   ///< The newest judged session was on pace.
    uint8_t latestFeel = 0;      ///< and its feel.

    /// Enough in a row for the app to speak of it.
    bool streak() const { return offRun >= kOffPaceRun; }
    /// ... and a plan change is the thing to offer.
    bool offerEase() const { return streak() && dir == Dir::Slow; }
};

Insight analyse(const ReviewLog& log);

/// The one line the Form page says of the pattern.
enum class Line : uint8_t {
    None = 0,
    OnTrack,      ///< The newest session was on pace.
    OnTired,      ///< ... and the runner was tired: held the pace anyway.
    SlowTired,    ///< A run too slow, mostly tired.
    SlowFresh,    ///< A run too slow, though fresh or good: the paces may be set too hard.
    SlowMixed,    ///< A run too slow, no clear feel.
    Fast,         ///< A run too fast.
};

Line lineFor(const Insight& in);

}  // namespace Review
}  // namespace XTraining

#endif  // XTRAINING_REVIEW_INSIGHT_HPP
