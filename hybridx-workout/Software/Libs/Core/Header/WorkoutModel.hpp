/**
 ******************************************************************************
 * @file    WorkoutModel.hpp
 * @brief   A workout as a run of laps, with heart-rate zones and calories as
 *          the SDK's Workout example keeps them. Pure C++, header-only, no
 *          heap, with the clock passed in. (X Sauna's SessionModel without the
 *          phases, temperature and recovery.)
 *
 * Time. The clock is the kernel's wrapping 32-bit millisecond count: every
 * difference is an unsigned subtraction. "Active" time excludes pauses;
 * "elapsed" is wall time from the start to now (or to the stop). The model
 * only moves when it is called, so callers pass the same clock to every call.
 *
 * Samples arrive about once a second (addSample): heart rate in bpm (0: none).
 * Active time goes to the zone of the latest sample (HrZones.hpp), so a
 * second's time counts in the zone known when it began. Calories are worked
 * out from the time in each zone and the weight, per lap and for the session.
 *
 * Auto lap (the example's "time alert"): with WorkoutConfig::autoLapMs set, a
 * sample that finds the lap's active time at or past it ends the lap there.
 *
 * Bounds. At most kMaxLaps laps; the next lap is refused (LapResult::Full)
 * and nothing changes. Sums are 32 or 64 bit and counters saturate.
 *
 * Size. The laps make the model about 3.6 KB: keep one in static storage, not
 * on a stack.
 ******************************************************************************
 */

#ifndef WORKOUT_WORKOUT_MODEL_HPP
#define WORKOUT_WORKOUT_MODEL_HPP

#include <cstdint>

#include "HrZones.hpp"

namespace Workout {

constexpr uint8_t kMaxLaps = 64;

enum class RunState : uint8_t { Idle = 0, Running, Paused, Stopped };
enum class LapResult : uint8_t { Ok = 0, NotRunning, Full };

/// The example's auto-lap choices, in minutes (0: off).
constexpr uint8_t kAutoLapCount = 7;
constexpr uint8_t kAutoLapMinutes[kAutoLapCount] = {0, 1, 5, 10, 15, 20, 30};

struct WorkoutConfig {
    HrZones  zones;
    uint32_t weightG = kDefaultWeightG;
    uint32_t autoLapMs = 0;           ///< 0: off
};

/// One lap.
struct LapStats {
    uint32_t activeMs = 0;
    uint32_t pausedMs = 0;            ///< Wall time spent paused inside this lap
    uint32_t hrSum = 0;
    uint32_t hrCount = 0;
    uint16_t hrMax = 0;
    bool     autoEnded = false;       ///< Ended by the auto lap, not the button
    uint32_t zoneMs[kZoneCount] = {}; ///< Active time in each zone (0: below zone 1 or no HR)

    /// Mean heart rate, rounded; 0 with no samples.
    uint16_t hrAvg() const
    {
        return hrCount == 0 ? 0 : static_cast<uint16_t>((hrSum + hrCount / 2) / hrCount);
    }
};

/// The whole workout at a glance, for the summary screen.
struct SessionSummary {
    uint8_t  laps = 0;
    uint16_t hrAvg = 0;               ///< Over every sample in the session
    uint16_t hrMax = 0;
    uint32_t zoneMs[kZoneCount] = {};
    uint32_t kcalX10 = 0;             ///< Calories, tenths of a kcal
    uint32_t restingKcalX10 = 0;      ///< The resting part of kcalX10
};

class WorkoutModel {
public:
    WorkoutModel() = default;

    /// Begin a workout at @p nowMs: running, in lap 1.
    void start(uint32_t nowMs, const WorkoutConfig &config)
    {
        // Field by field, not `*this = WorkoutModel()`: that would build a
        // second copy (about 3.6 KB of laps) on the service's small stack.
        mCfg      = config;
        mState    = RunState::Running;
        mStartMs  = nowMs;
        mLastMs   = nowMs;
        mStopMs   = 0;
        mPausedAt = 0;
        mActiveMs = 0;
        mZone     = 0;
        mHrSum    = 0;
        mHrCount  = 0;
        mHrMax    = 0;
        mLapCount = 1;
        mLaps[0]  = LapStats();
    }

    /// End this lap and begin the next. Refused when not running, or at kMaxLaps.
    LapResult lap(uint32_t nowMs)
    {
        return endLap(nowMs, false);
    }

    bool pause(uint32_t nowMs)
    {
        if (mState != RunState::Running) {
            return false;
        }
        advance(nowMs);
        mState    = RunState::Paused;
        mPausedAt = nowMs;
        return true;
    }

    bool resume(uint32_t nowMs)
    {
        if (mState != RunState::Paused) {
            return false;
        }
        LapStats &cur = mLaps[mLapCount - 1];
        cur.pausedMs  = sat(cur.pausedMs, nowMs - mPausedAt);
        mLastMs = nowMs;
        mState  = RunState::Running;
        return true;
    }

    bool stop(uint32_t nowMs)
    {
        if (mState != RunState::Running && mState != RunState::Paused) {
            return false;
        }
        if (mState == RunState::Running) {
            advance(nowMs);
        } else {
            LapStats &cur = mLaps[mLapCount - 1];     // stopped while paused
            cur.pausedMs  = sat(cur.pausedMs, nowMs - mPausedAt);
        }
        mState  = RunState::Stopped;
        mStopMs = nowMs;
        return true;
    }

    /**
     * @brief   A sample at @p nowMs: @p hr bpm (0 none). Counts only while running.
     * @return  true when it ended the lap by the auto lap.
     */
    bool addSample(uint32_t nowMs, uint16_t hr)
    {
        if (mState != RunState::Running) {
            return false;
        }
        advance(nowMs);   // the time since the last call, in the zone known then
        mZone = mCfg.zones.zoneOf(hr);

        LapStats &cur = mLaps[mLapCount - 1];
        if (hr != 0) {
            cur.hrSum = sat(cur.hrSum, hr);
            cur.hrCount = sat(cur.hrCount, 1u);
            if (hr > cur.hrMax) {
                cur.hrMax = hr;
            }
            mHrSum = sat(mHrSum, hr);
            mHrCount = sat(mHrCount, 1u);
            if (hr > mHrMax) {
                mHrMax = hr;
            }
        }
        if (mCfg.autoLapMs > 0 && cur.activeMs >= mCfg.autoLapMs) {
            return endLap(nowMs, true) == LapResult::Ok;
        }
        return false;
    }

    RunState state() const { return mState; }
    const WorkoutConfig &config() const { return mCfg; }
    /// The zone active time is going to now.
    uint8_t zone() const { return mZone; }

    uint8_t lapCount() const { return mLapCount; }
    /// Lap @p i, from 0; the last one is the current lap. Out of range gives the last.
    const LapStats &lapAt(uint8_t i) const
    {
        return mLaps[i < mLapCount ? i : (mLapCount > 0 ? mLapCount - 1 : 0)];
    }
    const LapStats &current() const { return lapAt(static_cast<uint8_t>(mLapCount > 0 ? mLapCount - 1 : 0)); }

    /// Active time of the workout, pauses excluded.
    uint32_t activeMs(uint32_t nowMs) const
    {
        return sat(mActiveMs, pending(nowMs));
    }
    /// Active time in the current lap.
    uint32_t lapActiveMs(uint32_t nowMs) const
    {
        return mLapCount == 0 ? 0 : sat(current().activeMs, pending(nowMs));
    }
    /// Wall time since the start (to the stop, once stopped).
    uint32_t elapsedMs(uint32_t nowMs) const
    {
        switch (mState) {
        case RunState::Idle:    return 0;
        case RunState::Stopped: return mStopMs - mStartMs;
        default:                return nowMs - mStartMs;
        }
    }

    /// Calories of lap @p i so far, tenths of a kcal.
    uint32_t lapKcalX10(uint8_t i) const { return caloriesX10(lapAt(i).zoneMs, mCfg.weightG); }
    uint32_t lapRestingKcalX10(uint8_t i) const
    {
        return restingCaloriesX10(lapAt(i).activeMs, mCfg.weightG);
    }

    SessionSummary summary() const
    {
        SessionSummary s;
        s.laps = mLapCount;
        uint32_t active = 0;
        for (uint8_t i = 0; i < mLapCount; ++i) {
            const LapStats &l = mLaps[i];
            for (uint8_t z = 0; z < kZoneCount; ++z) {
                s.zoneMs[z] = sat(s.zoneMs[z], l.zoneMs[z]);
            }
            active = sat(active, l.activeMs);
        }
        s.hrMax          = mHrMax;
        s.hrAvg          = mHrCount == 0 ? 0 : static_cast<uint16_t>((mHrSum + mHrCount / 2) / mHrCount);
        s.kcalX10        = caloriesX10(s.zoneMs, mCfg.weightG);
        s.restingKcalX10 = restingCaloriesX10(active, mCfg.weightG);
        return s;
    }

private:
    static uint32_t sat(uint32_t a, uint32_t b)
    {
        return a > UINT32_MAX - b ? UINT32_MAX : a + b;
    }

    /// Active time since the last call, while running.
    uint32_t pending(uint32_t nowMs) const
    {
        return mState == RunState::Running ? nowMs - mLastMs : 0u;
    }

    /// Bring the active clocks up to @p nowMs (running only).
    void advance(uint32_t nowMs)
    {
        if (mState != RunState::Running) {
            return;
        }
        const uint32_t dt = nowMs - mLastMs;
        mActiveMs = sat(mActiveMs, dt);
        LapStats &cur = mLaps[mLapCount - 1];
        cur.activeMs = sat(cur.activeMs, dt);
        cur.zoneMs[mZone < kZoneCount ? mZone : 0] = sat(cur.zoneMs[mZone < kZoneCount ? mZone : 0], dt);
        mLastMs = nowMs;
    }

    LapResult endLap(uint32_t nowMs, bool autoEnded)
    {
        if (mState != RunState::Running) {
            return LapResult::NotRunning;
        }
        if (mLapCount >= kMaxLaps) {
            return LapResult::Full;
        }
        advance(nowMs);
        mLaps[mLapCount - 1].autoEnded = autoEnded;
        mLaps[mLapCount++] = LapStats();
        return LapResult::Ok;
    }

    WorkoutConfig mCfg;
    RunState      mState = RunState::Idle;
    uint32_t      mStartMs = 0;
    uint32_t      mLastMs = 0;
    uint32_t      mStopMs = 0;
    uint32_t      mPausedAt = 0;
    uint32_t      mActiveMs = 0;
    uint8_t       mZone = 0;

    uint32_t      mHrSum = 0;
    uint32_t      mHrCount = 0;
    uint16_t      mHrMax = 0;

    uint8_t       mLapCount = 0;
    LapStats      mLaps[kMaxLaps];
};

} // namespace Workout

#endif // WORKOUT_WORKOUT_MODEL_HPP
