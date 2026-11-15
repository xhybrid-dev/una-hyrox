/**
 ******************************************************************************
 * @file    Vo2Window.hpp
 * @brief   Turns a run's seconds into one-minute window records.
 *
 * The service calls addSecond() once per track tick (1 Hz). Each full window
 * becomes a WindowRecord of integers: how many seconds were usable, the mean
 * speed, grade and heart rate, how steady they were, and how many seconds ran
 * outside the grade limits. The builder judges nothing. HybridX Run decides
 * which windows count and works out the VO2max (Vo2Run); HybridX Trail only
 * builds records and writes them to the shared folder for Run to read.
 *
 * Copied byte for byte into Trail (with Vo2WindowConfig.hpp). Fixed memory,
 * no allocation, no SDK headers.
 ******************************************************************************
 */

#ifndef RUN_VO2_WINDOW_HPP
#define RUN_VO2_WINDOW_HPP

#include <cstdint>

#include "Vo2WindowConfig.hpp"

namespace RunVo2
{

/// One second of the track, as the service has it.
struct Sample {
    bool    active        = false;   ///< track running (not paused)
    float   speedMs       = 0.0f;
    bool    speedValid    = false;
    bool    deadReckoning = false;
    float   gradePct      = 0.0f;
    bool    gradeValid    = false;
    uint8_t hrBpm         = 0;
    float   hrTrust       = 0.0f;   ///< the kernel's raw trust level
    float   speedScale    = 0.0f;   ///< the GPS speed correction in force (1.0 = none); 0 = not known
};

/// One closed window. Means are over the good seconds; all zero when there
/// were none.
struct WindowRecord {
    uint16_t index           = 0;   ///< 1 = the run's first window
    uint32_t activeSec       = 0;   ///< active seconds at the window's end
    uint16_t goodSec         = 0;   ///< seconds with good HR and speed
    uint16_t speedMMinX10    = 0;   ///< mean speed, m/min x 10
    uint16_t spreadX10       = 0;   ///< speed spread, % x 10
    int16_t  gradeX10        = 0;   ///< mean grade, % x 10 (unknown seconds as 0)
    uint8_t  steepSec        = 0;   ///< seconds with a valid grade outside the limits
    uint16_t hrX10           = 0;   ///< mean HR, bpm x 10
    uint8_t  hrMin           = 0;
    uint8_t  hrMax           = 0;
    uint16_t speedScaleX1000 = 0;   ///< mean GPS speed correction, x 1000; 0 = not known
};

class WindowBuilder
{
public:
    /// Forget everything; call at track start.
    void reset();

    /// One track tick.
    void addSecond(const Sample& s);

    /// The window that closed since the last call; false when none has. Call
    /// once per addSecond().
    bool popWindow(WindowRecord& out);

    /// Highest HR held for kAutoMaxHoldSec with good trust after the
    /// warm-up; 0 if none.
    uint8_t sustainedMaxHr() const { return mSustainedMax; }

    uint32_t activeSec() const { return mActiveSec; }

    /// The run's average speed as if level (ACSM's uphill gain removed), m/s,
    /// over the seconds spent moving; 0 when there were none. Pace is 1 / this.
    float avgGradeAdjustedSpeedMs() const;

    /// Seconds behind it.
    uint32_t movingSec() const { return mGapSec; }

private:
    void closeWindow();
    void clearWindow();

    // Current window.
    uint16_t mSec       = 0;
    uint16_t mGood      = 0;
    // Speed sums are of the deviation from the window's first good sample
    // (a shifted sum): E[x^2] - E[x]^2 on raw single-precision speeds loses
    // the variance of a steady pace to rounding.
    float    mSpeedRef  = 0.0f;
    float    mSumSpeed  = 0.0f;   ///< sum of (speed - mSpeedRef)
    float    mSumSpeed2 = 0.0f;   ///< sum of (speed - mSpeedRef)^2
    float    mSumGrade  = 0.0f;
    float    mSumScale  = 0.0f;   ///< sum of the speed correction over the good seconds
    uint32_t mSumHr     = 0;
    uint8_t  mMinHr     = 255;
    uint8_t  mMaxHr     = 0;
    uint16_t mSteepSec  = 0;

    uint32_t mActiveSec   = 0;
    uint16_t mWindowIndex = 0;

    // Average grade-adjusted speed: the sum of each moving second's level-
    // equivalent speed. A float sum of well under 10^5 terms of under 10 m/s
    // keeps full precision for the average.
    double   mGapSum = 0.0;
    uint32_t mGapSec = 0;

    WindowRecord mClosed;
    bool         mClosedReady = false;

    // Auto max: the last kAutoMaxHoldSec good HR readings.
    uint8_t  mHold[Config::kAutoMaxHoldSec] = {};
    uint8_t  mHoldCount    = 0;
    uint8_t  mHoldNext     = 0;
    uint8_t  mSustainedMax = 0;
};

}  // namespace RunVo2

#endif  // RUN_VO2_WINDOW_HPP
