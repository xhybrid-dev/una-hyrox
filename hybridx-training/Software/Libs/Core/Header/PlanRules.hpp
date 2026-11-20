/**
 ******************************************************************************
 * @file    PlanRules.hpp
 * @brief   The plan's memory and recalibration (TP5; PLANS.md 2.6 and 4.3, brief section 9).
 *
 * Pure rules over a PlanState: no clock (the caller passes today) and no file I/O.
 *
 *   - effective sessions: the plan's own session on a day, with the days the runner moved,
 *     made easy or rested (PlanState overrides), a pause (rest) and a retest (easy running);
 *   - settle(): the days gone since the last time are logged Missed. A missed key session
 *     takes the place of the week's later quality if there is one; a missed easy run is
 *     dropped; at a week's end, a week less than half done is repeated (with a race date it
 *     takes the next week's place, never touching the last two weeks of the 10K or three of
 *     the half; without one the plan grows a week, twice at most), and a week with nothing
 *     done repeats the last cut-back week instead;
 *   - offers: yesterday's missed long run (once, today, if today is easy or rest) and a
 *     missed key session instead of today's easier session;
 *   - moves: tomorrow's session today, today's to tomorrow, skip; a day moved stays moved;
 *     hard days are kept apart (the nearer one becomes easy);
 *   - pauses: nothing in them is missed. 8 to 14 days repeat the last completed week, then the
 *     plan goes on (with no race date it shifts later; with one, the calendar stands and the
 *     next week is repeated); over 14 days, easy running with a long-day time trial until a
 *     trial is run, then, with no race date, the plan resumes at the week it was paused in;
 *   - recalibration: a time trial or race gives a 5K equivalent; faster applies, slower by
 *     more than 2% asks, a jump of more than 6% asks.
 *
 * plan_sim (Tools/plan_sim) prints scenarios of all of these.
 ******************************************************************************
 */

#ifndef XTRAINING_PLAN_RULES_HPP
#define XTRAINING_PLAN_RULES_HPP

#include "PaceModel.hpp"
#include "PlanSchedule.hpp"
#include "PlanState.hpp"

#include <cstdint>

namespace XTraining {
namespace PlanRules {

/// A 5K time from a trial more than this much (per cent) slower asks; any faster by more than
/// kJumpPercent asks too. PLANS.md 2.6: "slower by more than 2%", "more than about 6%".
constexpr uint32_t kSlowerAskPercent = 2;
constexpr uint32_t kJumpPercent      = 6;
/// With no race date the plan may grow by this many weeks (PLANS.md 4.3).
constexpr uint8_t kMaxGrowth = 2;
/// A pause of at least this many days repeats the week before; over kRetestDays, a retest.
constexpr uint32_t kRepeatPauseDays = 8;
constexpr uint32_t kRetestDays      = 14;

/// The weeks at the end of a plan with a race that a repeat never touches (PLANS.md 4.3).
uint8_t protectedWeeks(PlanId plan);

// ---- The day's session ---------------------------------------------------------------

/// The session on @p date: the plan's own, then the plan's memory applied. False if the date is
/// not in the plan.
bool sessionOn(const PlanState& st, const PlanDef& def, const Paces& paces, Ymd date, DaySession& out);

/// The seven days of run week @p runWeek, as sessionOn() has them.
bool week(const PlanState& st, const PlanDef& def, const Paces& paces, uint8_t runWeek, DaySession out[7]);

// ---- Settling ------------------------------------------------------------------------

struct Settled {
    uint8_t missed      = 0;      ///< Sessions logged Missed now.
    uint8_t keyMoved    = 0;      ///< Missed key sessions that took a later quality's place.
    bool    repeated    = false;  ///< A week was repeated.
    bool    nothingDone = false;  ///< ... because nothing was done, so the last cut-back week.
    uint8_t weekRepeated = 0;     ///< The run week that was repeated (0-based).
    /// A week less than half done that could not be repeated, and why.
    enum class Denied : uint8_t { None, LastWeeks, GrowthLimit, PlanEnd, NoRoom } denied = Denied::None;
    uint8_t weekDenied = 0;       ///< Its run week (0-based).
};

/// Settle every day before @p today (and before a pause) not settled yet. Idempotent for a day.
Settled settle(PlanState& st, const PlanDef& def, const Paces& paces, Ymd today);

/// TP8: "hold the plan". After sessions off pace the runner may choose to run this week's plan week
/// again next week, as a repeated week would (the same limits: not in the last weeks before a race,
/// the growth limit, the plan's end). False, with the reason in @p why, when it is not allowed;
/// NoRoom when the next week already repeats it (or the plan's adjustments are used up).
bool easeNextWeek(PlanState& st, const PlanDef& def, Ymd today, Settled::Denied* why = nullptr);
/// Whether easeNextWeek would be allowed, without doing it (so that it is not offered when it cannot be done).
bool canEase(const PlanState& st, const PlanDef& def, Ymd today, Settled::Denied* why = nullptr);

// ---- Offers --------------------------------------------------------------------------

struct Offer {
    enum class Kind : uint8_t { None, LongRun, KeySession };
    Kind    kind = Kind::None;
    Ymd     from = 0;                 ///< The missed day offered.
    Session session;                  ///< What it was, as the plan has it (level 2).
};

/// The offer for @p today, if any: yesterday's missed long run when today is easy or a rest day,
/// or a missed key session of this week (none later in it to take its place) when today is an
/// easier session. Not offered again once declined (PlanState::offerDeclined).
Offer offer(const PlanState& st, const PlanDef& def, const Paces& paces, Ymd today);

/// Accept: today runs the offered session. Decline: not offered again.
void acceptOffer(PlanState& st, const Offer& o, Ymd today);
void declineOffer(PlanState& st, const Offer& o);

// ---- Moves ---------------------------------------------------------------------------

enum class MoveResult : uint8_t {
    Ok,
    NothingToMove,  ///< No session on the day or the day after, or one already done.
    NotAllowed,     ///< A day already moved, or the move would lose a hard session.
};

/// Today runs tomorrow's session and tomorrow today's (hard days kept apart).
MoveResult doTomorrowToday(PlanState& st, const PlanDef& def, const Paces& paces, Ymd today);
/// Today's session runs tomorrow, and today rests; tomorrow's own session must not be a hard one.
MoveResult moveTodayTomorrow(PlanState& st, const PlanDef& def, const Paces& paces, Ymd today);
/// Skip today's session (logged Skipped); a skipped key session is treated as a missed one.
MoveResult skipToday(PlanState& st, const PlanDef& def, const Paces& paces, Ymd today);

// ---- Pauses --------------------------------------------------------------------------

/// Pause from @p today: until resumed, every day is a rest day and nothing is missed.
void pause(PlanState& st, Ymd today);

enum class Resumed : uint8_t {
    NotPaused,
    Resumed,       ///< Up to a week: the plan carries on where the calendar is.
    RepeatedWeek,  ///< 8 to 14 days: the week before is repeated.
    Retest,        ///< Over 14 days: easy running until a time trial.
};
Resumed resume(PlanState& st, const PlanDef& def, const Paces& paces, Ymd today, uint32_t* daysPaused = nullptr);

// ---- Recalibration -------------------------------------------------------------------

struct Recal {
    enum class Action : uint8_t {
        Keep,   ///< Not faster, and slower by no more than 2%: the paces stay.
        Apply,  ///< Faster: new paces from the next run.
        Ask,    ///< Slower by more than 2%, or a jump of more than 6%: ask the runner.
    };
    Action   action = Action::Keep;
    uint32_t fiveKSec = 0;   ///< The 5K equivalent of the result; 0 if it could not be worked out.
    bool     jump     = false;   ///< Ask because of a jump (faster by more than 6%), not slower.
};

/// What a time trial or race of @p distanceM in @p timeSec does to a 5K of @p currentFiveKSec.
Recal recalibrate(uint32_t currentFiveKSec, uint32_t distanceM, uint32_t timeSec);

/// A saved time trial (5K or 10K) or race: applies, asks or keeps, and clears a retest. Returns
/// the decision. @p raceM is the distance of a race (the plan's), used for Kind::Race.
Recal onTrial(PlanState& st, Kind kind, uint32_t raceM, uint32_t timeSec, Ymd today);

/// The runner's answer to a question (PlanState::askFiveKSec): update the paces or keep them.
void answerAsk(PlanState& st, bool update, Ymd today);

}  // namespace PlanRules
}  // namespace XTraining

#endif  // XTRAINING_PLAN_RULES_HPP
