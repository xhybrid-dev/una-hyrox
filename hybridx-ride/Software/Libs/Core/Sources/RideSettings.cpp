/**
 ******************************************************************************
 * @file    RideSettings.cpp
 * @brief   The settings file (see the header).
 ******************************************************************************
 */

#include "RideSettings.hpp"

#include <cstring>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"
#include "RideModel.hpp"

namespace Ride {

size_t RideSettings::toJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(1));
    w.add("autolap", autoLapMin);
    w.add("outdoor", static_cast<uint8_t>(outdoor ? 1 : 0));
    w.endMap();
    if (w.isError()) {
        buf[0] = '\0';
        return 0;
    }
    return std::strlen(buf);
}

bool RideSettings::fromJson(const char* text, size_t len)
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
        if (kAutoLapMinutes[i] != lap) {
            continue;
        }
        bool mode = outdoor;          // a file without the key keeps the default
        int64_t out = 0;
        if (r.get("outdoor", out)) {
            if (out != 0 && out != 1) {
                return false;
            }
            mode = out == 1;
        }
        autoLapMin = kAutoLapMinutes[i];   // nothing is changed unless the whole file is good
        outdoor    = mode;
        return true;
    }
    return false;
}

} // namespace Ride
