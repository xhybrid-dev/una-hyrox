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
}

void Vo2Run::addSecond(const Sample& s)
{
    mBuilder.addSecond(s);

    WindowRecord w;
    if (!mBuilder.popWindow(w)) {
        return;
    }
    uint16_t cost = 0;
    const WindowOutcome outcome = judge(w, cost);

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
    mLogReady            = true;
}

WindowOutcome Vo2Run::addWindow(const WindowRecord& w)
{
    uint16_t cost = 0;
    return judge(w, cost);
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

WindowOutcome Vo2Run::judge(const WindowRecord& w, uint16_t& costX10)
{
    costX10 = 0;
    if (w.goodSec > 0) {
        costX10 = static_cast<uint16_t>(std::lround(acsmRunningVo2(w.speedMMinX10, w.gradeX10) * 10.0f));
    }

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
    if (mWindowCount >= Config::kMaxWindows) {
        ++mCounts.overflow;
        return WindowOutcome::Overflow;
    }

    mWindows[mWindowCount++] = Window{costX10, w.hrX10};
    ++mCounts.accepted;
    return WindowOutcome::Kept;
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
