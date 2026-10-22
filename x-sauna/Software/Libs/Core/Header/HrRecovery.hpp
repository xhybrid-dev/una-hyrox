/**
 ******************************************************************************
 * @file    HrRecovery.hpp
 * @brief   Heart-rate recovery after a phase: the heart rate when the phase
 *          ended, minus the heart rate a minute later. Pure C++, header-only,
 *          with the clock passed in.
 *
 * One Recovery belongs to one lap. begin() is called when the lap ends, with
 * the heart rate then (0 if there was none); feed() is called with every
 * later sample until it resolves:
 *   - the first valid sample from 60 s on, within kRecoveryWindowMs of it,
 *     gives the drop (positive: the heart rate fell; negative: it rose);
 *   - if none arrives in that window the result is NoResult, and so it is
 *     when there was no heart rate at the end, or the session stops first.
 * Times are the wrapping 32-bit millisecond clock: unsigned differences.
 ******************************************************************************
 */

#ifndef SAUNA_HR_RECOVERY_HPP
#define SAUNA_HR_RECOVERY_HPP

#include <cstdint>

namespace Sauna {

constexpr uint32_t kRecoveryDelayMs  = 60000;   ///< How long after the phase ends
constexpr uint32_t kRecoveryWindowMs = 5000;    ///< How late a sample may be
/// A heart rate older than this at the end of a phase is not "the" end rate.
constexpr uint32_t kRecoveryEndMaxAgeMs = 5000;

struct Recovery {
    enum class State : uint8_t {
        None = 0,    ///< Not started: the lap has not ended
        Pending,     ///< Waiting for the heart rate a minute on
        Done,        ///< `drop` holds the answer
        NoResult,    ///< No usable heart rate: show "--"
    };

    State    state = State::None;
    uint16_t endHr = 0;       ///< bpm when the phase ended
    int16_t  drop = 0;        ///< endHr minus the later heart rate, bpm
    uint32_t endMs = 0;       ///< When the phase ended

    /// The phase ended at @p nowMs with @p hrAtEnd bpm (0: none).
    void begin(uint32_t nowMs, uint16_t hrAtEnd)
    {
        endMs = nowMs;
        endHr = hrAtEnd;
        drop  = 0;
        state = hrAtEnd == 0 ? State::NoResult : State::Pending;
    }

    /// A sample at @p nowMs (@p hr 0: no heart rate in it).
    void feed(uint32_t nowMs, uint16_t hr)
    {
        if (state != State::Pending) {
            return;
        }
        const uint32_t since = nowMs - endMs;
        if (since < kRecoveryDelayMs) {
            return;
        }
        if (since > kRecoveryDelayMs + kRecoveryWindowMs) {
            state = State::NoResult;
        } else if (hr != 0) {
            int32_t d = static_cast<int32_t>(endHr) - static_cast<int32_t>(hr);
            if (d > INT16_MAX) {
                d = INT16_MAX;
            } else if (d < INT16_MIN) {
                d = INT16_MIN;
            }
            drop  = static_cast<int16_t>(d);
            state = State::Done;
        }
    }

    /// The session stopped: anything still waiting will not resolve.
    void giveUp()
    {
        if (state == State::Pending) {
            state = State::NoResult;
        }
    }
};

} // namespace Sauna

#endif // SAUNA_HR_RECOVERY_HPP
