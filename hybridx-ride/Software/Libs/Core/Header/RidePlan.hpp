/**
 ******************************************************************************
 * @file    RidePlan.hpp
 * @brief   Indoor interval plans on the turbo (Jon, 2 November 2026: Intervals
 *          and Tabata): HybridX Workout's SessionPlan (W1), cut to the kinds a
 *          ride offers. Pure C++, header-only (the GUI uses it), no heap.
 *
 * A plan is a list of steps worked out from a few numbers; each step is one
 * lap. All of a ride's steps are timed: a work or rest interval that ends by
 * itself (PlanRunner.hpp). There is no rest after the last work interval.
 *   - Free: no plan (laps on the button, the auto lap).
 *   - Intervals: work, rest and rounds (default 40 s, 20 s, 6).
 *   - Tabata: 8 rounds of 20 s work and 10 s rest, fixed.
 *
 * Indoors only: outdoors the road decides. Built so that a power or cadence
 * target can be added to a step when UNA's sensor support arrives (Ride NOTES,
 * section 1): a step is data, so a target is one more field.
 *
 * Bounds: every setting has a range; clamp() pulls any value into it, so no
 * plan has more steps than RideModel's 64 laps (the most is 30 rounds: 59).
 ******************************************************************************
 */

#ifndef RIDE_RIDE_PLAN_HPP
#define RIDE_RIDE_PLAN_HPP

#include <cstdint>

#include "RideModel.hpp"

namespace Ride {

enum class SessionType : uint8_t { Plain = 0, Custom, Tabata };
constexpr uint8_t kSessionTypeCount = 3;

enum class StepKind : uint8_t { Work = 0, Rest };

constexpr uint16_t kWorkSecMin = 5,  kWorkSecMax = 600, kWorkSecDefault = 40;
constexpr uint16_t kRestSecMin = 5,  kRestSecMax = 600, kRestSecDefault = 20;
constexpr uint8_t  kRoundsMin  = 1,  kRoundsMax  = 30,  kRoundsDefault  = 6;

constexpr uint8_t  kTabataRounds  = 8;
constexpr uint16_t kTabataWorkSec = 20;
constexpr uint16_t kTabataRestSec = 10;

/// The warning before a timed step ends.
constexpr uint32_t kWarnMs = 3000;

struct PlanConfig {
    SessionType type = SessionType::Plain;
    uint16_t workSec = kWorkSecDefault;
    uint16_t restSec = kRestSecDefault;
    uint8_t  rounds  = kRoundsDefault;
};

template <typename T>
constexpr T clampTo(T v, T lo, T hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

constexpr PlanConfig clamp(PlanConfig c)
{
    if (static_cast<uint8_t>(c.type) >= kSessionTypeCount) {
        c.type = SessionType::Plain;
    }
    c.workSec = clampTo<uint16_t>(c.workSec, kWorkSecMin, kWorkSecMax);
    c.restSec = clampTo<uint16_t>(c.restSec, kRestSecMin, kRestSecMax);
    c.rounds  = clampTo<uint8_t>(c.rounds, kRoundsMin, kRoundsMax);
    return c;
}

struct Step {
    StepKind kind = StepKind::Work;
    uint32_t durationMs = 0;   ///< 0: open (a free ride has no steps)
    uint8_t  round = 0;        ///< from 1
    uint8_t  rounds = 0;

    constexpr bool timed() const { return durationMs > 0; }
    constexpr uint32_t endsAfterMs() const { return durationMs; }
};

constexpr uint8_t stepCount(const PlanConfig &raw)
{
    const PlanConfig c = clamp(raw);
    switch (c.type) {
    case SessionType::Custom: return static_cast<uint8_t>(2 * c.rounds - 1);
    case SessionType::Tabata: return static_cast<uint8_t>(2 * kTabataRounds - 1);
    case SessionType::Plain:
    default:                  return 0;
    }
}

constexpr Step stepAt(const PlanConfig &raw, uint8_t i)
{
    const PlanConfig c = clamp(raw);
    Step s;
    if (i >= stepCount(c)) {
        return s;
    }
    const bool odd = (i % 2) == 1;
    const bool tabata = c.type == SessionType::Tabata;
    s.round  = static_cast<uint8_t>(i / 2 + 1);
    s.rounds = tabata ? kTabataRounds : c.rounds;
    s.kind   = odd ? StepKind::Rest : StepKind::Work;
    const uint16_t sec = tabata ? (odd ? kTabataRestSec : kTabataWorkSec) : (odd ? c.restSec : c.workSec);
    s.durationMs = static_cast<uint32_t>(sec) * 1000u;
    return s;
}

/// The plan's length in active time (0 for a free ride).
constexpr uint32_t plannedMs(const PlanConfig &c)
{
    uint32_t total = 0;
    const uint8_t n = stepCount(c);
    for (uint8_t i = 0; i < n; ++i) {
        total += stepAt(c, i).durationMs;
    }
    return total;
}

static_assert(2 * kRoundsMax - 1 <= kMaxLaps, "an interval plan must fit the laps");

} // namespace Ride

#endif // RIDE_RIDE_PLAN_HPP
