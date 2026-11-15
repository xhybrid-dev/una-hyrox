/**
 ******************************************************************************
 * @file    RepJudge.hpp
 * @brief   A short rep judged when it ends (brief 8.2; PLANS.md 2.5): strides,
 *          repetitions and 30-30s are too short for live alerts, so the next
 *          step's card says how the last one went. Header-only, pure.
 *
 * A distance rep that ran its distance is judged by its time ("1:36" against
 * 1:34-1:40 for 400 m); any other rep, by its average pace against the step's
 * band. Under kMinMetres there is nothing to judge (no GPS, or a lap pressed
 * at once).
 ******************************************************************************
 */

#ifndef XTRAINING_REP_JUDGE_HPP
#define XTRAINING_REP_JUDGE_HPP

#include "SDK/Workout/WorkoutProgram.hpp"

#include <cstdint>

namespace XTraining {
namespace RepJudge {

enum class Verdict : uint8_t { None = 0, InBand, Slow, Fast };

struct Result {
    bool     ok         = false;   ///< enough to judge
    bool     byTime     = false;   ///< value is the rep's time; else its pace, s/km
    uint32_t value      = 0;       ///< seconds, or s/km
    uint32_t fastEnd    = 0;       ///< the target's fast end in the same unit; 0 for no target
    uint32_t slowEnd    = 0;
    Verdict  verdict    = Verdict::None;
};

constexpr uint32_t kMinMetres = 20;

/// @p sec and @p metres are the rep's own (its lap); @p ranOut is true when the step's
/// time or distance ran out rather than the lap button ending it.
inline Result judge(const SDK::Workout::Step& s, uint32_t sec, uint32_t metres, bool ranOut)
{
    Result r;
    if (sec == 0 || metres < kMinMetres) {
        return r;
    }
    r.ok = true;
    const bool band = s.target == SDK::Workout::Target::Speed && s.speedLowMmps > 0 && s.speedHighMmps > 0;
    const uint64_t slowMmps = band ? (s.speedLowMmps < s.speedHighMmps ? s.speedLowMmps : s.speedHighMmps) : 0;
    const uint64_t fastMmps = band ? (s.speedLowMmps < s.speedHighMmps ? s.speedHighMmps : s.speedLowMmps) : 0;
    if (s.end == SDK::Workout::StepEnd::Distance && ranOut && s.distanceCm > 0) {
        r.byTime = true;
        r.value  = sec;
        if (band) {
            // t = distance / speed: cm * 10 / (mm/s), rounded.
            const uint64_t mmTimes = static_cast<uint64_t>(s.distanceCm) * 10u;
            r.fastEnd = static_cast<uint32_t>((mmTimes + fastMmps / 2) / fastMmps);
            r.slowEnd = static_cast<uint32_t>((mmTimes + slowMmps / 2) / slowMmps);
        }
    } else {
        r.value = static_cast<uint32_t>((static_cast<uint64_t>(sec) * 1000u + metres / 2) / metres);
        if (band) {
            r.fastEnd = static_cast<uint32_t>((1000000u + fastMmps / 2) / fastMmps);
            r.slowEnd = static_cast<uint32_t>((1000000u + slowMmps / 2) / slowMmps);
        }
    }
    if (band) {
        r.verdict = r.value < r.fastEnd ? Verdict::Fast : r.value > r.slowEnd ? Verdict::Slow : Verdict::InBand;
    }
    return r;
}

}  // namespace RepJudge
}  // namespace XTraining

#endif  // XTRAINING_REP_JUDGE_HPP
