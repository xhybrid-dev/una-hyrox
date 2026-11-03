/**
 ******************************************************************************
 * @file    PlanRunner.hpp
 * @brief   Runs a RidePlan over a RideModel (HybridX Workout's PlanRunner, W1,
 *          cut to a ride's timed plans): ends timed steps when their
 *          time is up, ends open steps on the button, labels each lap with
 *          its step, and says what to buzz. Pure C++, header-only, no heap,
 *          with the clock passed in.
 *
 * Who calls it. The service, never the GUI: the timers must keep running if
 * the screen closes mid-workout. The service calls tick() with each sample
 * (about once a second, after RideModel::addSample) and press() for the lap
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

#ifndef RIDE_PLAN_RUNNER_HPP
#define RIDE_PLAN_RUNNER_HPP

#include <cstdint>

#include "RideModel.hpp"
#include "RidePlan.hpp"

namespace Ride {

/// What just happened, for the service to buzz and the GUI to show.
enum class Cue : uint8_t {
    None = 0,
    Work,       ///< A work step began (a set, a minute, an interval)
    Rest,       ///< A rest began
    Warning,    ///< kWarnMs left in a timed step
    Round,      ///< A free ride's lap was taken
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
    uint8_t  done = 0;
    uint32_t stepMs = 0;        ///< Active time into the step
    uint32_t remainingMs = 0;   ///< Until the step ends by itself; 0 when open
};

class PlanRunner {
public:
    /// Start the workout and its plan at @p nowMs. A plan switches the auto lap off.
    void start(const PlanConfig &plan, RideModel &m, uint32_t nowMs, RideConfig config)
    {
        mCfg   = clamp(plan);
        mCount = stepCount(mCfg);
        if (mCount > 0) {
            config.autoLapMs     = 0;   // a plan's steps are its laps
            config.autoLapDistCm = 0;
        }
        m.start(nowMs, config);
        mStep        = 0;
        mStepStartMs = 0;
        mWarned      = false;
        mDone        = false;
        label(m);
    }

    /// The lap button. A free ride takes a lap; the timers ignore it.
    Cue press(RideModel &m, uint32_t nowMs)
    {
        if (m.state() != RunState::Running || mDone) {
            return Cue::None;
        }
        if (mCount == 0) {
            return m.lap(nowMs) == LapResult::Ok ? Cue::Round : Cue::None;
        }
        return Cue::None;
    }

    /// Once a second (after the model's sample): ends a step whose time is up,
    /// or warns that it soon will be.
    Cue tick(RideModel &m, uint32_t nowMs)
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

    PlanView view(const RideModel &m, uint32_t nowMs) const
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
        v.round       = s.round;
        v.rounds      = s.rounds;
        v.done        = mDone ? 1 : 0;
        const uint32_t active = m.activeMs(nowMs);
        v.stepMs = active - mStepStartMs;
        const uint32_t ends = s.endsAfterMs();
        if (!mDone && ends > v.stepMs) {
            v.remainingMs = ends - v.stepMs;
        }
        return v;
    }

private:
    /// Label the current lap with the current step.
    void label(RideModel &m) const
    {
        if (mCount == 0) {
            return;
        }
        const Step s = stepAt(mCfg, mStep);
        m.setLapInfo(static_cast<uint8_t>(s.kind), s.round);
    }

    /// End the current step; the next begins at @p nextStartMs of active time.
    Cue finishStep(RideModel &m, uint32_t nowMs, uint32_t nextStartMs, bool byTime)
    {
        if (mStep + 1u >= mCount) {
            mDone = true;
            if (byTime) {
                m.markLapTimed();
            }
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
        return stepAt(mCfg, mStep).kind == StepKind::Rest ? Cue::Rest : Cue::Work;
    }

    PlanConfig mCfg;
    uint8_t    mCount = 0;
    uint8_t    mStep = 0;
    uint32_t   mStepStartMs = 0;   ///< In the model's active time
    bool       mWarned = false;
    bool       mDone = false;
};

} // namespace Ride

#endif // RIDE_PLAN_RUNNER_HPP
