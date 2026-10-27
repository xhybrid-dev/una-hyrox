/**
 ******************************************************************************
 * @file    FormStats.hpp
 * @brief   Running form: the average cadence and step length of a run and of
 *          each lap.
 *
 * Fed once per active second with that second's cadence (steps per minute) and
 * step length, each only when it is valid and fresh. The service fills the
 * second from the same values it writes to the FIT record, so the screen, the
 * averages and the file agree.
 *
 *   - Average cadence is the mean over the seconds that had a valid cadence.
 *   - Average step length is the mean weighted by cadence, over the seconds that
 *     had both: sum(step * cadence) / sum(cadence). Each second's distance is
 *     step * cadence / 60, so this is total distance / total steps: it agrees
 *     with the run's distance and step count, as a plain mean of step lengths
 *     would not when the cadence changes.
 *
 * Integers only: cadence in tenths of a step per minute, step length in
 * millimetres. 0 means no data (never a real value: cadence is above 0 and step
 * length is gated to 150-2500 mm by the SDK). Pure C++, no kernel.
 ******************************************************************************
 */

#ifndef RUN_FORM_STATS_HPP
#define RUN_FORM_STATS_HPP

#include <cstdint>

namespace RunVo2
{

/// One active second of running form.
struct FormSecond {
    bool     cadenceValid = false;   ///< a cadence reading that is valid and fresh
    uint16_t cadenceX10   = 0;       ///< steps per minute x 10
    bool     stepValid    = false;   ///< a step length that is valid (needs a valid cadence)
    uint16_t stepMm       = 0;       ///< millimetres
};

class FormStats
{
public:
    /// A new run: forget everything.
    void reset();
    /// A new lap: forget the lap's totals only.
    void resetLap();

    /// One active second. A paused second is not added.
    void addSecond(const FormSecond& s);

    /// The run's averages; 0 when no second had the data.
    uint16_t cadenceAvgX10() const { return average(mRun.cadenceSumX10, mRun.cadenceSeconds); }
    uint16_t stepAvgMm() const { return stepAverage(mRun); }
    /// The lap's.
    uint16_t lapCadenceAvgX10() const { return average(mLap.cadenceSumX10, mLap.cadenceSeconds); }
    uint16_t lapStepAvgMm() const { return stepAverage(mLap); }

    /// Seconds that had a valid cadence, for the run and the lap (for tests and logs).
    uint32_t cadenceSeconds() const { return mRun.cadenceSeconds; }
    uint32_t lapCadenceSeconds() const { return mLap.cadenceSeconds; }

private:
    struct Totals {
        uint32_t cadenceSumX10  = 0;   ///< sum of cadence x 10 over valid-cadence seconds
        uint32_t cadenceSeconds = 0;
        uint64_t stepWeighted   = 0;   ///< sum of step mm x cadence x 10 over both-valid seconds
        uint32_t stepWeight     = 0;   ///< sum of cadence x 10 over the same seconds
    };

    static uint16_t average(uint32_t sum, uint32_t n);
    static uint16_t stepAverage(const Totals& t);
    static void     add(Totals& t, const FormSecond& s);

    Totals mRun;
    Totals mLap;
};

}  // namespace RunVo2

#endif  // RUN_FORM_STATS_HPP
