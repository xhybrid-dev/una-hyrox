/**
 ******************************************************************************
 * @file    Vo2History.cpp
 * @brief   The VO2max history ring, the run maxima, and their JSON.
 ******************************************************************************
 */

#include "Vo2History.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include "Vo2Profile.hpp"

namespace RunVo2
{

namespace
{

/// The SDK reader's integer getters do no type or range check (strtoul on
/// whatever is there), so read as int64 and bound it here.
bool readInt(const SDK::JsonStreamReader& r, const char* query, int64_t min, int64_t max, int64_t& out)
{
    int64_t v = 0;
    if (!r.get(query, v) || v < min || v > max) {
        return false;
    }
    out = v;
    return true;
}

}  // namespace

void Vo2History::add(const RunRecord& r)
{
    if (mCount < Config::kHistoryRuns) {
        mRuns[(mFirst + mCount) % Config::kHistoryRuns] = r;
        ++mCount;
    } else {
        mRuns[mFirst] = r;
        mFirst = static_cast<uint8_t>((mFirst + 1) % Config::kHistoryRuns);
    }
}

void Vo2History::addSorted(const RunRecord& r)
{
    RunRecord tmp[Config::kHistoryRuns + 1];
    uint8_t   n      = 0;
    bool      placed = false;
    for (uint8_t i = 0; i < mCount; ++i) {
        const RunRecord& e = at(i);
        if (!placed && r.utc < e.utc) {
            tmp[n++] = r;
            placed   = true;
        }
        tmp[n++] = e;
    }
    if (!placed) {
        tmp[n++] = r;
    }
    const uint8_t skip = n > Config::kHistoryRuns ? static_cast<uint8_t>(n - Config::kHistoryRuns) : 0;
    mFirst = 0;
    mCount = static_cast<uint8_t>(n - skip);
    for (uint8_t i = 0; i < mCount; ++i) {
        mRuns[i] = tmp[skip + i];
    }
}

uint8_t Vo2History::countFrom(RunSource src) const
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < mCount; ++i) {
        if (at(i).source == static_cast<uint8_t>(src)) {
            ++n;
        }
    }
    return n;
}

const RunRecord& Vo2History::at(uint8_t i) const
{
    static const RunRecord kNone{};
    if (i >= mCount) {
        return kNone;
    }
    return mRuns[(mFirst + i) % Config::kHistoryRuns];
}

uint16_t Vo2History::rollingOver(uint8_t n) const
{
    uint32_t sum = 0;
    uint32_t weight = 0;
    if (n > mCount) {
        n = mCount;
    }
    const uint8_t from = n > Config::kRollingRuns ? n - Config::kRollingRuns : 0;
    for (uint8_t i = from; i < n; ++i) {
        const RunRecord& r = at(i);
        const uint32_t w = r.windows < Config::kMaxRunWeight ? r.windows : Config::kMaxRunWeight;
        sum    += static_cast<uint32_t>(r.vo2x10) * w;
        weight += w;
    }
    if (weight == 0) {
        return 0;
    }
    return static_cast<uint16_t>((sum + weight / 2) / weight);
}

void Vo2History::addRunMax(uint8_t bpm)
{
    if (bpm < Config::kMinHrBpm || bpm > Config::kMaxHrBpm) {
        return;
    }
    if (mMaxCount < Config::kHistoryRuns) {
        mMaxes[(mMaxFirst + mMaxCount) % Config::kHistoryRuns] = bpm;
        ++mMaxCount;
    } else {
        mMaxes[mMaxFirst] = bpm;
        mMaxFirst = static_cast<uint8_t>((mMaxFirst + 1) % Config::kHistoryRuns);
    }
}

uint8_t Vo2History::autoMaxHr() const
{
    static_assert(Config::kAutoMaxConfirmRuns >= 1, "at least one run");
    if (mMaxCount < Config::kAutoMaxConfirmRuns) {
        return 0;
    }
    // The k-th highest: k passes, each taking the highest left below the last.
    uint8_t  value = 255;
    uint8_t  taken = 0;
    while (taken < Config::kAutoMaxConfirmRuns) {
        uint8_t best = 0;
        uint8_t copies = 0;
        for (uint8_t i = 0; i < mMaxCount; ++i) {
            const uint8_t m = mMaxes[i];
            if (m < value && m > best) {
                best = m;
                copies = 1;
            } else if (m == best && m < value) {
                ++copies;
            }
        }
        if (best == 0) {
            return 0;   // cannot happen with mMaxCount >= k; kept as a bound
        }
        value = best;
        taken = static_cast<uint8_t>(taken + copies);
    }
    return value;
}

void Vo2History::setWatchRestHr(uint8_t bpm)
{
    if (plausibleRestHr(bpm)) {
        mWatchRestHr = bpm;
    }
}

size_t Vo2History::toJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(3));
    w.add("restHr", mWatchRestHr);
    w.add("trail", mTrailMark);
    w.startArray("maxes");
    for (uint8_t i = 0; i < mMaxCount; ++i) {
        w.add(mMaxes[(mMaxFirst + i) % Config::kHistoryRuns]);
    }
    w.endArray();
    w.startArray("runs");
    for (uint8_t i = 0; i < mCount; ++i) {
        const RunRecord& r = at(i);
        w.startArray();
        w.add(r.utc);
        w.add(r.vo2x10);
        w.add(r.windows);
        w.add(r.source);
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

bool Vo2History::fromJson(const char* text, size_t len)
{
    clear();
    if (text == nullptr || len == 0) {
        return false;
    }
    SDK::JsonStreamReader r(text, len);
    if (!r.validate()) {
        return false;
    }

    Vo2History h;
    int64_t v = 0;
    if (!readInt(r, "v", 1, 3, v)) {
        return false;
    }
    const int64_t version = v;

    if (version == 1) {
        // R1-R3 kept one ratcheting value; it becomes one run's maximum, which
        // alone never confirms (kAutoMaxConfirmRuns).
        if (!readInt(r, "autoMaxHr", 0, Config::kMaxHrBpm, v)) {
            return false;
        }
        h.addRunMax(static_cast<uint8_t>(v));
    } else {
        if (!readInt(r, "restHr", 0, 255, v)) {
            return false;
        }
        h.setWatchRestHr(static_cast<uint8_t>(v));
        if (version >= 3) {
            if (!readInt(r, "trail", 0, 0xFFFFFFFFll, v)) {
                return false;
            }
            h.setTrailMarkUtc(static_cast<uint32_t>(v));
        }

        size_t n = 0;
        if (!r.getArrayLength("maxes", n)) {
            return false;
        }
        for (size_t i = 0; i < n; ++i) {
            char q[24];
            std::snprintf(q, sizeof(q), "maxes[%u]", static_cast<unsigned>(i));
            if (!readInt(r, q, Config::kMinHrBpm, Config::kMaxHrBpm, v)) {
                return false;
            }
            h.addRunMax(static_cast<uint8_t>(v));   // keeps the newest kHistoryRuns
        }
    }

    size_t runs = 0;
    if (!r.getArrayLength("runs", runs)) {
        return false;
    }
    for (size_t i = 0; i < runs; ++i) {
        char q[24];
        int64_t utc = 0, vo2 = 0, win = 0, src = 0;
        std::snprintf(q, sizeof(q), "runs[%u][0]", static_cast<unsigned>(i));
        bool ok = readInt(r, q, 1, 0xFFFFFFFFll, utc);
        std::snprintf(q, sizeof(q), "runs[%u][1]", static_cast<unsigned>(i));
        ok = ok && readInt(r, q, static_cast<int64_t>(Config::kMinPlausibleVo2 * 10.0f),
                           static_cast<int64_t>(Config::kMaxPlausibleVo2 * 10.0f), vo2);
        std::snprintf(q, sizeof(q), "runs[%u][2]", static_cast<unsigned>(i));
        ok = ok && readInt(r, q, Config::kMinWindows, Config::kMaxWindows, win);
        if (version >= 3) {
            std::snprintf(q, sizeof(q), "runs[%u][3]", static_cast<unsigned>(i));
            ok = ok && readInt(r, q, 0, 1, src);
        }
        if (!ok) {
            return false;
        }
        h.add(RunRecord{static_cast<uint32_t>(utc), static_cast<uint16_t>(vo2), static_cast<uint16_t>(win),
                        static_cast<uint8_t>(src)});
    }

    *this = h;
    return true;
}

size_t Vo2History::toSharedJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    const uint32_t last = mCount ? at(static_cast<uint8_t>(mCount - 1)).utc : 0;
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(1));
    w.add("vo2maxX10", rollingX10());
    w.add("runs", mCount);
    w.add("utc", last);
    w.endMap();
    if (w.isError()) {
        buf[0] = '\0';
        return 0;
    }
    return std::strlen(buf);
}

}  // namespace RunVo2
