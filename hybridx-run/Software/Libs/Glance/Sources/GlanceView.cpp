/**
 ******************************************************************************
 * @file    GlanceView.cpp
 * @brief   What the VO2max glance has to show (see the header).
 ******************************************************************************
 */

#include "GlanceView.hpp"

#include <cstdio>

#include "SDK/JSON/JsonStreamReader.hpp"

namespace RunVo2
{
namespace Glance
{

namespace
{
// The figures the app keeps (Vo2History reads runs in this range).
constexpr int64_t kMinX10 = static_cast<int64_t>(Config::kMinPlausibleVo2 * 10.0f);
constexpr int64_t kMaxX10 = static_cast<int64_t>(Config::kMaxPlausibleVo2 * 10.0f);

/// As Vo2History's: the SDK reader's getters do no range check.
bool readInt(const SDK::JsonStreamReader& r, const char* query, int64_t min, int64_t max, int64_t& out)
{
    int64_t v = 0;
    if (!r.get(query, v) || v < min || v > max) {
        return false;
    }
    out = v;
    return true;
}

bool read(const char* text, size_t len, View& v)
{
    SDK::JsonStreamReader r(text, len);
    if (!r.validate()) {
        return false;
    }
    int64_t n = 0;
    if (!readInt(r, "v", 2, 2, n)) {
        return false;   // version 1 has no trend; the app rewrites it on opening
    }
    if (!readInt(r, "runs", 0, kGlanceRuns, n)) {
        return false;
    }
    v.runs = static_cast<uint8_t>(n);
    if (v.runs == 0) {
        return true;   // a good file with no runs yet
    }
    if (!readInt(r, "vo2maxX10", kMinX10, kMaxX10, n)) {
        return false;
    }
    v.vo2x10 = static_cast<uint16_t>(n);
    // 0 = no figure before the latest run (the first run).
    if (!readInt(r, "beforeX10", 0, kMaxX10, n) || (n != 0 && n < kMinX10)) {
        return false;
    }
    v.beforeX10 = static_cast<uint16_t>(n);

    size_t points = 0;
    if (!r.getArrayLength("trend", points) || points == 0 || points > kGlanceRuns) {
        return false;
    }
    for (size_t i = 0; i < points; ++i) {
        char q[16];
        std::snprintf(q, sizeof(q), "trend[%u]", static_cast<unsigned>(i));
        if (!readInt(r, q, kMinX10, kMaxX10, n)) {
            return false;
        }
        v.trend[i] = static_cast<uint16_t>(n);
    }
    v.count = static_cast<uint8_t>(points);
    v.hasData = true;
    return true;
}
}  // namespace

bool makeView(const char* text, size_t len, View& out)
{
    out = View {};
    if (text == nullptr || len == 0 || len > kMaxFileBytes) {
        return false;
    }
    View v;
    if (!read(text, len, v)) {
        return false;
    }
    out = v;
    return out.hasData;
}

}  // namespace Glance
}  // namespace RunVo2
