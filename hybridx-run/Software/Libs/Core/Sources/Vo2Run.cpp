/**
 ******************************************************************************
 * @file    Vo2Run.cpp
 * @brief   One run's VO2max from steady windows.
 ******************************************************************************
 */

#include "Vo2Run.hpp"

#include <algorithm>
#include <cmath>

namespace RunVo2
{

namespace
{

bool goodHr(const Sample& s)
{
    return s.hrTrust >= Config::kMinHrTrust && s.hrTrust <= Config::kMaxHrTrust &&
           s.hrBpm >= Config::kMinHrBpm && s.hrBpm <= Config::kMaxHrBpm;
}

bool goodSpeed(const Sample& s)
{
    return s.speedValid && !s.deadReckoning &&
           s.speedMs >= Config::kMinSpeedMs && s.speedMs <= Config::kMaxSpeedMs;
}

/// ACSM running equation, ml/kg/min. Grade in percent; downhill counts as 0.
float acsmRunningVo2(float speedMs, float gradePct)
{
    const float v     = speedMs * 60.0f;
    const float grade = gradePct > 0.0f ? gradePct / 100.0f : 0.0f;
    return 3.5f + 0.2f * v + 0.9f * v * grade;
}

}  // namespace

void Vo2Run::reset()
{
    // Field by field: a whole-object temporary would put 1 KB on the
    // service's 10 KB stack.
    clearWindow();
    mActiveSec    = 0;
    mWindowIndex  = 0;
    mLog          = WindowLog{};
    mLogReady     = false;
    mWindowCount  = 0;
    mCounts       = WindowCounts{};
    mHoldCount    = 0;
    mHoldNext     = 0;
    mSustainedMax = 0;
}

void Vo2Run::clearWindow()
{
    mSec       = 0;
    mGood      = 0;
    mSpeedRef  = 0.0f;
    mSumSpeed  = 0.0f;
    mSumSpeed2 = 0.0f;
    mSumGrade  = 0.0f;
    mSumHr     = 0;
    mMinHr     = 255;
    mMaxHr     = 0;
    mTooSteep  = false;
}

void Vo2Run::addSecond(const Sample& s)
{
    if (!s.active) {
        // A pause breaks the window: what it held was not one steady minute.
        clearWindow();
        mHoldCount = 0;
        return;
    }

    ++mActiveSec;

    // Auto max: the lowest of the last few good readings is a level held for
    // that long; keep the highest such level. Not in the warm-up.
    if (goodHr(s) && mActiveSec > Config::kWarmUpSec) {
        mHold[mHoldNext] = s.hrBpm;
        mHoldNext = static_cast<uint8_t>((mHoldNext + 1) % Config::kAutoMaxHoldSec);
        if (mHoldCount < Config::kAutoMaxHoldSec) {
            ++mHoldCount;
        }
        if (mHoldCount == Config::kAutoMaxHoldSec) {
            const uint8_t held = *std::min_element(mHold, mHold + Config::kAutoMaxHoldSec);
            mSustainedMax = std::max(mSustainedMax, held);
        }
    } else {
        mHoldCount = 0;
    }

    ++mSec;
    const bool gradeOk = s.gradeValid;
    if (gradeOk && (s.gradePct > Config::kMaxGradePct || s.gradePct < Config::kMinGradePct)) {
        mTooSteep = true;
    }
    if (goodHr(s) && goodSpeed(s)) {
        if (mGood == 0) {
            mSpeedRef = s.speedMs;
        }
        ++mGood;
        const float d = s.speedMs - mSpeedRef;
        mSumSpeed  += d;
        mSumSpeed2 += d * d;
        // No valid grade: taken as level. The SDK's GRADE sensor has its own
        // validity; a missing one should not cost the window.
        mSumGrade  += gradeOk ? s.gradePct : 0.0f;
        mSumHr     += s.hrBpm;
        mMinHr      = std::min(mMinHr, s.hrBpm);
        mMaxHr      = std::max(mMaxHr, s.hrBpm);
    }

    if (mSec >= Config::kWindowSec) {
        closeWindow();
        clearWindow();
    }
}

bool Vo2Run::popWindowLog(WindowLog& out)
{
    if (!mLogReady) {
        return false;
    }
    out = mLog;
    mLogReady = false;
    return true;
}

void Vo2Run::closeWindow()
{
    // The window's figures first (the tuning log wants them whatever the
    // outcome), then the same tests in the same order as before.
    WindowLog& log = mLog;
    log           = WindowLog{};
    log.index     = ++mWindowIndex;
    log.activeSec = mActiveSec;
    log.goodSec   = mGood;
    mLogReady     = true;

    float mean = 0.0f, spread = 0.0f, grade = 0.0f, meanHr = 0.0f, cost = 0.0f;
    if (mGood > 0) {
        const float n   = static_cast<float>(mGood);
        const float dm  = mSumSpeed / n;
        mean            = mSpeedRef + dm;
        const float var = std::max(0.0f, mSumSpeed2 / n - dm * dm);
        spread          = 100.0f * std::sqrt(var) / mean;
        grade           = mSumGrade / n;
        meanHr          = static_cast<float>(mSumHr) / n;
        cost            = acsmRunningVo2(mean, grade);
        log.speedMMinX10 = static_cast<uint16_t>(std::lround(mean * 600.0f));
        log.spreadX10    = static_cast<uint16_t>(std::lround(std::min(spread, 6000.0f) * 10.0f));
        log.gradeX10     = static_cast<int16_t>(std::lround(std::max(-3000.0f, std::min(grade, 3000.0f)) * 10.0f));
        log.hrX10        = static_cast<uint16_t>(std::lround(meanHr * 10.0f));
        log.hrMin        = mMinHr;
        log.hrMax        = mMaxHr;
        log.costX10      = static_cast<uint16_t>(std::lround(cost * 10.0f));
    }

    if (mActiveSec <= Config::kWarmUpSec) {
        ++mCounts.warmUp;
        log.outcome = WindowOutcome::WarmUp;
        return;
    }
    if (mGood < Config::kMinGoodSecPerWindow) {
        ++mCounts.gaps;
        log.outcome = WindowOutcome::Gaps;
        return;
    }
    if (mTooSteep) {
        ++mCounts.grade;
        log.outcome = WindowOutcome::Grade;
        return;
    }
    if (spread > Config::kMaxSpeedSpreadPct || (mMaxHr - mMinHr) > Config::kMaxHrRangeBpm) {
        ++mCounts.unsteady;
        log.outcome = WindowOutcome::Unsteady;
        return;
    }
    if (mWindowCount >= Config::kMaxWindows) {
        ++mCounts.overflow;
        log.outcome = WindowOutcome::Overflow;
        return;
    }

    mWindows[mWindowCount++] = Window{log.costX10, log.hrX10};
    ++mCounts.accepted;
    log.outcome = WindowOutcome::Kept;
}

RunResult Vo2Run::estimate(const Profile& profile) const
{
    RunResult r;
    r.profileStatus = profile.status;
    if (profile.status != ProfileStatus::Ok) {
        r.status = RunStatus::ProfileIncomplete;
        return r;
    }

    // Per-window VO2max x 10, kept only within the intensity and sanity limits.
    uint16_t est[Config::kMaxWindows];
    uint16_t n = 0;
    const float rest    = static_cast<float>(profile.restHr);
    const float reserve = static_cast<float>(profile.maxHr) - rest;
    for (uint16_t i = 0; i < mWindowCount; ++i) {
        const float hr  = mWindows[i].hrX10 / 10.0f;
        const float hrr = (hr - rest) / reserve;
        if (hrr > 1.0f) {
            ++r.aboveMax;
            continue;
        }
        if (hrr * 100.0f < Config::kMinHrrPct) {
            ++r.tooEasy;
            continue;
        }
        const float cost = mWindows[i].costX10 / 10.0f;
        const float vo2  = 3.5f + (cost - 3.5f) / hrr;
        if (vo2 < Config::kMinPlausibleVo2 || vo2 > Config::kMaxPlausibleVo2) {
            ++r.implausible;
            continue;
        }
        est[n++] = static_cast<uint16_t>(std::lround(vo2 * 10.0f));
    }

    r.windowsUsed = n;
    if (n < Config::kMinWindows) {
        // Say why: enough steady running that the profile then ruled out
        // reads differently from too little steady running.
        if (n + r.aboveMax >= Config::kMinWindows && r.aboveMax >= r.tooEasy) {
            r.status = RunStatus::HrAboveMax;
        } else if (n + r.tooEasy >= Config::kMinWindows) {
            r.status = RunStatus::TooEasy;
        } else {
            r.status = RunStatus::NotEnoughRunning;
        }
        return r;
    }

    std::sort(est, est + n);
    r.vo2x10 = (n % 2 == 1)
        ? est[n / 2]
        : static_cast<uint16_t>((est[n / 2 - 1] + est[n / 2] + 1) / 2);
    r.status = RunStatus::Ok;
    return r;
}

}  // namespace RunVo2
