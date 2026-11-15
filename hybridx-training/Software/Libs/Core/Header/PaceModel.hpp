/**
 ******************************************************************************
 * @file    PaceModel.hpp
 * @brief   Training paces from a race time (PLANS.md section 2).
 *
 * The Daniels–Gilbert equations (Daniels and Gilbert 1979), as quoted and
 * checked in hybridx-run/docs/RACE_PREDICTIONS.md section 4.3:
 *
 *     VO2 cost      = -4.60 + 0.182258 v + 0.000104 v^2      (v in m/min)
 *     fraction held = 0.8 + 0.1894393 e^(-0.012778 t) + 0.2989558 e^(-0.1932605 t)
 *     fitness       = VO2 cost(D / t) / fraction held(t)      (t in min)
 *
 * The training fractions (kEasyFast ... kInterval) are the points in Daniels'
 * published ranges that reproduce the table rows found in the review
 * (REVIEW.md section 1); they are provisional until rows from the book are in
 * the fixtures.
 *
 * The fitness figure is internal: it is never shown, because it looks like a
 * VO2max and is on a different scale from X Run's (REVIEW.md E5).
 *
 * Floats inside, single precision as on the watch; integers out. Worked out
 * once when a time changes, never in the 1 Hz loop.
 ******************************************************************************
 */

#ifndef XTRAINING_PACE_MODEL_HPP
#define XTRAINING_PACE_MODEL_HPP

#include <cstdint>

namespace XTraining {

/// Seconds per kilometre, rounded to the second.
struct Paces {
    uint16_t easyFast   = 0;  ///< The fast end of easy: easy steps alert above it.
    uint16_t easySlow   = 0;
    uint16_t marathon   = 0;  ///< Shown, not used by the plans.
    uint16_t half       = 0;
    uint16_t threshold  = 0;
    uint16_t tenK       = 0;
    uint16_t interval   = 0;
    uint16_t repetition = 0;
    uint16_t fiveK      = 0;  ///< The 5K's own pace, for estimating a time trial.
};

/// A speed band for SDK::Workout::Step::speedLowMmps / speedHighMmps.
struct SpeedBand {
    uint32_t lowMmps  = 0;  ///< The slow edge.
    uint32_t highMmps = 0;  ///< The fast edge.
};

struct PaceInputs {
    uint32_t fiveKSec = 0;  ///< Required.
    uint32_t halfSec  = 0;  ///< 0 = none (or too old: the caller decides).
};

struct PaceResult {
    bool     ok = false;          ///< False if the 5K is missing or out of range.
    float    fitness5k   = 0.0f;
    float    fitnessHalf = 0.0f;  ///< 0 without a half.
    Paces    paces;
    uint32_t predicted10kSec      = 0;  ///< From the 5K.
    uint32_t predictedHalfSec     = 0;
    uint32_t predictedMarathonSec = 0;
    uint32_t halfAs5kSec  = 0;      ///< The 5K the half matches; 0 without a half.
    bool     enduranceGap = false;  ///< The half is well behind the 5K (PLANS.md 2.4).
};

namespace PaceConfig {
// Checked against Daniels' Running Formula, 4th edition (2022), Tables 5.1-5.3, every row from
// VDOT 20 to 85 (Tests/Host/fixtures/daniels_vdot.csv; DanielsTest.cpp).
constexpr float    kEasyFast  = 0.73f;   ///< Of the fitness: the fast end of easy (Table 5.2: within 3 s/km).
constexpr float    kEasySlow  = 0.63f;
constexpr float    kThreshold = 0.88f;
constexpr float    kInterval  = 0.975f;
/// Repetition is faster than interval by Daniels' "6-second rule" (6 s per 400 m) for better
/// runners, "a 7- or 8-second rule for runners in the 50 or 40 VDOT categories" (ch. 5): 8 s
/// per 400 m at fitness 47 and below, 6 s from 50, in between from one to the other (Table 5.2).
constexpr uint16_t kRepetitionFasterSPerKm    = 15;   ///< 6 s per 400 m, fitness 50 and above.
constexpr uint16_t kRepetitionFasterLowSPerKm = 20;   ///< 8 s per 400 m, fitness 47 and below.
constexpr float    kRepetitionRuleHigh = 50.0f;
constexpr float    kRepetitionRuleLow  = 47.0f;
/// Below fitness 30 Daniels' novice table (Table 5.3) keeps interval pace about 20 s/km faster
/// than threshold, slower than 97.5% of the fitness gives: at fitness 28 and below interval is
/// threshold less 20 s/km, blending into the 97.5% pace by 31.
constexpr uint16_t kSlowIntervalUnderThreshold = 20;
constexpr float    kSlowFitnessLow  = 28.0f;
constexpr float    kSlowFitnessHigh = 31.0f;
/// The half's 5K equivalent slower than the 5K by more than this, in per cent: a gap.
constexpr uint32_t kGapPercent = 4;

constexpr uint32_t kMin5kSec   = 12 * 60;
constexpr uint32_t kMax5kSec   = 59 * 60 + 59;
constexpr uint32_t kMinHalfSec = 55 * 60;
constexpr uint32_t kMaxHalfSec = 3 * 3600 + 59 * 60 + 59;

constexpr float kHalfM     = 21097.5f;
constexpr float kMarathonM = 42195.0f;
}  // namespace PaceConfig

namespace PaceModel {

/// Oxygen cost of running at @p mPerMin, ml/kg/min.
float vo2Cost(float mPerMin);

/// Fraction of VO2max held for a race lasting @p minutes.
float fractionHeld(float minutes);

/// The fitness of @p distanceM in @p seconds; 0 if either is 0.
float fitness(float distanceM, uint32_t seconds);

/// The race time over @p distanceM at @p fitness, in seconds; 0 if fitness is 0.
uint32_t raceSeconds(float fitness, float distanceM);

/// Speed in m/min at which running costs @p vo2.
float speedAt(float vo2);

/// Pace in s/km at a fraction of the fitness.
uint16_t paceAt(float fitness, float fraction);

/// Everything from a 5K (and a half, if given).
PaceResult compute(const PaceInputs& in);

/// A band of +/- @p plusMinus s/km around @p pace.
SpeedBand band(uint16_t paceSPerKm, uint16_t plusMinus);

/// The easy range as a band.
SpeedBand easyBand(const Paces& p);

/// Speed for a pace: 1e6 / s per km, in mm/s; 0 for 0.
uint32_t mmps(uint32_t paceSPerKm);

}  // namespace PaceModel
}  // namespace XTraining

#endif  // XTRAINING_PACE_MODEL_HPP
