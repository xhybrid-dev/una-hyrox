/**
 ******************************************************************************
 * @file    WorkoutRunner.hpp
 * @brief   One workout being run: the engine, the live target check and the
 *          cues, in one place the service drives once a second (Phase P3b).
 *
 * The service hands over the activity's **active** time and distance (pauses
 * already taken out by its counters), so a paused workout stands still with
 * no special case. Everything the service must act on comes back from
 * tick()/advance() as a TickResult: a lap to save (each step is its own lap),
 * a cue to buzz, the workout finishing. Everything the screen shows comes
 * from view().
 *
 * Cues (proposed defaults, NOTES P3b): a buzz when the debounced state goes
 * Under or Over; a reminder every kRemindMs while it stays out; nothing for
 * coming back in (the screen shows it). No cue in the first kSettleMs of a
 * step, while pace and heart rate catch up with the change of effort.
 ******************************************************************************
 */

#ifndef INTERVALS_WORKOUT_RUNNER_HPP
#define INTERVALS_WORKOUT_RUNNER_HPP

#include <cstdint>

#include "TargetEvaluator.hpp"
#include "WorkoutEngine.hpp"
#include "WorkoutTypes.hpp"

namespace Intervals
{

class WorkoutRunner {
public:
    static constexpr uint32_t kSettleMs = 15000;
    static constexpr uint32_t kRemindMs = 60000;

    struct TickResult {
        bool      stepEnded   = false;   ///< save a lap for endedStep
        uint8_t   endedStep   = 0;       ///< the step (and FIT workout_step index) that ended
        bool      stepStarted = false;   ///< a new step began (alert the athlete)
        bool      completed   = false;   ///< the last step ended
        bool      cue         = false;   ///< buzz: off target (cueState Under or Over)
        ZoneState cueState    = ZoneState::NoTarget;
        bool      reminder    = false;   ///< the cue is a reminder, not a change
    };

    struct View {
        bool        running     = false;   ///< started and not completed
        bool        completed   = false;
        uint8_t     stepIndex   = 0;
        const Step* step        = nullptr; ///< non-owning; null unless running
        int16_t     nextIndex   = -1;      ///< the step after this one, -1 if last
        uint32_t    elapsedMs   = 0;       ///< in this step
        uint32_t    remainingMs = 0;       ///< Time steps only
        uint32_t    remainingCm = 0;       ///< Distance steps only
        uint16_t    pass        = 0;       ///< repeat block: 1-based pass, 0 outside a block
        uint16_t    passes      = 0;
        ZoneState   zone        = ZoneState::NoTarget;   ///< debounced, for colour
        ZoneState   zoneLive    = ZoneState::NoTarget;   ///< this tick's raw classification
        bool        settling    = false;   ///< within kSettleMs of the step's start
    };

    /// `workout` must have passed validate() and outlive the runner.
    TickResult start(const Workout& workout, uint32_t activeMs, uint32_t distanceCm);
    /// Once a second while the activity runs (not while paused).
    TickResult tick(uint32_t activeMs, uint32_t distanceCm, const Sample& sample);
    /// The athlete pressed "next step": the only way an Open step ends.
    TickResult advance(uint32_t activeMs, uint32_t distanceCm);
    /// Forget the workout (the activity ended or was discarded).
    void stop();

    bool active() const { return mWorkout != nullptr; }
    bool completed() const { return mEngine.completed(); }
    View view(uint32_t activeMs, uint32_t distanceCm) const;

private:
    void absorb(const Events& events, uint32_t activeMs, TickResult& out);

    const Workout* mWorkout = nullptr;
    WorkoutEngine  mEngine;
    CueDebouncer   mDebouncer;
    ZoneState      mLive        = ZoneState::NoTarget;
    uint32_t       mStepStartMs = 0;
    uint32_t       mStepStartCm = 0;
    uint32_t       mLastCueMs   = 0;
    bool           mOutOfZone   = false;
};

} // namespace Intervals

#endif // INTERVALS_WORKOUT_RUNNER_HPP
