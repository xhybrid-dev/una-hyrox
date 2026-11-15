/**
 ******************************************************************************
 * @file    PaceModel.cpp
 * @brief   Training paces from a race time. See PaceModel.hpp.
 ******************************************************************************
 */

#include "PaceModel.hpp"

#include <cmath>

namespace XTraining {
namespace PaceModel {

namespace {

uint16_t paceFromSpeed(float mPerMin)
{
    if (!(mPerMin > 0.0f)) {
        return 0;
    }
    const float s = 60000.0f / mPerMin;
    if (s >= 65535.0f) {
        return 65535;
    }
    return static_cast<uint16_t>(std::lround(s));
}

uint16_t paceOf(float distanceM, uint32_t seconds)
{
    if (distanceM <= 0.0f || seconds == 0) {
        return 0;
    }
    return static_cast<uint16_t>(std::lround(static_cast<float>(seconds) * 1000.0f / distanceM));
}

}  // namespace

float vo2Cost(float mPerMin)
{
    return -4.60f + 0.182258f * mPerMin + 0.000104f * mPerMin * mPerMin;
}

float fractionHeld(float minutes)
{
    return 0.8f + 0.1894393f * std::exp(-0.012778f * minutes)
         + 0.2989558f * std::exp(-0.1932605f * minutes);
}

float fitness(float distanceM, uint32_t seconds)
{
    if (distanceM <= 0.0f || seconds == 0) {
        return 0.0f;
    }
    const float t = static_cast<float>(seconds) / 60.0f;
    return vo2Cost(distanceM / t) / fractionHeld(t);
}

uint32_t raceSeconds(float fit, float distanceM)
{
    if (!(fit > 0.0f) || distanceM <= 0.0f) {
        return 0;
    }
    // fitness() falls as the time rises, so bisect on the time in minutes.
    float lo = 0.5f;
    float hi = 1500.0f;
    for (int i = 0; i < 60; ++i) {
        const float mid = 0.5f * (lo + hi);
        const float t = mid;
        const float f = vo2Cost(distanceM / t) / fractionHeld(t);
        if (f > fit) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return static_cast<uint32_t>(std::lround(0.5f * (lo + hi) * 60.0f));
}

float speedAt(float vo2)
{
    // 0.000104 v^2 + 0.182258 v - (4.60 + vo2) = 0, the positive root.
    const float a = 0.000104f;
    const float b = 0.182258f;
    const float c = -(4.60f + vo2);
    const float disc = b * b - 4.0f * a * c;
    if (disc <= 0.0f) {
        return 0.0f;
    }
    return (-b + std::sqrt(disc)) / (2.0f * a);
}

uint16_t paceAt(float fit, float fraction)
{
    return paceFromSpeed(speedAt(fit * fraction));
}

uint32_t mmps(uint32_t paceSPerKm)
{
    if (paceSPerKm == 0) {
        return 0;
    }
    return (1000000u + paceSPerKm / 2u) / paceSPerKm;
}

SpeedBand band(uint16_t pace, uint16_t plusMinus)
{
    SpeedBand b;
    if (pace == 0) {
        return b;
    }
    const uint32_t slow = static_cast<uint32_t>(pace) + plusMinus;
    const uint32_t fast = pace > plusMinus ? static_cast<uint32_t>(pace) - plusMinus : 1u;
    b.lowMmps  = mmps(slow);
    b.highMmps = mmps(fast);
    return b;
}

SpeedBand easyBand(const Paces& p)
{
    SpeedBand b;
    b.lowMmps  = mmps(p.easySlow);
    b.highMmps = mmps(p.easyFast);
    return b;
}

PaceResult compute(const PaceInputs& in)
{
    using namespace PaceConfig;
    PaceResult r;
    if (in.fiveKSec < kMin5kSec || in.fiveKSec > kMax5kSec) {
        return r;
    }
    r.fitness5k = fitness(5000.0f, in.fiveKSec);
    const bool haveHalf = in.halfSec >= kMinHalfSec && in.halfSec <= kMaxHalfSec;
    if (haveHalf) {
        r.fitnessHalf = fitness(kHalfM, in.halfSec);
        r.halfAs5kSec = raceSeconds(r.fitnessHalf, 5000.0f);
        r.enduranceGap = static_cast<uint64_t>(r.halfAs5kSec) * 100u
                       > static_cast<uint64_t>(in.fiveKSec) * (100u + kGapPercent);
    }

    const float f = r.fitness5k;
    // Easy running follows the lower of the two: slower easy is the safe side.
    const float fEasy = haveHalf && r.fitnessHalf < f ? r.fitnessHalf : f;

    Paces& p = r.paces;
    p.easyFast  = paceAt(fEasy, kEasyFast);
    p.easySlow  = paceAt(fEasy, kEasySlow);
    p.threshold = paceAt(f, kThreshold);
    p.interval  = paceAt(f, kInterval);
    p.repetition = p.interval > kRepetitionFasterSPerKm
                 ? static_cast<uint16_t>(p.interval - kRepetitionFasterSPerKm) : p.interval;

    r.predicted10kSec      = raceSeconds(f, 10000.0f);
    r.predictedHalfSec     = raceSeconds(f, kHalfM);
    r.predictedMarathonSec = raceSeconds(f, kMarathonM);
    p.fiveK    = paceOf(5000.0f, in.fiveKSec);
    p.tenK     = paceOf(10000.0f, r.predicted10kSec);
    p.marathon = paceOf(kMarathonM, r.predictedMarathonSec);
    // Half race pace comes from the half itself when there is one.
    p.half = haveHalf ? paceOf(kHalfM, in.halfSec) : paceOf(kHalfM, r.predictedHalfSec);

    r.ok = p.easyFast > 0 && p.easySlow > p.easyFast && p.interval > 0;
    return r;
}

}  // namespace PaceModel
}  // namespace XTraining
