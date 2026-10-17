/**
 ******************************************************************************
 * @file    Window.cpp
 * @brief   Packing the trend's last days for the GUI (see the header).
 ******************************************************************************
 */

#include "Window.hpp"

#include "Trend.hpp"

namespace XWeighIn
{

namespace
{
uint16_t to16(int32_t hundredths)
{
    return hundredths <= 0 ? 0 : (hundredths > 0xFFFF ? 0xFFFF : static_cast<uint16_t>(hundredths));
}
}  // namespace

void fillWindow(const Trend& trend, Window& out)
{
    out = Window {};
    const Summary s = trend.summary();
    if (s.days == 0) {
        return;
    }
    int32_t first = s.lastDay - (kWindowDays - 1);
    if (first < s.firstDay) {
        first = s.firstDay;
    }
    out.firstDay = first;
    for (int32_t day = first; day <= s.lastDay && out.count < kWindowDays; ++day) {
        DayPoint p;
        if (!trend.at(day, p)) {
            break;   // cannot happen inside the window; stop rather than leave a gap
        }
        out.weight[out.count] = to16(p.hundredths);
        out.trend[out.count]  = to16(p.trend);
        ++out.count;
    }
}

}  // namespace XWeighIn
