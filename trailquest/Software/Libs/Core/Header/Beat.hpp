/**
 ******************************************************************************
 * @file    Beat.hpp
 * @brief   The timing beat: a needle sweeps a track, and a button press is
 *          judged by how close it lands to the target.
 *
 * Header-only: the GUI process links none of the Libs sources
 * (hybridx-race NOTES 3.4), and it draws the needle with the same functions
 * the service judges with.
 *
 * Pure and integer-only. The needle's position is a function of time alone:
 * the GUI draws it from its own clock, and the same GUI timestamps (the start
 * of the sweep and the press) come to the judge, so the two never need to
 * agree on a clock. Both timestamps are the kernel's wrapping millisecond
 * count, so every difference is an unsigned delta.
 *
 * The GUI runs at 10 frames a second and delivers one button code per frame
 * (una-sdk Docs/Tutorials/RunLVGL/ARCHITECTURE.md), so a press is only known
 * to the nearest 100 ms. The zones are therefore never narrower than two
 * frames (kMinPerfectMs): a "perfect" a player cannot reliably hit is not a
 * skill, it is luck.
 ******************************************************************************
 */

#ifndef TRAILQUEST_BEAT_HPP
#define TRAILQUEST_BEAT_HPP

#include <cstdint>

namespace TrailQuest
{

/// One GUI frame, ms (SDK::GUI::Config::kFrameRate = 10).
constexpr uint32_t kFrameMs = 100;

/// The narrowest "perfect" zone, ms: two frames.
constexpr uint32_t kMinPerfectMs = 2 * kFrameMs;

constexpr uint32_t kMinSweepMs = 800;
constexpr uint32_t kMaxSweepMs = 4000;

enum class BeatResult : uint8_t {
    Miss = 0,
    Good,
    Perfect,
};

/// The shape of one beat. All times in ms; the zones are centred on targetMs.
struct BeatConfig {
    uint32_t sweepMs   = 1600;   ///< One pass of the needle, then it starts again.
    uint32_t targetMs  = 1200;   ///< Where in the pass the target sits.
    uint32_t goodMs    = 400;    ///< Full width of the "good" zone.
    uint32_t perfectMs = 200;    ///< Full width of the "perfect" zone.
};

/// The judgement of one press.
struct BeatJudgement {
    BeatResult result   = BeatResult::Miss;
    int32_t    offsetMs = 0;   ///< Press minus target: negative is early, positive late.
};

/**
 * @brief   Bring a configuration into range: the sweep in [kMinSweepMs,
 *          kMaxSweepMs], the target inside the sweep, perfect at least
 *          kMinPerfectMs, good at least perfect and at most the sweep.
 */
inline BeatConfig clampBeat(BeatConfig c)
{
    if (c.sweepMs < kMinSweepMs) {
        c.sweepMs = kMinSweepMs;
    } else if (c.sweepMs > kMaxSweepMs) {
        c.sweepMs = kMaxSweepMs;
    }
    if (c.targetMs >= c.sweepMs) {
        c.targetMs = c.sweepMs - 1;
    }
    if (c.perfectMs < kMinPerfectMs) {
        c.perfectMs = kMinPerfectMs;
    }
    if (c.goodMs < c.perfectMs) {
        c.goodMs = c.perfectMs;
    }
    if (c.goodMs > c.sweepMs) {
        c.goodMs = c.sweepMs;
    }
    if (c.perfectMs > c.goodMs) {
        c.perfectMs = c.goodMs;
    }
    return c;
}

/// Where the needle is, in ms into the sweep: (now - start) mod sweepMs.
inline uint32_t beatPhaseMs(const BeatConfig& c, uint32_t startMs, uint32_t nowMs)
{
    // Unsigned subtraction is correct across the clock's wrap.
    const uint32_t elapsed = nowMs - startMs;
    return c.sweepMs == 0 ? 0 : elapsed % c.sweepMs;
}

/// The needle's position along the track, 0..1000.
inline uint32_t beatPermille(const BeatConfig& c, uint32_t startMs, uint32_t nowMs)
{
    return c.sweepMs == 0 ? 0 : beatPhaseMs(c, startMs, nowMs) * 1000u / c.sweepMs;
}

/**
 * @brief   Judge a press. The distance to the target is taken round the loop,
 *          so a target near the start of the sweep can be hit just before the
 *          needle wraps. @p c must be in range (see clampBeat()).
 */
inline BeatJudgement judgeBeat(const BeatConfig& c, uint32_t startMs, uint32_t pressMs)
{
    BeatJudgement j;
    const int32_t sweep = static_cast<int32_t>(c.sweepMs);
    int32_t d = static_cast<int32_t>(beatPhaseMs(c, startMs, pressMs))
              - static_cast<int32_t>(c.targetMs);
    // Fold into (-sweep/2, sweep/2]: the nearest way round the loop.
    if (d > sweep / 2) {
        d -= sweep;
    } else if (d <= -(sweep / 2)) {
        d += sweep;
    }
    j.offsetMs = d;

    const uint32_t away = static_cast<uint32_t>(d < 0 ? -d : d);
    if (away * 2u <= c.perfectMs) {
        j.result = BeatResult::Perfect;
    } else if (away * 2u <= c.goodMs) {
        j.result = BeatResult::Good;
    } else {
        j.result = BeatResult::Miss;
    }
    return j;
}

} // namespace TrailQuest

#endif // TRAILQUEST_BEAT_HPP
