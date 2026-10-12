/**
 ******************************************************************************
 * @file    GlanceView.cpp
 * @brief   What the glance has to show (see the header).
 ******************************************************************************
 */

#include "GlanceView.hpp"

#include "Trend.hpp"

namespace XWeighIn
{
namespace Glance
{

void makeView(const Trend& trend, int32_t today, bool clockSet, View& out)
{
    out = View {};
    const Summary s = trend.summary();
    if (s.days == 0) {
        return;
    }
    out.hasData       = true;
    out.trend         = s.trend;
    out.hasWeekChange = s.hasWeekChange;
    out.weekChange    = s.weekChange;
    out.lastDay       = s.lastDay;
    out.endDay        = clockSet && today > s.lastDay ? today : s.lastDay;

    for (uint8_t i = 0; i < kGlanceDays; ++i) {
        const int32_t day = out.endDay - (kGlanceDays - 1) + i;
        DayPoint p;
        // at() is false outside the log's last kHistoryDays days and after its
        // last day: no bar.
        if (trend.at(day, p) && p.hundredths > 0) {
            out.weight[i] = p.hundredths > 0xFFFF ? 0xFFFF : static_cast<uint16_t>(p.hundredths);
        }
    }
}

}  // namespace Glance
}  // namespace XWeighIn
