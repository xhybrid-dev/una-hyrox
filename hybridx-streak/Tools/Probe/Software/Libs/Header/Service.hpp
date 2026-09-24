/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   The Streak Probe's service: runs the checks once, saves the report.
 *
 * On start it records the clock, the system settings and the glance area,
 * runs Probe::Runner, writes probe.txt (the whole report, replaced each run)
 * and appends one line to probe-history.txt (kept, so a run before and a run
 * after a phone sync can be compared). Both files are in the probe's own
 * folder, /Apps/HXStreakProbe/. It then shows the result to the GUI
 * whenever the GUI starts, and exits when the GUI has gone.
 ******************************************************************************
 */

#ifndef STREAK_PROBE_SERVICE_HPP
#define STREAK_PROBE_SERVICE_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Kernel/Kernel.hpp"

#include "ProbeRunner.hpp"

class Service : private Probe::Host
{
public:
    explicit Service(SDK::Kernel& kernel);

    void run();

private:
    // Probe::Host
    void     line(const char* text) override;
    uint32_t nowMs() override;

    void     probe();
    void     clock();
    void     settings();
    void     glance();
    uint16_t historyRuns();
    void     save();
    void     sendResult();

    SDK::Kernel&  mKernel;
    Probe::Result mResult {};
    size_t        mReportLen  = 0;
    bool          mGuiStarted = false;
    char          mStamp[40] {};

    static constexpr uint32_t skStartupGraceMs = 5000u;
    static constexpr uint32_t skWaitMs         = 1000u;
};

#endif // STREAK_PROBE_SERVICE_HPP
