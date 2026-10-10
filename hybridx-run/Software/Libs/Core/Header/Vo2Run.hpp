/**
 ******************************************************************************
 * @file    Vo2Run.hpp
 * @brief   One run's VO2max: steady one-minute windows, then an estimate.
 *
 * The service calls addSecond() once per track tick (1 Hz) with the values it
 * already latches. Each full window that is steady enough keeps two numbers:
 * its oxygen cost from the ACSM running equation,
 *     VO2 = 3.5 + 0.2 v + 0.9 v grade   (ml/kg/min, v in m/min),
 * and its mean HR. estimate() turns each into a VO2max with the athlete's
 * profile, using %HRR ~ %VO2R (Swain and Leutholtz 1997):
 *     VO2max = 3.5 + (VO2 - 3.5) / %HRR,
 * and takes the median.
 *
 * Fixed memory (about 1 KB), no allocation, float maths (the watch has an
 * FPU). Limits: Vo2Config.hpp.
 ******************************************************************************
 */

#ifndef RUN_VO2_RUN_HPP
#define RUN_VO2_RUN_HPP

#include <cstdint>

#include "Vo2Config.hpp"
#include "Vo2Profile.hpp"

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
    uint8_t hrTrust       = 0;
};

/// Why a window was not kept (counts are for the NOTES and the tests).
struct WindowCounts {
    uint16_t accepted   = 0;
    uint16_t warmUp     = 0;   ///< inside the warm-up
    uint16_t gaps       = 0;   ///< too few good seconds (HR, GPS, pause, speed)
    uint16_t unsteady   = 0;   ///< speed or HR not steady
    uint16_t grade      = 0;   ///< too steep, or steep downhill
    uint16_t overflow   = 0;   ///< accepted but no room left
};

enum class RunStatus : uint8_t {
    Ok,
    ProfileIncomplete,   ///< see Profile::status
    NotEnoughRunning,    ///< fewer than kMinWindows usable windows
};

struct RunResult {
    RunStatus     status        = RunStatus::NotEnoughRunning;
    ProfileStatus profileStatus = ProfileStatus::Ok;
    uint16_t      vo2x10        = 0;   ///< ml/kg/min x 10
    uint16_t      windowsUsed   = 0;   ///< windows behind the estimate
};

class Vo2Run
{
public:
    /// Forget everything; call at track start.
    void reset();

    /// One track tick.
    void addSecond(const Sample& s);

    /// The estimate so far, with this profile. Can be called at any time.
    RunResult estimate(const Profile& profile) const;

    /// Highest HR held for kAutoMaxHoldSec with good trust; 0 if none.
    uint8_t sustainedMaxHr() const { return mSustainedMax; }

    const WindowCounts& counts() const { return mCounts; }

private:
    struct Window {
        uint16_t costX10;   ///< ACSM VO2, ml/kg/min x 10
        uint16_t hrX10;     ///< mean HR, bpm x 10
    };

    void closeWindow();
    void clearWindow();

    // Current window.
    uint16_t mSec       = 0;
    uint16_t mGood      = 0;
    float    mSumSpeed  = 0.0f;
    float    mSumSpeed2 = 0.0f;
    float    mSumGrade  = 0.0f;
    uint32_t mSumHr     = 0;
    uint8_t  mMinHr     = 255;
    uint8_t  mMaxHr     = 0;
    bool     mTooSteep  = false;

    uint32_t mActiveSec = 0;

    Window   mWindows[Config::kMaxWindows] = {};
    uint16_t mWindowCount = 0;
    WindowCounts mCounts;

    // Auto max: the last kAutoMaxHoldSec good HR readings.
    uint8_t  mHold[Config::kAutoMaxHoldSec] = {};
    uint8_t  mHoldCount = 0;
    uint8_t  mHoldNext  = 0;
    uint8_t  mSustainedMax = 0;
};

}  // namespace RunVo2

#endif  // RUN_VO2_RUN_HPP
