/**
 ******************************************************************************
 * @file    Window.hpp
 * @brief   The last kWindowDays days of the trend, packed small enough for one
 *          service -> GUI message (the kernel's pool blocks are 256 bytes).
 *
 * The struct and its readers are header-only (the GUI uses them); filling it
 * from a Trend is in Window.cpp, on the service side. Host-tested.
 ******************************************************************************
 */

#ifndef XWEIGHIN_WINDOW_HPP
#define XWEIGHIN_WINDOW_HPP

#include <cstdint>

namespace XWeighIn
{

class Trend;

/// Days the graph shows, and the most a Window holds.
constexpr uint8_t kWindowDays = 30;

/**
 * @brief   Up to kWindowDays consecutive days ending at the log's last day.
 *
 * Weights in 0.01 kg fit 16 bits: the log caps them at 500.00 kg (50 000).
 * weight[i] is 0 on a day with no weigh-in (a filled-in day); trend[i] is
 * always set for i < count.
 */
struct Window {
    int32_t  firstDay = 0;                 ///< Local day number of index 0.
    uint8_t  count    = 0;                 ///< Days held, 0..kWindowDays.
    uint16_t weight[kWindowDays] = {};
    uint16_t trend[kWindowDays]  = {};
};

/// Fill @p out from @p trend: the last kWindowDays days (fewer if the log is
/// younger). Empty (count 0) when the trend has no data.
void fillWindow(const Trend& trend, Window& out);

/// One row of the Recent page.
struct RecentDay {
    int32_t  day        = 0;
    uint16_t hundredths = 0;
};

/**
 * @brief   The days in @p w with a weigh-in, newest first, at most @p max.
 * @return  How many were written to @p out.
 */
inline uint8_t recentDays(const Window& w, RecentDay* out, uint8_t max)
{
    uint8_t n = 0;
    const uint8_t count = w.count <= kWindowDays ? w.count : kWindowDays;
    for (int i = static_cast<int>(count) - 1; i >= 0 && n < max; --i) {
        if (w.weight[i] != 0) {
            out[n].day        = w.firstDay + i;
            out[n].hundredths = w.weight[i];
            ++n;
        }
    }
    return n;
}

}  // namespace XWeighIn

#endif  // XWEIGHIN_WINDOW_HPP
