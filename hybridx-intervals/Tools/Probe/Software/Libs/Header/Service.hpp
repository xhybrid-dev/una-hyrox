/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   The Intervals Probe's service: runs the check once, saves the report.
 *
 * On start it records the clock (so Jon can line the watch's time up against
 * the PC script's "sentAt" timestamp), runs Probe::Runner over its own
 * "Plans/" folder, writes probe.txt (replaced each run) and appends one line
 * to probe-history.txt (kept, so a run before and a run after sending a file
 * can be compared). Both files sit in the probe's own folder,
 * /Apps/HXIntervalsProbe/. It shows the result to the GUI whenever the GUI
 * starts, and exits once the GUI has gone.
 ******************************************************************************
 */

#ifndef INTERVALS_PROBE_SERVICE_HPP
#define INTERVALS_PROBE_SERVICE_HPP

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
    void line(const char* text) override;

    void     probe();
    void     clock();
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

#endif // INTERVALS_PROBE_SERVICE_HPP
