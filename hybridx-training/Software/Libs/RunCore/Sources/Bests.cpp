/**
 ******************************************************************************
 * @file    Bests.cpp
 * @brief   HybridX Run: personal bests (see the header).
 ******************************************************************************
 */

#include "Bests.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

namespace RunVo2
{

bool Bests::plausible(uint8_t i, uint32_t ms)
{
    if (i >= kCount || ms == 0) {
        return false;
    }
    const double metres  = StatsConfig::kSplitM[i];
    const double fastest = metres / kMaxSpeedMs * 1000.0;
    const double slowest = metres / kMinSpeedMs * 1000.0;
    return ms >= fastest && ms <= slowest;
}

uint8_t Bests::update(const uint32_t bestMs[kCount], uint32_t runUtc)
{
    uint8_t improved = 0;
    if (bestMs == nullptr) {
        return 0;
    }
    for (uint8_t i = 0; i < kCount; ++i) {
        const uint32_t ms = bestMs[i];
        if (!plausible(i, ms)) {
            continue;
        }
        if (mBest[i].ms == 0 || ms < mBest[i].ms) {
            mBest[i].ms  = ms;
            mBest[i].utc = runUtc;
            improved     = static_cast<uint8_t>(improved | (1u << i));
        }
    }
    return improved;
}

const Bests::Best& Bests::at(uint8_t i) const
{
    static const Best kNone {};
    return i < kCount ? mBest[i] : kNone;
}

size_t Bests::toJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(1));
    w.startArray("b");
    for (uint8_t i = 0; i < kCount; ++i) {
        w.startArray();
        w.add(mBest[i].ms);
        w.add(mBest[i].utc);
        w.endArray();
    }
    w.endArray();
    w.endMap();
    if (w.isError()) {
        buf[0] = '\0';
        return 0;
    }
    return std::strlen(buf);
}

bool Bests::fromJson(const char* text, size_t len)
{
    clear();
    if (text == nullptr || len == 0) {
        return false;
    }
    SDK::JsonStreamReader r(text, len);
    if (!r.validate()) {
        return false;
    }
    int64_t v = 0;
    if (!r.get("v", v) || v != 1) {
        return false;
    }
    size_t n = 0;
    if (!r.getArrayLength("b", n) || n != kCount) {
        return false;
    }
    Bests b;
    for (uint8_t i = 0; i < kCount; ++i) {
        char q[16];
        size_t fields = 0;
        std::snprintf(q, sizeof(q), "b[%u]", static_cast<unsigned>(i));
        if (!r.getArrayLength(q, fields) || fields != 2) {
            return false;
        }
        int64_t ms = 0, utc = 0;
        std::snprintf(q, sizeof(q), "b[%u][0]", static_cast<unsigned>(i));
        if (!r.get(q, ms) || ms < 0 || ms > 0xFFFFFFFFll) {
            return false;
        }
        std::snprintf(q, sizeof(q), "b[%u][1]", static_cast<unsigned>(i));
        if (!r.get(q, utc) || utc < 0 || utc > 0xFFFFFFFFll) {
            return false;
        }
        if (ms != 0 && !plausible(i, static_cast<uint32_t>(ms))) {
            return false;
        }
        b.mBest[i].ms  = static_cast<uint32_t>(ms);
        b.mBest[i].utc = ms != 0 ? static_cast<uint32_t>(utc) : 0;
    }
    *this = b;
    return true;
}

}  // namespace RunVo2
