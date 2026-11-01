/**
 ******************************************************************************
 * @file    PlanRunner.hpp
 * @brief   Runs a SessionPlan over a WorkoutModel: ends timed steps when their
 *          time is up, ends open steps on the button, labels each lap with
 *          its step, and says what to buzz. Pure C++, header-only, no heap,
 *          with the clock passed in.
 *
 * Who calls it. The service, never the GUI: the timers must keep running if
 * the screen closes mid-workout. The service calls tick() with each sample
 * (about once a second, after WorkoutModel::addSample) and press() for the lap
 * button (after its LapLockout), and turns the returned cue into a buzz.
 *
 * The clock. Steps are measured in the model's active time, so a pause stops
 * the plan too. A timed step's end is anchored to its planned time, not to the
 * tick that noticed it: a tick up to a second late makes that lap up to a
 * second long and the next one as much short, and the plan never drifts (60
 * EMOM minutes end at 60:00 of active time, give or take one tick).
 *
 * The end. After the last step the runner pauses the model and is done; the
 * last lap is the last step. The service then offers End and Discard, not
 * Resume (resuming would stretch the last step's lap).
 *
 * A plain workout (SessionType::Plain) has no plan: press() takes a lap and
 * tick() does nothing; the model's own auto lap works as in W0.
 ******************************************************************************
 */

#ifndef WORKOUT_PLAN_RUNNER_HPP
#define WORKOUT_PLAN_RUNNER_HPP

#include <cstdint>

#include "SessionPlan.hpp"
#include "WorkoutModel.hpp"

namespace Workout {

/// What just happened, for the service to buzz and the GUI to show.
enum class Cue : uint8_t {
    None = 0,
    Work,       ///< A work step began (a set, a minute, an interval)
    Rest,       ///< A rest began
    Run,        ///< A circuit run began
    Station,    ///< A circuit station began
    Warning,    ///< kWarnMs left in a timed step
    Round,      ///< An AMRAP round was counted (or a plain lap taken)
    Done,       ///< The plan is over; the workout is paused
};

/// The plan as the screens show it.
struct PlanView {
    uint8_t  type = 0;          ///< SessionType
    uint8_t  kind = 0;          ///< StepKind of the current step
    uint8_t  step = 0;          ///< Current step, from 1 (0: plain)
    uint8_t  steps = 0;         ///< Steps in the plan (0: plain)
    uint8_t  round = 0;         ///< Set, round or minute, from 1
    uint8_t  rounds = 0;
    uint8_t  station = 0;       ///< Circuit station, from 1; 0 none
    uint8_t  done = 0;
    uint16_t amrapRounds = 0;   ///< AMRAP rounds counted so far
    uint16_t distanceM = 0;     ///< The step's planned metres; 0 none
    uint32_t stepMs = 0;        ///< Active time into the step
    uint32_t remainingMs = 0;   ///< Until the step ends by itself; 0 when open
};

class PlanRunner {
public:
    /// Start the workout and its plan at @p nowMs. A plan switches the auto lap off.
    void start(const PlanConfig &plan, WorkoutModel &m, uint32_t nowMs, WorkoutConfig config)
    {
        mCfg   = clamp(plan);
        mCount = stepCount(mCfg);
        if (mCount > 0) {
            config.autoLapMs = 0;
        }
        m.start(nowMs, config);
        mStep        = 0;
        mStepStartMs = 0;
        mWarned      = false;
        mDone        = false;
        mRounds      = 0;
        label(m);
    }

    /// The lap button. The timers (EMOM, Tabata, custom) ignore it.
    Cue press(WorkoutModel &m, uint32_t nowMs)
    {
        if (m.state() != RunState::Running || mDone) {
            return Cue::None;
        }
        if (mCount == 0) {
            return m.lap(nowMs) == LapResult::Ok ? Cue::Round : Cue::None;
        }
        switch (mCfg.type) {
        case SessionType::Amrap:
            if (mRounds < UINT16_MAX) {
                ++mRounds;
            }
            if (m.lap(nowMs) == LapResult::Ok) {
                m.setLapInfo(static_cast<uint8_t>(StepKind::Work), roundByte(mRounds + 1u), 0);
            }
            return Cue::Round;
        case SessionType::Strength:
        case SessionType::Circuit:
            return finishStep(m, nowMs, m.activeMs(nowMs), false);
        default:
            return Cue::None;
        }
    }

    /// Once a second (after the model's sample): ends a step whose time is up,
    /// or warns that it soon will be.
    Cue tick(WorkoutModel &m, uint32_t nowMs)
    {
        if (mCount == 0 || mDone || m.state() != RunState::Running) {
            return Cue::None;
        }
        const uint32_t active = m.activeMs(nowMs);
        Cue cue = Cue::None;
        // A long gap can cross more than one step; each is ended in turn.
        for (uint8_t guard = 0; guard < mCount && !mDone; ++guard) {
            const Step s = stepAt(mCfg, mStep);
            const uint32_t ends = s.endsAfterMs();
            if (ends == 0 || active - mStepStartMs < ends) {
                break;
            }
            cue = finishStep(m, nowMs, mStepStartMs + ends, true);
        }
        if (cue != Cue::None) {
            return cue;
        }
        const Step s = stepAt(mCfg, mStep);
        if (s.timed() && !mWarned && s.durationMs > kWarnMs) {
            const uint32_t into = active - mStepStartMs;
            if (into < s.durationMs && s.durationMs - into <= kWarnMs) {
                mWarned = true;
                return Cue::Warning;
            }
        }
        return Cue::None;
    }

    bool done() const { return mDone; }
    bool hasPlan() const { return mCount > 0; }
    const PlanConfig &config() const { return mCfg; }
    uint16_t amrapRounds() const { return mRounds; }

    PlanView view(const WorkoutModel &m, uint32_t nowMs) const
    {
        PlanView v;
        v.type = static_cast<uint8_t>(mCfg.type);
        if (mCount == 0) {
            return v;
        }
        const Step s  = stepAt(mCfg, mStep);
        v.kind        = static_cast<uint8_t>(s.kind);
        v.step        = static_cast<uint8_t>(mStep + 1);
        v.steps       = mCount;
        v.round       = mCfg.type == SessionType::Amrap ? roundByte(mRounds + 1u) : s.round;
        v.rounds      = s.rounds;
        v.station     = s.station;
        v.done        = mDone ? 1 : 0;
        v.amrapRounds = mRounds;
        v.distanceM   = s.distanceM;
        const uint32_t active = m.activeMs(nowMs);
        v.stepMs = active - mStepStartMs;
        const uint32_t ends = s.endsAfterMs();
        if (!mDone && ends > v.stepMs) {
            v.remainingMs = ends - v.stepMs;
        }
        return v;
    }

private:
    static uint8_t roundByte(uint32_t r) { return r > 255u ? 255u : static_cast<uint8_t>(r); }

    /// Label the current lap with the current step.
    void label(WorkoutModel &m) const
    {
        if (mCount == 0) {
            return;
        }
        const Step s = stepAt(mCfg, mStep);
        m.setLapInfo(static_cast<uint8_t>(s.kind),
                     mCfg.type == SessionType::Amrap ? 1 : s.round, s.station);
    }

    /// End the current step; the next begins at @p nextStartMs of active time.
    Cue finishStep(WorkoutModel &m, uint32_t nowMs, uint32_t nextStartMs, bool byTime)
    {
        if (mStep + 1u >= mCount) {
            mDone = true;
            m.pause(nowMs);          // the last lap stays the last step
            return Cue::Done;
        }
        const LapResult r = byTime ? m.lapTimed(nowMs) : m.lap(nowMs);
        if (r != LapResult::Ok) {
            return Cue::None;        // not running, or no room: nothing changes
        }
        ++mStep;
        mStepStartMs = nextStartMs;
        mWarned      = false;
        label(m);
        switch (stepAt(mCfg, mStep).kind) {
        case StepKind::Rest:    return Cue::Rest;
        case StepKind::Run:     return Cue::Run;
        case StepKind::Station: return Cue::Station;
        case StepKind::Work:
        default:                return Cue::Work;
        }
    }

    PlanConfig mCfg;
    uint8_t    mCount = 0;
    uint8_t    mStep = 0;
    uint32_t   mStepStartMs = 0;   ///< In the model's active time
    bool       mWarned = false;
    bool       mDone = false;
    uint16_t   mRounds = 0;        ///< AMRAP rounds counted
};

} // namespace Workout

#endif // WORKOUT_PLAN_RUNNER_HPP
