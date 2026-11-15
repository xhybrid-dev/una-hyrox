/**
 ******************************************************************************
 * @file    Vo2Window.cpp
 * @brief   Turns a run's seconds into one-minute window records.
 ******************************************************************************
 */

#include "Vo2Window.hpp"

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

}  // namespace

void WindowBuilder::reset()
{
    // Field by field: a whole-object temporary would put the window record and
    // the hold buffer on the service's 10 KB stack for nothing.
    clearWindow();
    mActiveSec    = 0;
    mWindowIndex  = 0;
    mGapSum       = 0.0;
    mGapSec       = 0;
    mClosedReady  = false;
    mHoldCount    = 0;
    mHoldNext     = 0;
    mSustainedMax = 0;
}

void WindowBuilder::clearWindow()
{
    mSec       = 0;
    mGood      = 0;
    mSpeedRef  = 0.0f;
    mSumSpeed  = 0.0f;
    mSumSpeed2 = 0.0f;
    mSumGrade  = 0.0f;
    mSumScale  = 0.0f;
    mSumHr     = 0;
    mMinHr     = 255;
    mMaxHr     = 0;
    mSteepSec  = 0;
}

void WindowBuilder::addSecond(const Sample& s)
{
    if (!s.active) {
        // A pause breaks the window: what it held was not one steady minute.
        clearWindow();
        mHoldCount = 0;
        return;
    }

    ++mActiveSec;

    // Average grade-adjusted speed: every moving second, whatever its heart
    // rate, so it is the run's, not the steady windows'.
    if (s.speedValid && !s.deadReckoning && s.speedMs >= Config::kMinMovingMs) {
        float grade = 0.0f;
        if (s.gradeValid) {
            grade = std::max(0.0f, std::min(s.gradePct, Config::kGapMaxGradePct)) / 100.0f;
        }
        mGapSum += static_cast<double>(s.speedMs) * (1.0 + static_cast<double>(Config::kAcsmGradeFactor * grade));
        ++mGapSec;
    }

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
        if (mSteepSec < 255) {
            ++mSteepSec;
        }
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
        mSumScale  += s.speedScale;
        mSumHr     += s.hrBpm;
        mMinHr      = std::min(mMinHr, s.hrBpm);
        mMaxHr      = std::max(mMaxHr, s.hrBpm);
    }

    if (mSec >= Config::kWindowSec) {
        closeWindow();
        clearWindow();
    }
}

float WindowBuilder::avgGradeAdjustedSpeedMs() const
{
    return mGapSec == 0 ? 0.0f : static_cast<float>(mGapSum / static_cast<double>(mGapSec));
}

bool WindowBuilder::popWindow(WindowRecord& out)
{
    if (!mClosedReady) {
        return false;
    }
    out          = mClosed;
    mClosedReady = false;
    return true;
}

void WindowBuilder::closeWindow()
{
    WindowRecord& w = mClosed;
    w           = WindowRecord{};
    w.index     = ++mWindowIndex;
    w.activeSec = mActiveSec;
    w.goodSec   = mGood;
    w.steepSec  = static_cast<uint8_t>(mSteepSec);
    mClosedReady = true;

    if (mGood == 0) {
        return;
    }
    const float n   = static_cast<float>(mGood);
    const float dm  = mSumSpeed / n;
    const float mean = mSpeedRef + dm;
    const float var = std::max(0.0f, mSumSpeed2 / n - dm * dm);
    const float spread = 100.0f * std::sqrt(var) / mean;
    const float grade  = mSumGrade / n;
    const float meanHr = static_cast<float>(mSumHr) / n;

    w.speedMMinX10    = static_cast<uint16_t>(std::lround(mean * 600.0f));
    w.spreadX10       = static_cast<uint16_t>(std::lround(std::min(spread, 6000.0f) * 10.0f));
    w.gradeX10        = static_cast<int16_t>(std::lround(std::max(-3000.0f, std::min(grade, 3000.0f)) * 10.0f));
    w.hrX10           = static_cast<uint16_t>(std::lround(meanHr * 10.0f));
    w.hrMin           = mMinHr;
    w.hrMax           = mMaxHr;
    w.speedScaleX1000 = static_cast<uint16_t>(std::lround(std::max(0.0f, std::min(mSumScale / n, 65.0f)) * 1000.0f));
}

}  // namespace RunVo2
