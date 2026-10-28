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

/// ACSM running equation, ml/kg/min. Speed in m/min x 10, grade in percent
/// x 10 (a window record's units); downhill counts as 0.
float acsmRunningVo2(uint16_t speedMMinX10, int16_t gradeX10)
{
    const float v     = speedMMinX10 / 10.0f;
    const float grade = gradeX10 > 0 ? gradeX10 / 1000.0f : 0.0f;
    return 3.5f + 0.2f * v + 0.9f * v * grade;
}

}  // namespace

void Vo2Run::reset()
{
    mBuilder.reset();
    mNotedMax     = 0;
    mLog          = WindowLog{};
    mLogReady     = false;
    mWindowCount  = 0;
    mCounts       = WindowCounts{};
    mHavePrev      = false;
    mPrevActiveSec = 0;
    mPrevSpeedX10  = 0;
    mLagKnownOnly  = false;
    mLastWindow    = WindowRecord{};
}

void Vo2Run::addSecond(const Sample& s)
{
    mBuilder.addSecond(s);

    WindowRecord w;
    if (!mBuilder.popWindow(w)) {
        return;
    }
    mLastWindow = w;
    uint16_t cost = 0;
    bool     afterChange = false;
    const WindowOutcome outcome = judge(w, cost, afterChange);

    mLog                 = WindowLog{};
    mLog.index           = w.index;
    mLog.activeSec       = w.activeSec;
    mLog.goodSec         = w.goodSec;
    mLog.outcome         = outcome;
    mLog.speedMMinX10    = w.speedMMinX10;
    mLog.spreadX10       = w.spreadX10;
    mLog.gradeX10        = w.gradeX10;
    mLog.hrX10           = w.hrX10;
    mLog.hrMin           = w.hrMin;
    mLog.hrMax           = w.hrMax;
    mLog.costX10         = cost;
    mLog.speedScaleX1000 = w.speedScaleX1000;
    mLog.afterChange     = afterChange;
    mLogReady            = true;
}

WindowOutcome Vo2Run::addWindow(const WindowRecord& w)
{
    uint16_t cost = 0;
    bool     afterChange = false;
    return judge(w, cost, afterChange);
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

WindowOutcome Vo2Run::judge(const WindowRecord& w, uint16_t& costX10, bool& afterChange)
{
    costX10 = 0;
    if (w.goodSec > 0) {
        costX10 = static_cast<uint16_t>(std::lround(acsmRunningVo2(w.speedMMinX10, w.gradeX10) * 10.0f));
    }

    // HR lag (P3): was the minute before this one at the same pace, and was it the minute before?
    // Active seconds do not count a pause, but a pause drops the part-built window, so a window
    // after one ends more than a minute after the last. (A pause exactly on a window's edge is
    // not seen.) Every window judged becomes the next one's "before", whatever its outcome.
    const uint16_t speed = w.goodSec > 0 ? w.speedMMinX10 : 0;
    const bool contiguous = mHavePrev && w.activeSec - mPrevActiveSec == Config::kWindowSec;
    // Known only (a kept-minutes file): an unknown minute before is not a change.
    afterChange = !mLagKnownOnly;
    if (contiguous && mPrevSpeedX10 > 0 && speed > 0) {
        const float change = std::fabs(static_cast<float>(speed) - static_cast<float>(mPrevSpeedX10)) * 100.0f /
                             static_cast<float>(mPrevSpeedX10);
        afterChange = change > Config::kLagMaxSpeedChangePct;
    }
    mHavePrev      = true;
    mPrevActiveSec = w.activeSec;
    mPrevSpeedX10  = speed;

    if (w.activeSec <= Config::kWarmUpSec) {
        ++mCounts.warmUp;
        return WindowOutcome::WarmUp;
    }
    if (w.goodSec < Config::kMinGoodSecPerWindow) {
        ++mCounts.gaps;
        return WindowOutcome::Gaps;
    }
    // Steep: the window's mean is outside the limits, or too many of its
    // seconds were. One stray second is sensor noise, not a hill.
    const bool meanSteep = w.gradeX10 > static_cast<int16_t>(Config::kMaxGradePct * 10.0f) ||
                           w.gradeX10 < static_cast<int16_t>(Config::kMinGradePct * 10.0f);
    if (meanSteep || w.steepSec > Config::kMaxSteepSecPerWindow) {
        ++mCounts.grade;
        return WindowOutcome::Grade;
    }
    if (w.spreadX10 > static_cast<uint16_t>(Config::kMaxSpeedSpreadPct * 10.0f) ||
        (w.hrMax - w.hrMin) > Config::kMaxHrRangeBpm) {
        ++mCounts.unsteady;
        return WindowOutcome::Unsteady;
    }
    if (afterChange && Config::kLagGuardEnabled) {
        ++mCounts.lagged;
        return WindowOutcome::Lagged;
    }
    if (mWindowCount >= Config::kMaxWindows) {
        ++mCounts.overflow;
        return WindowOutcome::Overflow;
    }

    mWindows[mWindowCount++] = Window{costX10, w.hrX10, afterChange};
    ++mCounts.accepted;
    if (afterChange) {
        ++mCounts.afterChange;
    }
    return WindowOutcome::Kept;
}

bool Vo2Run::windowEstimate(const Window& w, const Profile& profile, RunResult& r, uint16_t& out) const
{
    const float rest    = static_cast<float>(profile.restHr);
    const float reserve = static_cast<float>(profile.maxHr) - rest;
    const float hr      = w.hrX10 / 10.0f;
    const float hrr     = (hr - rest) / reserve;
    if (hrr > 1.0f) {
        ++r.aboveMax;
        return false;
    }
    if (hrr * 100.0f < Config::kMinHrrPct) {
        ++r.tooEasy;
        return false;
    }
    const float cost = w.costX10 / 10.0f;
    const float vo2  = 3.5f + (cost - 3.5f) / hrr;
    if (vo2 < Config::kMinPlausibleVo2 || vo2 > Config::kMaxPlausibleVo2) {
        ++r.implausible;
        return false;
    }
    out = static_cast<uint16_t>(std::lround(vo2 * 10.0f));
    return true;
}

namespace
{

/// The median of the first n of @p v, sorting them in place.
uint16_t medianOf(uint16_t* v, uint16_t n)
{
    std::sort(v, v + n);
    return (n % 2 == 1) ? v[n / 2] : static_cast<uint16_t>((v[n / 2 - 1] + v[n / 2] + 1) / 2);
}

}  // namespace

RunResult Vo2Run::estimate(const Profile& profile) const
{
    RunResult r;
    r.profileStatus = profile.status;
    if (profile.status != ProfileStatus::Ok) {
        r.status = RunStatus::ProfileIncomplete;
        return r;
    }

    // Per-window VO2max x 10, kept only within the intensity and sanity limits, in run order.
    // Two arrays of 240: about 1 KB of the service's stack, as before plus the second.
    uint16_t est[Config::kMaxWindows];
    uint16_t tmp[Config::kMaxWindows];
    uint16_t n = 0;
    uint16_t clean = 0;   // used windows not after a change of pace
    for (uint16_t i = 0; i < mWindowCount; ++i) {
        uint16_t v = 0;
        if (!windowEstimate(mWindows[i], profile, r, v)) {
            continue;
        }
        est[n++] = v;
        if (mWindows[i].afterChange) {
            ++r.afterChange;
        } else {
            tmp[clean++] = v;
        }
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

    // P3, measured first. Without the windows after a change of pace (tmp holds them in order).
    if (clean >= Config::kMinWindows) {
        r.withoutLagX10 = medianOf(tmp, clean);
    }
    // The halves of the windows used, in run order (est is still in order here).
    const uint16_t half = n / 2;
    if (half >= Config::kMinHalfWindows && n - half >= Config::kMinHalfWindows) {
        std::copy(est, est + half, tmp);
        r.firstHalfX10 = medianOf(tmp, half);
        std::copy(est + half, est + n, tmp);
        r.secondHalfX10 = medianOf(tmp, static_cast<uint16_t>(n - half));
    }

    r.vo2x10 = medianOf(est, n);   // sorts est
    // Half the interquartile range (nearest rank): a typical distance of a window from the middle.
    const uint16_t q1 = est[n / 4];
    const uint16_t q3 = est[(3 * n) / 4 < n ? (3 * n) / 4 : n - 1];
    r.spreadX10 = static_cast<uint16_t>((q3 - q1 + 1) / 2);
    r.status = RunStatus::Ok;
    return r;
}

}  // namespace RunVo2
