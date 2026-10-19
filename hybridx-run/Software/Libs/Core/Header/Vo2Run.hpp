/**
 ******************************************************************************
 * @file    Vo2Run.hpp
 * @brief   One run's VO2max: steady one-minute windows, then an estimate.
 *
 * The service calls addSecond() once per track tick (1 Hz) with the values it
 * already latches; a WindowBuilder turns them into window records, and this
 * class judges each record. Windows built elsewhere (HybridX Trail's, read
 * from the shared folder) come in through addWindow() and are judged by the
 * same rules. Each window that is steady enough keeps two numbers:
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

#include <algorithm>
#include <cstdint>

#include "Vo2Config.hpp"
#include "Vo2Profile.hpp"
#include "Vo2Window.hpp"

namespace RunVo2
{

/// Why a window was not kept (counts are for the NOTES and the tests).
struct WindowCounts {
    uint16_t accepted   = 0;
    uint16_t warmUp     = 0;   ///< inside the warm-up
    uint16_t gaps       = 0;   ///< too few good seconds (HR, GPS, pause, speed)
    uint16_t unsteady   = 0;   ///< speed or HR not steady
    uint16_t grade      = 0;   ///< too steep, or steep downhill
    uint16_t overflow   = 0;   ///< accepted but no room left
};

// Values are sent to the GUI as uint8_t (CustomMessage::Vo2Info): append only.
/// What happened to a window (the tuning log; Vo2Log names them).
enum class WindowOutcome : uint8_t { Kept, WarmUp, Gaps, Unsteady, Grade, Overflow };

/// One closed window, as the tuning log records it. Means are over the good
/// seconds; all zero when there were none.
struct WindowLog {
    uint16_t      index        = 0;   ///< 1 = the run's first window
    uint32_t      activeSec    = 0;   ///< active seconds at the window's end
    uint16_t      goodSec      = 0;
    WindowOutcome outcome      = WindowOutcome::Gaps;
    uint16_t      speedMMinX10 = 0;   ///< mean speed, m/min x 10
    uint16_t      spreadX10    = 0;   ///< speed spread, % x 10
    int16_t       gradeX10     = 0;   ///< mean grade, % x 10 (unknown seconds as 0)
    uint16_t      hrX10        = 0;   ///< mean HR, bpm x 10
    uint8_t       hrMin        = 0;
    uint8_t       hrMax        = 0;
    uint16_t      costX10      = 0;   ///< ACSM oxygen cost, ml/kg/min x 10
    uint16_t      speedScaleX1000 = 0; ///< mean GPS speed correction, x 1000; 0 = not known
};

enum class RunStatus : uint8_t {
    Ok,
    ProfileIncomplete,   ///< see Profile::status
    NotEnoughRunning,    ///< fewer than kMinWindows steady windows
    TooEasy,             ///< steady windows, but under the intensity floor
    HrAboveMax,          ///< steady windows, but HR above the max HR in use
};

struct RunResult {
    RunStatus     status        = RunStatus::NotEnoughRunning;
    ProfileStatus profileStatus = ProfileStatus::Ok;
    uint16_t      vo2x10        = 0;   ///< ml/kg/min x 10
    uint16_t      windowsUsed   = 0;   ///< windows behind the estimate
    uint16_t      tooEasy       = 0;   ///< steady windows under the intensity floor
    uint16_t      aboveMax      = 0;   ///< steady windows with HR above max HR
    uint16_t      implausible   = 0;   ///< steady windows giving a result outside 15-95
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

    /// Judge one window built elsewhere (Trail's, from the shared folder) and
    /// keep it if it counts. Same rules as the run's own windows; no log.
    /// Returns the outcome.
    WindowOutcome addWindow(const WindowRecord& w);

    /// Highest HR held for kAutoMaxHoldSec with good trust after the
    /// warm-up; 0 if none. Includes noteSustainedMax().
    uint8_t sustainedMaxHr() const { return std::max(mBuilder.sustainedMaxHr(), mNotedMax); }

    /// The run's average speed as if level, m/s (0 = none), and the seconds
    /// behind it. See WindowBuilder.
    float    avgGradeAdjustedSpeedMs() const { return mBuilder.avgGradeAdjustedSpeedMs(); }
    uint32_t movingSec() const { return mBuilder.movingSec(); }

    /// A sustained max HR measured elsewhere, for a run whose windows came in
    /// through addWindow().
    void noteSustainedMax(uint8_t bpm) { mNotedMax = std::max(mNotedMax, bpm); }

    const WindowCounts& counts() const { return mCounts; }

    /// The window that closed since the last call, for the tuning log; false
    /// when none has. Call once per addSecond().
    bool popWindowLog(WindowLog& out);

private:
    struct Window {
        uint16_t costX10;   ///< ACSM VO2, ml/kg/min x 10
        uint16_t hrX10;     ///< mean HR, bpm x 10
    };

    /// The tests in their fixed order; stores the window when it counts.
    WindowOutcome judge(const WindowRecord& w, uint16_t& costX10);

    WindowBuilder mBuilder;
    uint8_t       mNotedMax = 0;

    WindowLog mLog;
    bool      mLogReady = false;

    Window   mWindows[Config::kMaxWindows] = {};
    uint16_t mWindowCount = 0;
    WindowCounts mCounts;
};

}  // namespace RunVo2

#endif  // RUN_VO2_RUN_HPP
