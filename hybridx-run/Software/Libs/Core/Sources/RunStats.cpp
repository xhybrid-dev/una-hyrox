/**
 ******************************************************************************
 * @file    RunStats.cpp
 * @brief   The run detail page's numbers (see the header).
 ******************************************************************************
 */

#include "RunStats.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace RunVo2
{

namespace
{

// The VO2max core's own test for a usable heart rate (Vo2Window.cpp, goodHr).
bool goodHr(const Sample& s)
{
    return s.hrTrust >= Config::kMinHrTrust && s.hrTrust <= Config::kMaxHrTrust &&
           s.hrBpm >= Config::kMinHrBpm && s.hrBpm <= Config::kMaxHrBpm;
}

uint32_t toU32(double v)
{
    return v <= 0.0 ? 0u : (v >= 4294967295.0 ? 4294967295u : static_cast<uint32_t>(v + 0.5));
}

}  // namespace

void RunStats::reset(const uint8_t* thresholds, uint8_t count)
{
    mNow       = Totals {};
    mCpCount   = 0;
    mCpSpacing = StatsConfig::kFirstSpacingS;

    std::memset(mZoneSec, 0, sizeof(mZoneSec));
    mZoneCount = 0;
    if (thresholds != nullptr) {
        mZoneCount = std::min<uint8_t>(count, StatsConfig::kZones);
        std::memcpy(mThreshold, thresholds, mZoneCount);
    }

    std::memset(mCross, 0, sizeof(mCross));
    mLatest    = 0;
    mOriginM   = 0.0f;
    mPrevDistM = 0.0f;
    std::memset(mBestMs, 0, sizeof(mBestMs));
}

float RunStats::avgGradeAdjustedSpeedMs() const
{
    return mNow.gapSec == 0 ? 0.0f
                            : static_cast<float>(static_cast<double>(mNow.gapSumMmps) / mNow.gapSec / 1000.0);
}

void RunStats::addSecond(const Sample& s, float distanceM)
{
    if (!s.active) {
        return;   // a pause: nothing moves, nothing counts
    }
    const uint32_t secBefore = mNow.sec;
    ++mNow.sec;
    mNow.distM = distanceM;

    // Heart rate, and the zone it is in.
    if (goodHr(s)) {
        ++mNow.hrSec;
        mNow.hrSum += s.hrBpm;
        if (mZoneCount > 0) {
            int zone = -1;   // the highest threshold the heart rate exceeds
            for (int i = mZoneCount - 1; i >= 0; --i) {
                if (s.hrBpm > mThreshold[i]) {
                    zone = i;
                    break;
                }
            }
            ++mZoneSec[zone + 1];
        }
    }

    // Grade-adjusted speed: every moving second, as WindowBuilder does (uphill only).
    if (s.speedValid && !s.deadReckoning && s.speedMs >= Config::kMinMovingMs) {
        float grade = 0.0f;
        if (s.gradeValid) {
            grade = std::max(0.0f, std::min(s.gradePct, Config::kGapMaxGradePct)) / 100.0f;
        }
        const double level = static_cast<double>(s.speedMs) * (1.0 + static_cast<double>(Config::kAcsmGradeFactor * grade));
        mNow.gapSumMmps += toU32(level * 1000.0);
        ++mNow.gapSec;
    }

    // Distance boundaries every kRingStepM metres.
    const float stepM = distanceM - mPrevDistM;
    if (stepM < 0.0f || stepM > StatsConfig::kMaxJumpM) {
        // Not a real second of running: no split may span it.
        mOriginM = distanceM;
        mLatest  = 0;
        mCross[0] = secBefore * 1000u + 1000u;
    } else {
        const uint32_t newest = static_cast<uint32_t>((distanceM - mOriginM) / StatsConfig::kRingStepM);
        for (uint32_t n = mLatest + 1; n <= newest; ++n) {
            addBoundary(n, mPrevDistM, stepM, secBefore);
        }
    }
    mPrevDistM = distanceM;

    checkpoint();
}

void RunStats::addBoundary(uint32_t n, float distBeforeM, float stepM, uint32_t secBefore)
{
    // How far into this second the boundary was crossed.
    const float into = static_cast<float>(n) * StatsConfig::kRingStepM - (distBeforeM - mOriginM);
    float       f    = stepM > 0.0f ? into / stepM : 1.0f;
    f                = std::max(0.0f, std::min(f, 1.0f));
    mCross[n % StatsConfig::kRingCount] = secBefore * 1000u + static_cast<uint32_t>(f * 1000.0f + 0.5f);
    mLatest                             = n;
    checkSplits(n);
}

void RunStats::checkSplits(uint32_t n)
{
    const float endM = static_cast<float>(n) * StatsConfig::kRingStepM;
    for (uint8_t i = 0; i < StatsConfig::kSplits; ++i) {
        const float length = StatsConfig::kSplitM[i];
        if (endM < length) {
            continue;
        }
        const float    startM = endM - length;
        const uint32_t j      = static_cast<uint32_t>(startM / StatsConfig::kRingStepM);
        if (n - j >= StatsConfig::kRingCount) {
            continue;   // beyond the ring: cannot happen for the lengths above
        }
        const double frac  = static_cast<double>(startM - static_cast<float>(j) * StatsConfig::kRingStepM) /
                             StatsConfig::kRingStepM;
        const double t0    = mCross[j % StatsConfig::kRingCount];
        const double t1    = mCross[(j + 1) % StatsConfig::kRingCount];
        const double start = t0 + (t1 - t0) * frac;
        const uint32_t ms  = toU32(static_cast<double>(mCross[n % StatsConfig::kRingCount]) - start);
        if (ms > 0 && (mBestMs[i] == 0 || ms < mBestMs[i])) {
            mBestMs[i] = ms;
        }
    }
}

void RunStats::checkpoint()
{
    if (mNow.sec % mCpSpacing != 0) {
        return;
    }
    if (mCpCount == StatsConfig::kCheckpoints) {
        // Full: keep every second one and double the spacing, so a run of any length fits.
        for (uint8_t i = 1; i < mCpCount; i += 2) {
            mCp[i / 2] = mCp[i];
        }
        mCpCount = static_cast<uint8_t>(mCpCount / 2);
        mCpSpacing = static_cast<uint16_t>(mCpSpacing * 2u);
        if (mNow.sec % mCpSpacing != 0) {
            return;
        }
    }
    mCp[mCpCount++] = mNow;
}

RunStats::Totals RunStats::lerp(const Totals& a, const Totals& b, double f)
{
    const auto mix = [f](double x, double y) { return x + (y - x) * f; };
    Totals     t;
    t.sec        = toU32(mix(a.sec, b.sec));
    t.distM      = static_cast<float>(mix(a.distM, b.distM));
    t.hrSec      = toU32(mix(a.hrSec, b.hrSec));
    t.hrSum      = toU32(mix(a.hrSum, b.hrSum));
    t.gapSec     = toU32(mix(a.gapSec, b.gapSec));
    t.gapSumMmps = toU32(mix(a.gapSumMmps, b.gapSumMmps));
    return t;
}

RunStats::Totals RunStats::totalsAt(uint32_t sec) const
{
    if (sec >= mNow.sec) {
        return mNow;
    }
    Totals prev;   // the run's start: all zero
    for (uint8_t i = 0; i <= mCpCount; ++i) {
        const Totals& next = i < mCpCount ? mCp[i] : mNow;
        if (next.sec >= sec) {
            const uint32_t span = next.sec - prev.sec;
            return span == 0 ? next : lerp(prev, next, static_cast<double>(sec - prev.sec) / span);
        }
        prev = next;
    }
    return mNow;
}

RunStatsResult RunStats::result() const
{
    RunStatsResult r;
    r.activeSec = mNow.sec;
    std::memcpy(r.zoneSec, mZoneSec, sizeof(r.zoneSec));
    r.zoneCount = mZoneCount;
    std::memcpy(r.bestMs, mBestMs, sizeof(r.bestMs));
    if (mNow.gapSec > 0) {
        r.gapSpeedMmps = static_cast<uint16_t>(std::min<uint32_t>(mNow.gapSumMmps / mNow.gapSec, 65535u));
    }

    // The first and second half of the whole run.
    constexpr uint32_t kMinHalfRunSec = 120;
    if (mNow.sec >= kMinHalfRunSec) {
        const Totals mid = totalsAt(mNow.sec / 2);
        const float  d1  = mid.distM;
        const float  d2  = mNow.distM - mid.distM;
        if (d1 >= 1.0f) {
            r.halfPace[0] = static_cast<float>(mid.sec) / d1;
        }
        if (d2 >= 1.0f) {
            r.halfPace[1] = static_cast<float>(mNow.sec - mid.sec) / d2;
        }
        if (mid.hrSec > 0) {
            r.halfHr[0] = static_cast<uint8_t>(mid.hrSum / mid.hrSec);
        }
        if (mNow.hrSec > mid.hrSec) {
            r.halfHr[1] = static_cast<uint8_t>((mNow.hrSum - mid.hrSum) / (mNow.hrSec - mid.hrSec));
        }
    }

    // Drift: the halves after the warm-up.
    const uint32_t warm = Config::kWarmUpSec;
    if (mNow.sec <= warm || mNow.sec - warm < StatsConfig::kMinDriftSec) {
        r.driftStatus = DriftStatus::TooShort;
        return r;
    }
    const Totals a   = totalsAt(warm);
    const Totals mid = totalsAt((warm + mNow.sec) / 2);
    const Totals* edge[3] = {&a, &mid, &mNow};
    double        level[2] = {};   // level-equivalent speed, m/s
    double        ef[2]    = {};   // (m/min) per bpm
    for (int h = 0; h < 2; ++h) {
        const Totals& from = *edge[h];
        const Totals& to   = *edge[h + 1];
        const uint32_t secs   = to.sec - from.sec;
        const uint32_t hrSec  = to.hrSec - from.hrSec;
        const uint32_t gapSec = to.gapSec - from.gapSec;
        if (secs == 0 || static_cast<uint64_t>(hrSec) * 100u < static_cast<uint64_t>(StatsConfig::kMinShareSecPct) * secs ||
            static_cast<uint64_t>(gapSec) * 100u < static_cast<uint64_t>(StatsConfig::kMinShareSecPct) * secs ||
            to.hrSum <= from.hrSum) {
            r.driftStatus = DriftStatus::NoHeartRate;
            return r;
        }
        const double hr = static_cast<double>(to.hrSum - from.hrSum) / hrSec;
        level[h]        = static_cast<double>(to.gapSumMmps - from.gapSumMmps) / gapSec / 1000.0;
        ef[h]           = level[h] * 60.0 / hr;
        r.efX1000[h]    = static_cast<uint16_t>(std::min(ef[h] * 1000.0 + 0.5, 65535.0));
    }
    if (level[0] <= 0.0 || ef[0] <= 0.0 ||
        std::fabs(level[1] - level[0]) / level[0] * 100.0 > static_cast<double>(StatsConfig::kMaxHalfSpeedDiffPct)) {
        r.driftStatus = DriftStatus::NotSteady;
        return r;
    }
    const double drift = (ef[0] - ef[1]) / ef[0] * 1000.0;   // % x 10
    r.driftX10         = static_cast<int16_t>(std::max(-1000.0, std::min(drift, 1000.0)) + (drift >= 0.0 ? 0.5 : -0.5));
    r.driftStatus      = DriftStatus::Ok;
    return r;
}

}  // namespace RunVo2
