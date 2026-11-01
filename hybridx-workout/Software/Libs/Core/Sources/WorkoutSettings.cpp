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
    const PlanConfig p = clamp(plan);
    w.add("v", static_cast<uint8_t>(2));
    w.add("autolap", autoLapMin);
    w.add("type", static_cast<uint8_t>(p.type));
    w.add("sets", p.sets);
    w.add("srest", p.setRestSec);
    w.add("scap", p.setCapSec);
    w.add("emom", p.emomMin);
    w.add("amrap", p.amrapMin);
    w.add("work", p.workSec);
    w.add("rest", p.restSec);
    w.add("rounds", p.rounds);
    w.add("crounds", p.circuitRounds);
    w.add("run", p.runM);
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
    if (!r.validate() || !r.get("v", v) || (v != 1 && v != 2) || !r.get("autolap", lap)) {
        return false;
    }
    bool known = false;
    for (uint8_t i = 0; i < kAutoLapCount; ++i) {
        if (kAutoLapMinutes[i] == lap) {
            known = true;
        }
    }
    if (!known) {
        return false;
    }

    PlanConfig p;   // the defaults; version 1 has nothing more
    if (v == 2) {
        // Clamp to the field's own width first, then clamp() pulls it into range.
        auto u8 = [&](const char *key, uint8_t &out) {
            int64_t x = 0;
            if (r.get(key, x)) {
                out = static_cast<uint8_t>(x < 0 ? 0 : (x > 255 ? 255 : x));
            }
        };
        auto u16 = [&](const char *key, uint16_t &out) {
            int64_t x = 0;
            if (r.get(key, x)) {
                out = static_cast<uint16_t>(x < 0 ? 0 : (x > 65535 ? 65535 : x));
            }
        };
        uint8_t type = 0;
        u8("type", type);
        p.type = static_cast<SessionType>(type);
        u8("sets", p.sets);
        u16("srest", p.setRestSec);
        u16("scap", p.setCapSec);
        u8("emom", p.emomMin);
        u8("amrap", p.amrapMin);
        u16("work", p.workSec);
        u16("rest", p.restSec);
        u8("rounds", p.rounds);
        u8("crounds", p.circuitRounds);
        u16("run", p.runM);
    }
    autoLapMin = static_cast<uint8_t>(lap);
    plan = clamp(p);
    return true;
}

} // namespace Workout
