/**
 ******************************************************************************
 * @file    WorkoutSettings.cpp
 * @brief   The settings file (see the header).
 ******************************************************************************
 */

#include "WorkoutSettings.hpp"

#include <cstring>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"
#include "WorkoutModel.hpp"

namespace Workout {

size_t WorkoutSettings::toJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(1));
    w.add("autolap", autoLapMin);
    w.endMap();
    if (w.isError()) {
        buf[0] = '\0';
        return 0;
    }
    return std::strlen(buf);
}

bool WorkoutSettings::fromJson(const char* text, size_t len)
{
    if (text == nullptr || len == 0) {
        return false;
    }
    SDK::JsonStreamReader r(text, len);
    int64_t v = 0;
    int64_t lap = 0;
    if (!r.validate() || !r.get("v", v) || v != 1 || !r.get("autolap", lap)) {
        return false;
    }
    for (uint8_t i = 0; i < kAutoLapCount; ++i) {
        if (kAutoLapMinutes[i] == lap) {
            autoLapMin = kAutoLapMinutes[i];
            return true;
        }
    }
    return false;
}

} // namespace Workout
