/**
 ******************************************************************************
 * @file    SessionModel.hpp
 * @brief   A sauna session as a run of laps: Sauna, Plunge and (optionally)
 *          Rest, swapped by the lap button. Pure C++, header-only, no heap,
 *          with the clock passed in.
 *
 * The phase order on a lap press:
 *   - Sauna + Plunge:  Sauna -> Plunge -> (Rest) -> Sauna -> ...
 *   - Just sauna, Rest on:  Sauna <-> Rest
 *   - Just sauna, Rest off: Sauna -> Sauna, a new round each press
 * The round number counts the Sauna phases (the first is round 1).
 *
 * Time. The clock is the kernel's wrapping 32-bit millisecond count: every
 * difference is an unsigned subtraction. "Active" time excludes pauses;
 * "elapsed" is wall time from the start to now (or to the stop). The model
 * only moves when it is called, so callers pass the same clock to every call.
 *
 * Samples arrive about once a second (addSample): heart rate in bpm (0: none)
 * and the watch's temperature in hundredths of its own unit (kX100Invalid:
 * none). Per-lap statistics are integer sums. Each lap carries a Recovery
 * (HrRecovery.hpp) that starts when it ends.
 *
 * Bounds. At most kMaxLaps laps; the next lap press is refused (LapResult::Full)
 * and nothing changes. Sums are 32 or 64 bit and counters saturate.
 *
 * Size. The laps make the model about 4 KB (3.7 KB on the watch's CPU): keep one in static storage, not
 * on a stack.
 ******************************************************************************
 */

#ifndef SAUNA_SESSION_MODEL_HPP
#define SAUNA_SESSION_MODEL_HPP

#include <cstdint>

#include "FixedPoint.hpp"
#include "HeatHint.hpp"
#include "HrRecovery.hpp"

namespace Sauna {

constexpr uint8_t kMaxLaps = 64;

enum class Phase : uint8_t { Sauna = 0, Plunge, Rest };
enum class Mode : uint8_t { SaunaPlunge = 0, SaunaOnly };
enum class RunState : uint8_t { Idle = 0, Running, Paused, Stopped };
enum class LapResult : uint8_t { Ok = 0, NotRunning, Full };

inline const char *phaseName(Phase p)
{
    switch (p) {
    case Phase::Sauna:  return "SAUNA";
    case Phase::Plunge: return "PLUNGE";
    case Phase::Rest:   return "REST";
    }
    return "?";
}

struct SessionConfig {
    Mode           mode = Mode::SaunaPlunge;
    bool           rest = false;     ///< Include a Rest phase
    HeatHintConfig hint;             ///< Off by default
};

/// One lap: a stretch in one phase.
struct LapStats {
    Phase    phase = Phase::Sauna;
    uint8_t  round = 1;
    uint32_t activeMs = 0;

    uint32_t hrSum = 0;
    uint32_t hrCount = 0;
    uint16_t hrMax = 0;

    int32_t  tempMinX100 = kX100Invalid;
    int32_t  tempMaxX100 = kX100Invalid;
    int64_t  tempSumX100 = 0;
    uint32_t tempCount = 0;

    Recovery recovery;

    /// Mean heart rate, rounded; 0 with no samples.
    uint16_t hrAvg() const
    {
        return hrCount == 0 ? 0 : static_cast<uint16_t>((hrSum + hrCount / 2) / hrCount);
    }

    /// Mean temperature, hundredths; kX100Invalid with no samples.
    int32_t tempAvgX100() const
    {
        if (tempCount == 0) {
            return kX100Invalid;
        }
        const int64_t n = static_cast<int64_t>(tempCount);
        const int64_t s = tempSumX100;
        return static_cast<int32_t>((s >= 0 ? s + n / 2 : s - n / 2) / n);
    }
};

/// The whole session at a glance, for the summary screen.
struct SessionSummary {
    uint8_t  laps = 0;
    uint8_t  rounds = 0;        ///< Sauna phases
    uint32_t saunaMs = 0;
    uint32_t plungeMs = 0;
    uint32_t restMs = 0;
    uint16_t hrAvg = 0;         ///< Over every sample in the session
    uint16_t hrMax = 0;
    bool     hasRecovery = false;
    int16_t  bestDrop = 0;      ///< The largest drop among the resolved recoveries
};

class SessionModel {
public:
    SessionModel() = default;

    /// Begin a session at @p nowMs: running, in the first Sauna lap.
    void start(uint32_t nowMs, const SessionConfig &config)
    {
        // Field by field, not `*this = SessionModel()`: that would build a
        // second copy (about 4 KB of laps) on the service's small stack.
        mCfg      = config;
        mHint     = HeatHint(config.hint);
        mState    = RunState::Running;
        mStartMs  = nowMs;
        mLastMs   = nowMs;
        mStopMs   = 0;
        mActiveMs = 0;
        mHaveHr   = false;
        mLastHr   = 0;
        mLastHrMs = 0;
        mHrSum    = 0;
        mHrCount  = 0;
        mHrMax    = 0;
        mLapCount = 1;
        mLaps[0]  = LapStats();
    }

    /// Swap to the next phase. Refused when not running, or at kMaxLaps.
    LapResult lap(uint32_t nowMs)
    {
        if (mState != RunState::Running) {
            return LapResult::NotRunning;
        }
        if (mLapCount >= kMaxLaps) {
            return LapResult::Full;
        }
        advance(nowMs);

        LapStats &ending = mLaps[mLapCount - 1];
        const bool freshHr = mHaveHr && (nowMs - mLastHrMs) <= kRecoveryEndMaxAgeMs;
        ending.recovery.begin(nowMs, freshHr ? mLastHr : 0);

        LapStats next;
        next.phase = nextPhase(ending.phase);
        next.round = next.phase == Phase::Sauna ? static_cast<uint8_t>(ending.round + 1u) : ending.round;
        mLaps[mLapCount++] = next;
        return LapResult::Ok;
    }

    bool pause(uint32_t nowMs)
    {
        if (mState != RunState::Running) {
            return false;
        }
        advance(nowMs);
        mState = RunState::Paused;
        return true;
    }

    bool resume(uint32_t nowMs)
    {
        if (mState != RunState::Paused) {
            return false;
        }
        mLastMs = nowMs;
        mState  = RunState::Running;
        return true;
    }

    /// End the session. Recoveries still waiting will not resolve.
    bool stop(uint32_t nowMs)
    {
        if (mState != RunState::Running && mState != RunState::Paused) {
            return false;
        }
        if (mState == RunState::Running) {
            advance(nowMs);
        }
        for (uint8_t i = 0; i < mLapCount; ++i) {
            mLaps[i].recovery.giveUp();
        }
        mState  = RunState::Stopped;
        mStopMs = nowMs;
        return true;
    }

    /// A sample at @p nowMs: @p hr bpm (0 none), @p tempX100 (kX100Invalid none).
    /// Recoveries keep resolving while paused; lap statistics only while running.
    void addSample(uint32_t nowMs, uint16_t hr, int32_t tempX100)
    {
        if (mState != RunState::Running && mState != RunState::Paused) {
            return;
        }
        if (mState == RunState::Running) {
            advance(nowMs);
        }
        if (hr != 0) {
            mHaveHr   = true;
            mLastHr   = hr;
            mLastHrMs = nowMs;
        }
        for (uint8_t i = 0; i < mLapCount; ++i) {
            mLaps[i].recovery.feed(nowMs, hr);
        }
        if (mState != RunState::Running) {
            return;
        }

        LapStats &cur = mLaps[mLapCount - 1];
        if (hr != 0) {
            cur.hrSum += hr;
            ++cur.hrCount;
            if (hr > cur.hrMax) {
                cur.hrMax = hr;
            }
            mHrSum += hr;
            ++mHrCount;
            if (hr > mHrMax) {
                mHrMax = hr;
            }
        }
        if (tempX100 != kX100Invalid) {
            if (cur.tempCount == 0 || tempX100 < cur.tempMinX100) {
                cur.tempMinX100 = tempX100;
            }
            if (cur.tempCount == 0 || tempX100 > cur.tempMaxX100) {
                cur.tempMaxX100 = tempX100;
            }
            cur.tempSumX100 += tempX100;
            ++cur.tempCount;
            mHint.add(nowMs, tempX100);
        }
    }

    RunState state() const { return mState; }
    const SessionConfig &config() const { return mCfg; }

    uint8_t lapCount() const { return mLapCount; }
    /// Lap @p i, from 0; the last one is the current lap. Out of range gives the last.
    const LapStats &lapAt(uint8_t i) const
    {
        return mLaps[i < mLapCount ? i : (mLapCount > 0 ? mLapCount - 1 : 0)];
    }
    const LapStats &current() const { return lapAt(static_cast<uint8_t>(mLapCount > 0 ? mLapCount - 1 : 0)); }
    Phase phase() const { return current().phase; }
    uint8_t round() const { return current().round; }

    /// Active time of the session, pauses excluded.
    uint32_t activeMs(uint32_t nowMs) const
    {
        return mActiveMs + pending(nowMs);
    }
    /// Active time in the current lap.
    uint32_t lapActiveMs(uint32_t nowMs) const
    {
        return mLapCount == 0 ? 0 : current().activeMs + pending(nowMs);
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

    const HeatHint &hint() const { return mHint; }

    SessionSummary summary() const
    {
        SessionSummary s;
        s.laps = mLapCount;
        for (uint8_t i = 0; i < mLapCount; ++i) {
            const LapStats &l = mLaps[i];
            switch (l.phase) {
            case Phase::Sauna:
                s.saunaMs = sat(s.saunaMs, l.activeMs);
                ++s.rounds;
                break;
            case Phase::Plunge: s.plungeMs = sat(s.plungeMs, l.activeMs); break;
            case Phase::Rest:   s.restMs = sat(s.restMs, l.activeMs); break;
            }
            if (l.recovery.state == Recovery::State::Done &&
                (!s.hasRecovery || l.recovery.drop > s.bestDrop)) {
                s.hasRecovery = true;
                s.bestDrop    = l.recovery.drop;
            }
        }
        s.hrMax = mHrMax;
        s.hrAvg = mHrCount == 0 ? 0 : static_cast<uint16_t>((mHrSum + mHrCount / 2) / mHrCount);
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
        mLastMs = nowMs;
    }

    Phase nextPhase(Phase from) const
    {
        if (mCfg.mode == Mode::SaunaPlunge) {
            switch (from) {
            case Phase::Sauna:  return Phase::Plunge;
            case Phase::Plunge: return mCfg.rest ? Phase::Rest : Phase::Sauna;
            case Phase::Rest:   return Phase::Sauna;
            }
        } else {
            switch (from) {
            case Phase::Sauna:  return mCfg.rest ? Phase::Rest : Phase::Sauna;
            default:            return Phase::Sauna;
            }
        }
        return Phase::Sauna;
    }

    SessionConfig mCfg;
    HeatHint      mHint;
    RunState      mState = RunState::Idle;
    uint32_t      mStartMs = 0;
    uint32_t      mLastMs = 0;
    uint32_t      mStopMs = 0;
    uint32_t      mActiveMs = 0;

    bool          mHaveHr = false;
    uint16_t      mLastHr = 0;
    uint32_t      mLastHrMs = 0;
    uint32_t      mHrSum = 0;
    uint32_t      mHrCount = 0;
    uint16_t      mHrMax = 0;

    uint8_t       mLapCount = 0;
    LapStats      mLaps[kMaxLaps];
};

} // namespace Sauna

#endif // SAUNA_SESSION_MODEL_HPP
