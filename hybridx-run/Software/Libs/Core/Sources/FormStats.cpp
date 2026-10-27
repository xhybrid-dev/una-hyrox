/**
 ******************************************************************************
 * @file    FormStats.cpp
 * @brief   Running form averages (see the header).
 ******************************************************************************
 */

#include "FormStats.hpp"

namespace RunVo2
{

void FormStats::reset()
{
    mRun = Totals {};
    mLap = Totals {};
}

void FormStats::resetLap()
{
    mLap = Totals {};
}

uint16_t FormStats::average(uint32_t sum, uint32_t n)
{
    if (n == 0) {
        return 0;
    }
    const uint32_t avg = (sum + n / 2) / n;   // rounded to nearest
    return avg > 0xFFFFu ? 0xFFFFu : static_cast<uint16_t>(avg);
}

uint16_t FormStats::stepAverage(const Totals& t)
{
    if (t.stepWeight == 0) {
        return 0;
    }
    const uint64_t avg = (t.stepWeighted + t.stepWeight / 2) / t.stepWeight;
    return avg > 0xFFFFu ? 0xFFFFu : static_cast<uint16_t>(avg);
}

void FormStats::add(Totals& t, const FormSecond& s)
{
    if (!s.cadenceValid || s.cadenceX10 == 0) {
        return;   // no cadence: nothing to add, and a step length needs one
    }
    // Totals do not wrap in any run the file format can hold: 86,400 s x 3,000 x10 steps/min is
    // 2.6e8 in a uint32, and the weighted sum is 64-bit.
    t.cadenceSumX10 += s.cadenceX10;
    ++t.cadenceSeconds;
    if (s.stepValid && s.stepMm != 0) {
        t.stepWeighted += static_cast<uint64_t>(s.stepMm) * s.cadenceX10;
        t.stepWeight += s.cadenceX10;
    }
}

void FormStats::addSecond(const FormSecond& s)
{
    add(mRun, s);
    add(mLap, s);
}

}  // namespace RunVo2
