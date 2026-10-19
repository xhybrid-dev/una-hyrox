/**
 ******************************************************************************
 * @file    Beat.hpp
 * @brief   The timing beat: a needle sweeps a track, and a button press is
 *          judged by how close it lands to the target.
 *
 * Header-only: the GUI process links none of the Libs sources
 * (hybridx-race NOTES 3.4), and it draws the needle and the zones with the
 * same functions the service judges with.
 *
 * Pure and integer-only. The needle's position is a function of time alone:
 * the GUI draws it from its own clock, and the same GUI timestamps (the start
 * of the sweep and the press) come to the judge, so the two never need to
 * agree on a clock.
 *
 * Which clock: LVGL's tick, lv_tick_get(). The SDK's port accumulates the
 * kernel clock into a tick that wraps at 2^32 (LvglPort.cpp, Port::tickCb), so
 * an unsigned difference is right across the wrap. The kernel's own
 * sys.getTimeMs() returns to 0 short of 2^32 and must not be mixed in.
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
 * @brief   Bring a configuration into range:
 *          - the sweep in [kMinSweepMs, kMaxSweepMs];
 *          - perfect at least kMinPerfectMs;
 *          - good at least perfect and at most half the sweep;
 *          - the target where the whole good zone fits on the track, so the
 *            zones are drawn as one piece and never straddle the needle's
 *            restart.
 */
inline BeatConfig clampBeat(BeatConfig c)
{
    if (c.sweepMs < kMinSweepMs) {
        c.sweepMs = kMinSweepMs;
    } else if (c.sweepMs > kMaxSweepMs) {
        c.sweepMs = kMaxSweepMs;
    }
    if (c.perfectMs < kMinPerfectMs) {
        c.perfectMs = kMinPerfectMs;
    }
    if (c.goodMs < c.perfectMs) {
        c.goodMs = c.perfectMs;
    }
    if (c.goodMs > c.sweepMs / 2) {
        c.goodMs = c.sweepMs / 2;   // kMinSweepMs / 2 >= kMinPerfectMs, so still >= perfect's floor
    }
    if (c.perfectMs > c.goodMs) {
        c.perfectMs = c.goodMs;
    }
    const uint32_t half = (c.goodMs + 1u) / 2u;   // round up: the zone's whole width fits
    if (c.targetMs < half) {
        c.targetMs = half;
    } else if (c.targetMs > c.sweepMs - half) {
        c.targetMs = c.sweepMs - half;
    }
    return c;
}

/**
 * @brief   The beat for a stage (0-4) and a lap (0 the first crossing): stage 1
 *          is the Q0 beat Jon found "fairly easy but good"; each later stage
 *          sweeps 100 ms faster with a good zone 40 ms narrower, and each lap
 *          after the first a further 100 ms faster (to 1000) and 20 ms
 *          narrower (to 200). The perfect zone stays at its floor of two
 *          frames (kMinPerfectMs): at 10 frames a second nothing narrower can
 *          be told from luck, so "more accurate" means the good zone shrinks.
 *          The target stays at three quarters of the sweep. Always in range.
 */
inline BeatConfig beatFor(uint8_t stage, uint8_t lap)
{
    const uint32_t s = stage > 4u ? 4u : stage;
    const uint32_t l = lap > 20u ? 20u : lap;
    BeatConfig c;
    const uint32_t faster = 100u * s + 100u * l;
    c.sweepMs   = faster >= 600u ? 1000u : 1600u - faster;
    const uint32_t narrower = 40u * s + 20u * l;
    c.goodMs    = narrower >= 200u ? 200u : 400u - narrower;
    c.perfectMs = kMinPerfectMs;
    c.targetMs  = c.sweepMs * 3u / 4u;
    return clampBeat(c);
}

/// Where the needle is, in ms into the sweep: (now - start) mod sweepMs.
inline uint32_t beatPhaseMs(const BeatConfig& c, uint32_t startMs, uint32_t nowMs)
{
    // Unsigned subtraction is correct across the tick's wrap.
    const uint32_t elapsed = nowMs - startMs;
    return c.sweepMs == 0 ? 0 : elapsed % c.sweepMs;
}

/**
 * @brief   A time in the sweep as a distance along a track @p width long:
 *          0 at the start, @p width at the end. Used for the needle and for
 *          both edges of each zone, so what is drawn is what is judged.
 *          @p ms beyond the sweep is held at the end.
 */
inline uint32_t beatToTrack(const BeatConfig& c, uint32_t ms, uint32_t width)
{
    if (c.sweepMs == 0) {
        return 0;
    }
    if (ms > c.sweepMs) {
        ms = c.sweepMs;
    }
    return static_cast<uint32_t>(static_cast<uint64_t>(ms) * width / c.sweepMs);
}

/**
 * @brief   Judge a press. The distance to the target is taken the near way
 *          round the loop, folded into (-sweep/2, sweep/2]. @p c must be in
 *          range (see clampBeat()).
 */
inline BeatJudgement judgeBeat(const BeatConfig& c, uint32_t startMs, uint32_t pressMs)
{
    BeatJudgement j;
    const int32_t sweep = static_cast<int32_t>(c.sweepMs);
    int32_t d = static_cast<int32_t>(beatPhaseMs(c, startMs, pressMs))
              - static_cast<int32_t>(c.targetMs);
    // Doubled, so an odd sweep folds exactly: 2d in (-sweep, sweep].
    if (2 * d > sweep) {
        d -= sweep;
    } else if (2 * d <= -sweep) {
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
