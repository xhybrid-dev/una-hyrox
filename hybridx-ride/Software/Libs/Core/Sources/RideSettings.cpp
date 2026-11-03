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
    const PlanConfig p = clamp(plan);
    w.add("v", static_cast<uint8_t>(2));
    w.add("autolap", autoLapMin);
    w.add("outdoor", static_cast<uint8_t>(outdoor ? 1 : 0));
    w.add("autopause", static_cast<uint8_t>(autoPause ? 1 : 0));
    w.add("lapkm", autoLapKm);
    w.add("type", static_cast<uint8_t>(p.type));
    w.add("work", p.workSec);
    w.add("rest", p.restSec);
    w.add("rounds", p.rounds);
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
    if (!r.validate() || !r.get("v", v) || (v != 1 && v != 2) || !r.get("autolap", lap)) {
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
        // Version 2's keys, each optional (a version 1 file has none of them).
        bool       pause = true;
        uint8_t    km    = 0;
        PlanConfig p;
        int64_t    x     = 0;
        if (r.get("autopause", x)) {
            pause = x != 0;
        }
        if (r.get("lapkm", x)) {
            bool known = false;
            for (uint8_t k = 0; k < kAutoLapKmCount; ++k) {
                known = known || kAutoLapKm[k] == x;
            }
            if (!known) {
                return false;
            }
            km = static_cast<uint8_t>(x);
        }
        auto u16 = [](int64_t y) { return static_cast<uint16_t>(y < 0 ? 0 : (y > 65535 ? 65535 : y)); };
        if (r.get("type", x)) {
            p.type = static_cast<SessionType>(x < 0 || x > 255 ? 255 : x);   // unknown: clamp() makes it Free
        }
        if (r.get("work", x)) {
            p.workSec = u16(x);
        }
        if (r.get("rest", x)) {
            p.restSec = u16(x);
        }
        if (r.get("rounds", x)) {
            p.rounds = static_cast<uint8_t>(x < 0 ? 0 : (x > 255 ? 255 : x));
        }
        autoLapMin = kAutoLapMinutes[i];   // nothing is changed unless the whole file is good
        autoLapKm  = autoLapMin > 0 ? 0 : km;   // one auto lap at a time
        outdoor    = mode;
        autoPause  = pause;
        plan       = clamp(p);
        return true;
    }
    return false;
}

} // namespace Ride
