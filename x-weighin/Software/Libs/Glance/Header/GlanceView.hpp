/**
 ******************************************************************************
 * @file    GlanceView.hpp
 * @brief   What the glance has to show, taken from the trend: pure,
 *          host-tested (GlanceViewTest).
 *
 * The glance reads the app's SharedData copy of the log into a Trend, the
 * same code the app uses, so the two always agree. This picks out the
 * headline (the trend at the last weigh-in and the weekly change, as on
 * Home) and the last kGlanceDays days ending today, one weigh-in each or
 * none. When the clock is not set the days end at the log's last day.
 ******************************************************************************
 */

#ifndef XWEIGHIN_GLANCE_VIEW_HPP
#define XWEIGHIN_GLANCE_VIEW_HPP

#include <cstdint>

namespace XWeighIn
{

class Trend;

namespace Glance
{

/// Days the glance shows as bars, the last one today.
constexpr uint8_t kGlanceDays = 14;

struct View {
    bool     hasData       = false;   ///< At least one weigh-in in the log.
    int32_t  trend         = 0;       ///< The trend at the last weigh-in, 0.01 kg.
    bool     hasWeekChange = false;   ///< The log spans at least a week.
    int32_t  weekChange    = 0;       ///< As Home's "this week", 0.01 kg.
    int32_t  lastDay       = 0;       ///< The log's last day.
    int32_t  endDay        = 0;       ///< The day of the last bar.
    /// The weigh-in of each day, oldest first, the last at endDay; 0 = none.
    /// 0.01 kg fits 16 bits: the log caps weights at 500.00 kg.
    uint16_t weight[kGlanceDays] = {};
};

/**
 * @brief   Fill @p out from @p trend.
 * @param   today     The local day number now.
 * @param   clockSet  Whether @p today can be trusted (WeighProfile::clockIsSet).
 *                    If not, or if today is before the log's last day (the
 *                    clock went back), the bars end at the last day.
 */
void makeView(const Trend& trend, int32_t today, bool clockSet, View& out);

}  // namespace Glance
}  // namespace XWeighIn

#endif  // XWEIGHIN_GLANCE_VIEW_HPP
