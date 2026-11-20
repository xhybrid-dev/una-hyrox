/**
 ******************************************************************************
 * @file    SessionReview.hpp
 * @brief   How well a planned session kept to its paces (TP8): the share of
 *          the judged time in the target band, whether the misses were too
 *          fast or too slow, the average gap, and the fade in the later reps.
 *          Pure, fixed-size, no heap; the service feeds it once a second.
 *
 * What is judged (NOTES "TP8"; Jon, 20 November 2026): "time in band plus
 * direction".
 *   - A step with a speed target is judged by the clock: each second is In,
 *     Fast or Slow (the zone the runner's own gauge shows), with how far
 *     outside the band the pace was, s/km.
 *   - A short rep (strides, repetitions, 30-30s) is judged once, by its
 *     average, as the step card says (RepJudge), and weighted by its time.
 *   - A session with quality steps is judged on those only (warm-up and
 *     cool-down would dilute "the interval paces"); an easy session is judged
 *     on its easy steps, where only "too fast" is a miss (a slower jog is not).
 *   - Heart-rate targets, a treadmill (its speed is assumed), seconds with
 *     no signal and steps with no target are not judged.
 *
 * Gaps are s/km and signed: negative is faster than the band, positive slower.
 ******************************************************************************
 */

#ifndef XTRAINING_SESSION_REVIEW_HPP
#define XTRAINING_SESSION_REVIEW_HPP

#include "PlanTypes.hpp"
#include "RepJudge.hpp"

#include <cstdint>

namespace XTraining {
namespace Review {

/// The numbers that decide things, in one place.
constexpr uint8_t  kOffPacePct    = 60;   ///< A judged session below this share in band is "off pace".
constexpr uint16_t kMinJudgedSec  = 180;  ///< Judged means at least this much time ...
constexpr uint16_t kMinRepsSec    = 60;   ///< ... or at least two reps and this much time.
constexpr uint8_t  kOffPaceRun    = 3;    ///< Sessions in a row off pace before a suggestion.
constexpr uint8_t  kFadeMinUnits  = 4;    ///< Reps needed to say anything of a fade.
constexpr int8_t   kFadeShowSecKm = 3;    ///< A fade of less than this is not worth saying.

/// How one second, or one rep, sat against its band.
enum class Cls : uint8_t { None = 0, In, Fast, Slow };

/// The direction of a session's misses.
enum class Dir : uint8_t { None = 0, Fast, Slow };

/// How the runner felt afterwards.
enum class Feel : uint8_t { NotAsked = 0, Fresh, Good, Steady, Tired, Spent };
constexpr uint8_t kFeelCount = 6;   ///< Including NotAsked.

inline bool isTired(uint8_t feel) { return feel == static_cast<uint8_t>(Feel::Tired) || feel == static_cast<uint8_t>(Feel::Spent); }
inline bool isFresh(uint8_t feel) { return feel == static_cast<uint8_t>(Feel::Fresh) || feel == static_cast<uint8_t>(Feel::Good); }

/// One session's review.
struct Result {
    bool     judged    = false;
    uint16_t judgedSec = 0;       ///< Seconds judged (a rep counts its own time), capped at 65535.
    uint8_t  inPct     = 0;       ///< The three shares add to 100 when judged.
    uint8_t  fastPct   = 0;
    uint8_t  slowPct   = 0;
    int16_t  gapSecKm  = 0;       ///< The average miss in the dominant direction: < 0 faster, > 0 slower; 0 for none.
    int8_t   fadeSecKm = 0;       ///< Later reps against the first: > 0 slower; 0 when under kFadeMinUnits reps.
    uint8_t  units     = 0;       ///< Reps (or steps) judged.
    uint8_t  unitsIn   = 0;       ///< ... of them in band.

    Dir dir() const
    {
        if (!judged || (fastPct == 0 && slowPct == 0)) {
            return Dir::None;
        }
        return fastPct > slowPct ? Dir::Fast : Dir::Slow;   // a tie is slow: the safer thing to say
    }
    bool offPace() const { return judged && inPct < kOffPacePct; }
};

/// Fed once a second while a judged step runs, and once per judged rep; closed at the end of a step.
class Accumulator {
public:
    static constexpr uint8_t kMaxUnits = 32;

    void reset();

    /// One second on a step judged by the clock: its class, and how far outside the band the pace was,
    /// s/km (> 0 slower, < 0 faster, 0 in band). @p fastOnly: an easy step, where Slow is not a miss.
    /// Cls::None (no signal, too early) is not counted.
    void addSecond(Cls cls, int32_t gapSecKm, bool fastOnly = false);

    /// A rep judged by its average when it ended (RepJudge), weighted by its @p sec.
    void addRep(Cls cls, int32_t gapSecKm, uint32_t sec);

    /// A step (or rep) ended: the seconds since the last close are one unit, for the fade.
    void endUnit();

    /// The review so far (the open unit included).
    Result finish() const;

private:
    struct Unit {
        uint16_t sec    = 0;
        int16_t  meanGap = 0;   ///< Time-weighted mean of the signed gaps, in-band seconds as 0.
        uint8_t  cls    = 0;    ///< Cls
    };

    void closeInto(uint16_t in, uint16_t fast, uint16_t slow, int32_t gapSum);

    uint32_t mInSec   = 0;
    uint32_t mFastSec = 0;
    uint32_t mSlowSec = 0;
    uint64_t mFastGap = 0;      ///< Sum of |gap| x seconds, fast misses.
    uint64_t mSlowGap = 0;
    // The open unit
    uint16_t mUIn   = 0;
    uint16_t mUFast = 0;
    uint16_t mUSlow = 0;
    int32_t  mUGap  = 0;        ///< Sum of signed gap x seconds.
    Unit     mUnit[kMaxUnits];
    uint8_t  mUnitCount = 0;
    uint8_t  mUnitsIn   = 0;    ///< Counted past kMaxUnits too.
    uint8_t  mUnitsAll  = 0;
};

/// The gap of a rep from its judge: how far outside the band, s/km. Time-judged reps are converted
/// to pace over the step's distance. 0 when the rep was in band or had no band.
int32_t repGapSecKm(const SDK::Workout::Step& step, const RepJudge::Result& r);

/// The class of a rep's verdict.
inline Cls repCls(RepJudge::Verdict v)
{
    switch (v) {
    case RepJudge::Verdict::InBand: return Cls::In;
    case RepJudge::Verdict::Fast:   return Cls::Fast;
    case RepJudge::Verdict::Slow:   return Cls::Slow;
    case RepJudge::Verdict::None:   break;
    }
    return Cls::None;
}

}  // namespace Review
}  // namespace XTraining

#endif  // XTRAINING_SESSION_REVIEW_HPP
