/**
 ******************************************************************************
 * @file    PlanBuilder.cpp
 * @brief   Sessions at a level, as programs. See PlanBuilder.hpp.
 ******************************************************************************
 */

#include "PlanBuilder.hpp"

#include "PlanText.hpp"

#include "SDK/Fit/FitProfile.hpp"

#include <cstdio>
#include <cstring>

namespace XTraining {
namespace PlanBuilder {

namespace {

using SDK::Workout::Intensity;
using SDK::Workout::Program;
using SDK::Workout::Step;
using SDK::Workout::StepEnd;
using SDK::Workout::Target;
namespace Fit = SDK::Fit;

// Checked against Daniels' Running Formula, 4th edition (2022), figure 4.1 and chapter 4: I the
// lesser of 10 km and 8% of the week, bouts to 5 min; T a steady 20 min at most, or 30 min in
// cruise intervals, about 10% of the week; R the lesser of 8 km and 5%, bouts to 2 min; L at most
// 150 min. In minutes at each level's weeks (PLANS.md 3.3), and his own plans for runners of the
// same volume (chapter 8: the red plan, 4 days a week, has 15-16 min of I; the blue, 64-84 km, 16
// min of I, 3.2 km of R and a 20 min tempo).
constexpr LevelCaps kCaps[kLevels] = {
    // interval, rep max, tempo, cruise, reps, 10K pace, half pace, finish, long, warm-up
    {10 * 60, 3 * 60, 15, 20 * 60, 0, 16 * 60, 25 * 60, 15, 150, 10},
    {16 * 60, 4 * 60, 20, 30 * 60, 2000, 27 * 60, 40 * 60, 30, 150, 15},
    {20 * 60, 5 * 60, 20, 30 * 60, 3200, 32 * 60, 50 * 60, 40, 150, 15},
};

constexpr uint16_t kCoolDownMin = 10;
constexpr uint16_t kStrideSec = 20;
constexpr uint16_t kStrideEasySec = 60;
constexpr uint16_t kWarmupStrideEasySec = 40;
constexpr uint8_t  kStrides = 6;
constexpr uint8_t  kWarmupStrides = 4;
constexpr uint8_t  kShakeoutStrides = 4;
constexpr uint16_t kShakeoutMin = 20;
constexpr uint16_t kBetweenSetsSec = 180;
constexpr uint16_t kOpenWarmupEstimateMin = 15;  ///< For estimates only: a warm-up ended by a press.
constexpr uint16_t kLevel1HillSec = 45;          ///< Level 1's hills in place of repetitions.
constexpr uint16_t kJogSlowerSPerKm = 45;        ///< Recovery jogs, for estimates.

uint32_t roundDiv(uint32_t num, uint32_t den) { return den == 0 ? 0 : (2u * num + den) / (2u * den); }

uint16_t round5(uint32_t minutes) { return static_cast<uint16_t>(roundDiv(minutes, 5) * 5); }

uint16_t clamp16(uint32_t v, uint16_t lo, uint16_t hi)
{
    return static_cast<uint16_t>(v < lo ? lo : (v > hi ? hi : v));
}

uint16_t roundTo(uint32_t v, uint32_t step) { return static_cast<uint16_t>(roundDiv(v, step) * step); }

const LevelCaps& capsAt(uint8_t level)
{
    const uint8_t i = level >= 1 && level <= kLevels ? static_cast<uint8_t>(level - 1) : 1;
    return kCaps[i];
}

/// Easy minutes at a level: level 2 as written, level 1 three quarters,
/// level 3 five quarters, to 5 min (PLANS.md 4.1); never under 30 min, Daniels' shortest E run
/// ("30 minutes be the minimum duration of any E run", chapter 4; figure 4.1: 30-150 min).
uint16_t easyMinutes(uint16_t a, uint8_t level)
{
    if (level == 1) {
        return clamp16(round5(roundDiv(a * 3u, 4)), 30, 35);
    }
    if (level == 3) {
        return clamp16(round5(roundDiv(a * 5u, 4)), 35, 60);
    }
    return a;
}

/// A long run's minutes at a level: the plan's level 2 range mapped onto the
/// level's, then the half's floor, to 5 min, within 20 and the cap.
uint16_t longMinutes(uint16_t a, const Scaling& sc)
{
    const PlanDef& p = *sc.plan;
    const LongRange& two = p.longRange[1];
    const LongRange& mine = p.longRange[sc.level - 1];
    int32_t m = a;
    const int32_t span2 = static_cast<int32_t>(two.peak) - two.start;
    if (span2 > 0) {
        const int32_t spanMine = static_cast<int32_t>(mine.peak) - mine.start;
        const int32_t num = (static_cast<int32_t>(a) - two.start) * spanMine;
        // Round to nearest, either sign.
        const int32_t q = num >= 0 ? (2 * num + span2) / (2 * span2) : -((-2 * num + span2) / (2 * span2));
        m = mine.start + q;
    }
    if (m < 20) {
        m = 20;
    }
    const uint32_t scaled = roundDiv(static_cast<uint32_t>(m) * sc.longPermille, 1000);
    return clamp16(round5(scaled), 20, capsAt(sc.level).longMaxMin);
}

/// Reps of @p repSec, @p reps of them, at a level, within a total cap.
void scaleReps(Session& s, uint8_t level, uint16_t repMax, uint32_t capSec, uint8_t minReps)
{
    uint32_t t = s.a < repMax ? s.a : repMax;
    uint32_t n = s.reps;
    if (level == 1) {
        const uint32_t total = roundDiv(static_cast<uint32_t>(s.reps) * s.a * 2u, 3);
        n = roundDiv(total, t);
    } else if (level == 3) {
        n = static_cast<uint32_t>(s.reps) + 1u;
    }
    if (n < minReps) {
        n = minReps;
    }
    if (n * t > capSec) {
        n = capSec / t;
        if (n < minReps) {
            n = minReps;
            t = (capSec / n) / 15u * 15u;
            if (t < 60) {
                t = 60;
            }
        }
    }
    s.reps = static_cast<uint8_t>(n > 30 ? 30 : n);
    s.a = static_cast<uint16_t>(t);
}

uint8_t countAtLevel(uint8_t reps, uint8_t level, uint8_t add3, uint8_t min1)
{
    if (level == 1) {
        const uint32_t n = roundDiv(reps * 2u, 3);
        return static_cast<uint8_t>(n < min1 ? min1 : n);
    }
    if (level == 3) {
        return static_cast<uint8_t>(reps + add3);
    }
    return reps;
}

// --- Building ---------------------------------------------------------------

enum class TargetKind : uint8_t { Open, Speed, HrZone };

struct TargetSpec {
    TargetKind kind = TargetKind::Open;
    SpeedBand  band;
    uint8_t    zone = 0;
};

class Emitter {
public:
    Emitter(Program& p, SessionMeta& m) : mP(p), mM(m) {}

    bool full() const { return mFull; }

    /// Adds a step; returns its index. @p value is ms for Time, cm for Distance.
    uint16_t add(Intensity in, const char* name, StepEnd end, uint32_t value, const TargetSpec& t,
                 AlertMode alert, uint32_t estSec, uint32_t estCm, bool quality)
    {
        if (mP.stepCount >= SDK::Workout::kMaxSteps) {
            mFull = true;
            return 0;
        }
        const uint16_t i = mP.stepCount++;
        Step& s = mP.steps[i];
        s = Step{};
        s.intensity = in;
        s.raw.intensity = static_cast<uint8_t>(fitIntensity(in));
        copyText(s.name, sizeof(s.name), name);
        s.end = end;
        switch (end) {
        case StepEnd::Time:
            s.durationMs = value;
            s.raw.durationType = static_cast<uint8_t>(Fit::WktStepDuration::Time);
            s.raw.durationValue = value;
            break;
        case StepEnd::Distance:
            s.distanceCm = value;
            s.raw.durationType = static_cast<uint8_t>(Fit::WktStepDuration::Distance);
            s.raw.durationValue = value;
            break;
        default:
            s.end = StepEnd::Open;
            s.raw.durationType = static_cast<uint8_t>(Fit::WktStepDuration::Open);
            s.raw.durationValue = 0;
            break;
        }
        switch (t.kind) {
        case TargetKind::Speed:
            s.target = Target::Speed;
            s.speedLowMmps = t.band.lowMmps;
            s.speedHighMmps = t.band.highMmps;
            s.raw.targetType = static_cast<uint8_t>(Fit::WktStepTarget::Speed);
            s.raw.targetValue = 0;
            s.raw.customLow = t.band.lowMmps;
            s.raw.customHigh = t.band.highMmps;
            break;
        case TargetKind::HrZone:
            s.target = Target::HeartRate;
            s.hrKind = SDK::Workout::HeartRateTarget::Zone;
            s.hrZone = t.zone;
            s.raw.targetType = static_cast<uint8_t>(Fit::WktStepTarget::HeartRate);
            s.raw.targetValue = t.zone;
            break;
        case TargetKind::Open:
            s.target = Target::Open;
            s.raw.targetType = static_cast<uint8_t>(Fit::WktStepTarget::Open);
            s.raw.targetValue = 0;
            break;
        }
        mM.alert[i] = alert;
        mSec += estSec;
        mCm += estCm;
        if (quality) {
            mQualitySec += estSec;
        }
        mCumSec[i] = mSec;
        mCumCm[i] = mCm;
        mCumQ[i] = mQualitySec;
        return i;
    }

    /// Repeat steps @p from .. the last one, @p count times in all.
    void repeat(uint16_t from, uint32_t count)
    {
        if (count < 2 || mP.stepCount == 0 || from >= mP.stepCount) {
            return;
        }
        if (mP.stepCount >= SDK::Workout::kMaxSteps) {
            mFull = true;
            return;
        }
        const uint16_t last = static_cast<uint16_t>(mP.stepCount - 1);
        const uint32_t before = from == 0 ? 0 : mCumSec[from - 1];
        const uint32_t beforeCm = from == 0 ? 0 : mCumCm[from - 1];
        const uint32_t beforeQ = from == 0 ? 0 : mCumQ[from - 1];
        const uint32_t blockSec = mCumSec[last] - before;
        const uint32_t blockCm = mCumCm[last] - beforeCm;
        const uint32_t blockQ = mCumQ[last] - beforeQ;
        mSec += blockSec * (count - 1);
        mCm += blockCm * (count - 1);
        mQualitySec += blockQ * (count - 1);

        const uint16_t i = mP.stepCount++;
        Step& s = mP.steps[i];
        s = Step{};
        s.end = StepEnd::Repeat;
        s.repeatFrom = from;
        s.repeatCount = count;
        s.raw.durationType = static_cast<uint8_t>(Fit::WktStepDuration::RepeatUntilStepsComplete);
        s.raw.durationValue = from;
        s.raw.targetValue = count;
        mM.alert[i] = AlertMode::None;
        mCumSec[i] = mSec;
        mCumCm[i] = mCm;
        mCumQ[i] = mQualitySec;
    }

    uint32_t sec() const { return mSec; }
    uint32_t cm() const { return mCm; }
    uint32_t qualitySec() const { return mQualitySec; }

    static void copyText(char* dst, size_t cap, const char* src)
    {
        if (cap == 0) {
            return;
        }
        size_t n = src != nullptr ? std::strlen(src) : 0;
        if (n > cap - 1) {
            n = cap - 1;
        }
        if (n > 0) {
            std::memcpy(dst, src, n);
        }
        dst[n] = '\0';
    }

private:
    static Fit::Intensity fitIntensity(Intensity in)
    {
        switch (in) {
        case Intensity::Active:   return Fit::Intensity::Active;
        case Intensity::Rest:     return Fit::Intensity::Rest;
        case Intensity::Warmup:   return Fit::Intensity::Warmup;
        case Intensity::Cooldown: return Fit::Intensity::Cooldown;
        }
        return Fit::Intensity::Active;
    }

    Program&     mP;
    SessionMeta& mM;
    bool     mFull = false;
    uint32_t mSec = 0, mCm = 0, mQualitySec = 0;
    uint32_t mCumSec[SDK::Workout::kMaxSteps] = {};
    uint32_t mCumCm[SDK::Workout::kMaxSteps] = {};
    uint32_t mCumQ[SDK::Workout::kMaxSteps] = {};
};

/// Distance in cm covered in @p sec at @p mmps.
uint32_t cmAt(uint32_t sec, uint32_t mmps) { return static_cast<uint32_t>((static_cast<uint64_t>(sec) * mmps) / 10u); }

/// Seconds to cover @p cm at @p mmps.
uint32_t secFor(uint32_t cm, uint32_t mmps)
{
    return mmps == 0 ? 0 : static_cast<uint32_t>((static_cast<uint64_t>(cm) * 10u + mmps / 2) / mmps);
}

uint32_t mid(const SpeedBand& b) { return (b.lowMmps + b.highMmps) / 2u; }

class SessionBuilder {
public:
    SessionBuilder(const Session& s, const Paces& paces, const BuildOptions& opt, uint32_t raceCm,
                   Program& out, SessionMeta& meta)
        : mS(s), mPaces(paces), mOpt(opt), mRaceCm(raceCm), mOut(out), mMeta(meta), mE(out, meta)
    {
        mEasyBand = PaceModel::easyBand(paces);
        mEasyMid = mid(mEasyBand);
        mJogMmps = PaceModel::mmps(static_cast<uint32_t>(paces.easySlow) + kJogSlowerSPerKm);
        mCaps = &capsAt(opt.level);
    }

    bool run()
    {
        switch (mS.kind) {
        case Kind::Rest:
            return false;
        case Kind::Easy:
            easy(mS.a, "Easy");
            break;
        case Kind::EasyStrides:
            easy(mS.a, "Easy");
            strides(kStrides, kStrideEasySec);
            break;
        case Kind::Long:
            easy(mS.a, "Long run");
            break;
        case Kind::LongFinish:
            if (mS.b == 0 || mS.b >= mS.a) {
                return false;
            }
            easy(static_cast<uint16_t>(mS.a - mS.b), "Long run");
            paced(Intensity::Active, "Half pace", minutesMs(mS.b), mPaces.half, 5, AlertMode::Both);
            break;
        case Kind::Tempo:
            warmup(false);
            pacedRep(Intensity::Active, "Threshold", static_cast<uint32_t>(mS.a) * 60u, mPaces.threshold, 5,
                     AlertMode::Both, false);
            cooldown();
            break;
        case Kind::Cruise:
            warmup(false);
            repsBlock("Threshold", mPaces.threshold, 5, AlertMode::Both);
            cooldown();
            break;
        case Kind::Intervals:
            warmup(true);
            repsBlock("Interval", mPaces.interval, 6, AlertMode::Both);
            cooldown();
            break;
        case Kind::Pace10k:
            warmup(false);
            repsBlock("10K pace", mPaces.tenK, 5, AlertMode::Both);
            cooldown();
            break;
        case Kind::PaceHalf:
            warmup(false);
            repsBlock("Half pace", mPaces.half, 5, AlertMode::Both);
            cooldown();
            break;
        case Kind::ThirtyThirty:
            warmup(true);
            thirtyThirty();
            cooldown();
            break;
        case Kind::Hills:
            warmup(false);
            hills();
            cooldown();
            break;
        case Kind::Reps:
            warmup(true);
            repetitions();
            cooldown();
            break;
        case Kind::TimeTrial5k:
        case Kind::TimeTrial10k:
            timeTrial(mS.kind == Kind::TimeTrial5k ? 500000u : 1000000u);
            break;
        case Kind::Shakeout:
            easy(kShakeoutMin, "Easy");
            strides(kShakeoutStrides, kStrideEasySec);
            break;
        case Kind::Race:
            if (mRaceCm == 0) {
                return false;
            }
            race();
            break;
        }
        if (mE.full() || mOut.stepCount == 0) {
            return false;
        }
        mMeta.plannedSec = mE.sec();
        mMeta.plannedM = (mE.cm() + 50u) / 100u;
        mMeta.qualitySec = mE.qualitySec();
        return true;
    }

private:
    static uint32_t minutesMs(uint32_t minutes) { return minutes * 60000u; }

    TargetSpec easyTarget() const
    {
        TargetSpec t;
        if (mOpt.easyByHr) {
            t.kind = TargetKind::HrZone;
            t.zone = 2;
        } else {
            t.kind = TargetKind::Speed;
            t.band = mEasyBand;
        }
        return t;
    }

    void easyStep(Intensity in, const char* name, uint32_t sec)
    {
        mE.add(in, name, StepEnd::Time, sec * 1000u, easyTarget(), AlertMode::FastOnly, sec,
               cmAt(sec, mEasyMid), false);
    }

    void easy(uint16_t minutes, const char* name) { easyStep(Intensity::Active, name, minutes * 60u); }

    void jog(uint32_t sec, AlertMode alert = AlertMode::None)
    {
        TargetSpec open;
        mE.add(Intensity::Rest, "Jog", StepEnd::Time, sec * 1000u, open, alert, sec, cmAt(sec, mJogMmps), false);
    }

    TargetSpec band(uint16_t pace, uint16_t pm) const
    {
        TargetSpec t;
        t.kind = TargetKind::Speed;
        t.band = PaceModel::band(pace, pm);
        return t;
    }

    void paced(Intensity in, const char* name, uint32_t ms, uint16_t pace, uint16_t pm, AlertMode alert)
    {
        const TargetSpec t = band(pace, pm);
        const uint32_t sec = ms / 1000u;
        mE.add(in, name, StepEnd::Time, ms, t, alert, sec, cmAt(sec, mid(t.band)), true);
        noteTarget(pace, pm);
    }

    /// One paced rep by time, or by distance when the option is on and @p allowDistance (a tempo
    /// run is always timed: run slow and a distance would make it longer, not easier).
    void pacedRep(Intensity in, const char* name, uint32_t sec, uint16_t pace, uint16_t pm, AlertMode alert,
                  bool allowDistance = true)
    {
        const TargetSpec t = band(pace, pm);
        const uint32_t m = pace == 0 ? 0 : roundTo(roundDiv(sec * 1000u, pace), 10);
        if (mMeta.repM == 0) {
            mMeta.repM = m;
        }
        if (mOpt.repsByDistance && allowDistance && pace > 0) {
            uint32_t cm = roundTo(roundDiv(sec * 1000u, pace), 100) * 100u;
            if (cm < 10000u) {
                cm = 10000u;
            }
            mE.add(in, name, StepEnd::Distance, cm, t, alert, secFor(cm, mid(t.band)), cm, true);
        } else {
            mE.add(in, name, StepEnd::Time, sec * 1000u, t, alert, sec, cmAt(sec, mid(t.band)), true);
        }
        noteTarget(pace, pm);
    }

    void noteTarget(uint16_t pace, uint16_t pm)
    {
        if (mMeta.targetPace == 0) {
            mMeta.targetPace = pace;
            mMeta.targetPlusMinus = pm;
        }
    }

    void warmup(bool stridesAtLevel3)
    {
        easyStep(Intensity::Warmup, "Warm up", mCaps->warmupMin * 60u);
        if (stridesAtLevel3 && mOpt.level == 3) {
            strides(kWarmupStrides, kWarmupStrideEasySec);
        }
    }

    void cooldown() { easyStep(Intensity::Cooldown, "Cool down", kCoolDownMin * 60u); }

    /// n x (20 s fast, then easy): the fast part at repetition pace, never alerted.
    void strides(uint8_t n, uint16_t easySec)
    {
        const TargetSpec fast = band(mPaces.repetition, 8);
        const uint16_t from = mE.add(Intensity::Active, "Stride", StepEnd::Time, kStrideSec * 1000u, fast,
                                     AlertMode::None, kStrideSec, cmAt(kStrideSec, mid(fast.band)), false);
        TargetSpec open;
        mE.add(Intensity::Rest, "Easy", StepEnd::Time, easySec * 1000u, open, AlertMode::None, easySec,
               cmAt(easySec, mJogMmps), false);
        mE.repeat(from, n);
    }

    void repsBlock(const char* name, uint16_t pace, uint16_t pm, AlertMode alert)
    {
        const uint32_t rep = mS.a;
        const uint32_t jogSec = mS.jogS != 0 ? mS.jogS : defaultJogSec(mS.kind, mS.a);
        // [rep, jog] x n; the last jog leads into the cool-down.
        const uint16_t first = mOut.stepCount;
        pacedRep(Intensity::Active, name, rep, pace, pm, alert);
        jog(jogSec);
        mE.repeat(first, mS.reps);
    }

    void thirtyThirty()
    {
        const TargetSpec fast = band(mPaces.interval, 8);
        const uint16_t setStart = mOut.stepCount;
        mE.add(Intensity::Active, "Fast", StepEnd::Time, 30000u, fast, AlertMode::None, 30,
               cmAt(30, mid(fast.band)), true);
        TargetSpec open;
        mE.add(Intensity::Rest, "Easy", StepEnd::Time, 30000u, open, AlertMode::None, 30, cmAt(30, mJogMmps),
               false);
        mE.repeat(setStart, mS.a);
        if (mS.reps > 1) {
            mE.add(Intensity::Rest, "Easy", StepEnd::Time, kBetweenSetsSec * 1000u, easyTarget(),
                   AlertMode::FastOnly, kBetweenSetsSec, cmAt(kBetweenSetsSec, mEasyMid), false);
            mE.repeat(setStart, mS.reps);
        }
        noteTarget(mPaces.interval, 8);
    }

    void hills()
    {
        TargetSpec open;
        // Uphill at about threshold effort; the pace means nothing on a hill,
        // so the estimate only uses threshold speed.
        const uint32_t up = mS.a;
        const uint32_t upMmps = PaceModel::mmps(mPaces.threshold);
        const uint16_t from = mE.add(Intensity::Active, "Hill", StepEnd::Time, up * 1000u, open, AlertMode::None,
                                     up, cmAt(up, upMmps) / 2u, true);
        // Back down at a jog, ended by a lap press: about 1.5 times as long.
        mE.add(Intensity::Rest, "Jog down", StepEnd::Open, 0, open, AlertMode::None, up * 3u / 2u,
               cmAt(up, upMmps) / 2u, false);
        mE.repeat(from, mS.reps);
    }

    void repetitions()
    {
        const TargetSpec fast = band(mPaces.repetition, 8);
        const uint32_t cm = static_cast<uint32_t>(mS.a) * 100u;
        const uint16_t from = mE.add(Intensity::Active, "Rep", StepEnd::Distance, cm, fast, AlertMode::AtEnd,
                                     secFor(cm, mid(fast.band)), cm, true);
        TargetSpec open;
        mE.add(Intensity::Rest, "Jog", StepEnd::Distance, cm, open, AlertMode::None, secFor(cm, mJogMmps), cm,
               false);
        mE.repeat(from, mS.reps);
        mMeta.repM = mS.a;
        noteTarget(mPaces.repetition, 8);
    }

    void openWarmup()
    {
        TargetSpec open;
        const uint32_t est = kOpenWarmupEstimateMin * 60u;
        mE.add(Intensity::Warmup, "Warm up", StepEnd::Open, 0, open, AlertMode::None, est, cmAt(est, mEasyMid),
               false);
    }

    void timeTrial(uint32_t cm)
    {
        openWarmup();
        TargetSpec open;
        const uint16_t pace = cm == 500000u ? mPaces.fiveK : mPaces.tenK;
        const uint32_t est = secFor(cm, PaceModel::mmps(pace));
        mE.add(Intensity::Active, cm == 500000u ? "5K time trial" : "10K time trial", StepEnd::Distance, cm, open,
               AlertMode::None, est, cm, true);
        const uint32_t cd = kCoolDownMin * 60u;
        mE.add(Intensity::Cooldown, "Cool down", StepEnd::Open, 0, open, AlertMode::None, cd, cmAt(cd, mJogMmps),
               false);
    }

    void race()
    {
        openWarmup();
        const uint16_t pace = mRaceCm <= 1000000u ? mPaces.tenK : mPaces.half;
        const TargetSpec t = band(pace, 5);
        mE.add(Intensity::Active, "Race", StepEnd::Distance, mRaceCm, t, AlertMode::Both,
               secFor(mRaceCm, mid(t.band)), mRaceCm, true);
        noteTarget(pace, 5);
    }

    const Session&      mS;
    const Paces&        mPaces;
    const BuildOptions& mOpt;
    uint32_t            mRaceCm;
    Program&            mOut;
    SessionMeta&        mMeta;
    Emitter             mE;
    const LevelCaps*    mCaps = nullptr;
    SpeedBand           mEasyBand;
    uint32_t            mEasyMid = 0;
    uint32_t            mJogMmps = 0;
};

}  // namespace

uint16_t defaultJogSec(Kind k, uint16_t repSec)
{
    switch (k) {
    case Kind::Cruise: {
        // 1 min for every 5 min of rep, in 30 s steps, at least 1 min.
        const uint16_t j = roundTo(repSec / 5u, 30);
        return j < 60 ? 60 : j;
    }
    case Kind::Intervals: {
        // About three quarters of the rep, in 15 s steps, at least 1 min: near Daniels' 1:1
        // work to rest for I, and his own examples (3 min with 2 min jog, 4 with 3, 5 with 4).
        const uint16_t j = roundTo(roundDiv(repSec * 3u, 4), 15);
        return j < 60 ? 60 : j;
    }
    case Kind::Pace10k:
    case Kind::PaceHalf:
        return 120;
    default:
        return 0;
    }
}

const LevelCaps& caps(uint8_t level) { return capsAt(level); }

Scaling scaling(const PlanDef& plan, uint8_t level, const Paces& paces, uint8_t runDays)
{
    Scaling sc;
    sc.plan = &plan;
    sc.runDays = runDays;
    sc.level = level >= 1 && level <= kLevels ? level : 2;
    const uint32_t floorM = plan.longFloorM[sc.level - 1];
    const uint32_t mean = (static_cast<uint32_t>(paces.easyFast) + paces.easySlow) / 2u;
    const uint16_t peak = plan.longRange[sc.level - 1].peak;
    if (floorM > 0 && mean > 0 && peak > 0) {
        const uint32_t peakM = roundDiv(static_cast<uint32_t>(peak) * 60u * 1000u, mean);
        if (peakM < floorM) {
            uint32_t permille = roundDiv(floorM * 1000u, peakM);
            const uint32_t maxPermille = roundDiv(capsAt(sc.level).longMaxMin * 1000u, peak);
            if (permille > maxPermille) {
                permille = maxPermille;
            }
            sc.longPermille = static_cast<uint16_t>(permille);
        }
    }
    return sc;
}

Session forLevel(const Session& in, const Scaling& sc)
{
    Session s = in;
    if (sc.plan == nullptr) {
        return s;
    }
    const uint8_t level = sc.level;
    const LevelCaps& c = capsAt(level);
    switch (s.kind) {
    case Kind::Rest:
    case Kind::TimeTrial5k:
    case Kind::TimeTrial10k:
    case Kind::Shakeout:
    case Kind::Race:
        break;
    case Kind::Easy:
    case Kind::EasyStrides:
        s.a = easyMinutes(s.a, level);
        break;
    case Kind::Long:
        s.a = longMinutes(s.a, sc);
        break;
    case Kind::LongFinish: {
        s.a = longMinutes(s.a, sc);
        uint32_t b = in.b;
        if (level == 1) {
            b = round5(roundDiv(b, 2));
        } else if (level == 3) {
            b = round5(roundDiv(b * 4u, 3));
        }
        if (b > c.finishMaxMin) {
            b = c.finishMaxMin;
        }
        if (sc.runDays <= 3 && b > kThreeDayFinishMaxMin) {
            b = kThreeDayFinishMaxMin;   // one of only three runs, and a hard one
        }
        if (b < 5) {
            b = 5;
        }
        if (s.a < b + 20) {
            b = s.a > 25 ? s.a - 20u : 5u;
        }
        s.b = static_cast<uint16_t>(b);
        break;
    }
    case Kind::Tempo: {
        uint32_t m = s.a;
        if (level == 1) {
            m = roundDiv(m * 2u, 3);
        } else if (level == 3) {
            m = roundDiv(m * 5u, 4);
        }
        s.a = clamp16(m, 10, c.tempoMin);
        break;
    }
    case Kind::Cruise:
        scaleReps(s, level, 0xFFFF, c.cruiseSec, 2);
        break;
    case Kind::Intervals:
        scaleReps(s, level, c.intervalRepMaxSec, c.intervalSec, 3);
        break;
    case Kind::Pace10k:
        scaleReps(s, level, 0xFFFF, c.pace10kSec, 2);
        break;
    case Kind::PaceHalf:
        scaleReps(s, level, 0xFFFF, c.paceHalfSec, 2);
        break;
    case Kind::ThirtyThirty: {
        uint32_t pairs = countAtLevel(static_cast<uint8_t>(s.a), level, 1, 4);
        const uint32_t sets = s.reps == 0 ? 1 : s.reps;
        if (sets * pairs * 30u > c.intervalSec) {
            pairs = c.intervalSec / (30u * sets);
        }
        s.a = static_cast<uint16_t>(pairs < 2 ? 2 : pairs);
        break;
    }
    case Kind::Hills:
        s.reps = countAtLevel(s.reps, level, 2, 4);
        break;
    case Kind::Reps:
        if (c.repsM == 0) {
            // Level 1: hills in place of repetitions (PLANS.md 4.1, C6).
            s.kind = Kind::Hills;
            s.reps = countAtLevel(s.reps, 1, 0, 4);
            s.a = kLevel1HillSec;
        } else {
            uint32_t n = countAtLevel(s.reps, level, 2, 4);
            if (s.a > 0 && n * s.a > c.repsM) {
                n = c.repsM / s.a;
            }
            s.reps = static_cast<uint8_t>(n < 1 ? 1 : n);
        }
        break;
    }
    return s;
}

bool build(const Session& s, const Paces& paces, const BuildOptions& opt, uint32_t raceCm,
           SDK::Workout::Program& out, SessionMeta& meta)
{
    out = Program{};
    meta = SessionMeta{};
    meta.session = s;
    if (paces.easyFast == 0 || paces.interval == 0) {
        return false;
    }
    SessionBuilder b(s, paces, opt, raceCm, out, meta);
    if (!b.run()) {
        out = Program{};
        meta = SessionMeta{};
        meta.session = s;
        return false;
    }
    char what[48];
    PlanText::what(s, raceCm, what, sizeof(what));
    std::snprintf(out.name, sizeof(out.name), "%s %s", PlanText::name(s.kind), what);
    Emitter::copyText(out.description, sizeof(out.description), PlanText::purpose(s.kind));
    out.sport = static_cast<uint8_t>(Fit::Sport::Running);
    out.subSport = static_cast<uint8_t>(Fit::SubSport::Generic);
    return true;
}

}  // namespace PlanBuilder
}  // namespace XTraining
